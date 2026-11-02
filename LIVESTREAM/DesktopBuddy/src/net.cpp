/*
 * The internet side of Buddy:
 *  - WiFi setup from a phone: Buddy makes a "DesktopBuddy" WiFi network with
 *    a setup page (pops up by itself on most phones, or http://192.168.4.1)
 *  - stock prices from Yahoo Finance, fetched in the background on the other
 *    CPU core so the face never freezes while it waits
 *  - a little web page at http://buddy.local to pick stocks and price targets
 */

#include "net.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

static const char *AP_NAME = "DesktopBuddy";
static const char *DEFAULT_STOCKS = "AAPL;MSFT;NVDA;GOOGL;AMZN;META;TSLA;AVGO";
static const char *BROWSER = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36";
static const uint32_t SETUP_TIMEOUT_MS = 5 * 60 * 1000UL;

static Stock stocks[MAX_STOCKS];
static int numStocks = 0;
static SemaphoreHandle_t lock;          // stops both cores using the stock list at once
static volatile bool refreshNow = true;
static WebServer server(80);
static bool started = false;            // clock, buddy.local and the phone page are running

// what the phone is doing during WiFi setup (set from WiFi events)
static volatile bool phoneJoined = false, phoneGotAddress = false, pageOpened = false;

// ---------- settings ----------
// Stocks are saved as "AAPL:150.00:0.00;MSFT:0.00:500.00;..." (ticker:buy below:sell above)

static String cleanSymbol(String s) {
  s.trim();
  s.toUpperCase();
  String out;
  for (char ch : s) {
    if (isalnum(ch) || ch == '.' || ch == '-' || ch == '^' || ch == '=') out += ch;
  }
  return out.substring(0, sizeof(Stock::symbol) - 1);
}

// call with the lock held
static void parseStocks(const String &s) {
  Stock old[MAX_STOCKS];
  int oldCount = numStocks;
  memcpy(old, stocks, sizeof stocks);

  numStocks = 0;
  int start = 0;
  while (start < (int)s.length() && numStocks < MAX_STOCKS) {
    int end = s.indexOf(';', start);
    if (end < 0) end = s.length();
    String item = s.substring(start, end);
    start = end + 1;

    int c1 = item.indexOf(':'), c2 = item.indexOf(':', c1 + 1);
    String sym = cleanSymbol(c1 < 0 ? item : item.substring(0, c1));
    if (!sym.length()) continue;

    Stock &st = stocks[numStocks++];
    memset(&st, 0, sizeof st);
    strlcpy(st.symbol, sym.c_str(), sizeof st.symbol);
    if (c1 >= 0) st.buyBelow = item.substring(c1 + 1, c2 < 0 ? item.length() : c2).toFloat();
    if (c2 >= 0) st.sellAbove = item.substring(c2 + 1).toFloat();

    // keep the price we already had for this ticker
    for (int i = 0; i < oldCount; i++) {
      if (!strcmp(old[i].symbol, st.symbol)) {
        st.price = old[i].price;
        st.change = old[i].change;
        st.valid = old[i].valid;
        strlcpy(st.name, old[i].name, sizeof st.name);
      }
    }
  }
}

static String getSetting(const char *key, const char *fallback = "") {
  Preferences prefs;
  prefs.begin("buddy", false);
  String s = prefs.getString(key, fallback);
  prefs.end();
  return s;
}

static void putSetting(const char *key, const String &value) {
  Preferences prefs;
  prefs.begin("buddy", false);
  prefs.putString(key, value);
  prefs.end();
}

static String htmlEscape(const String &s) {
  String out;
  for (char ch : s) {
    if (ch == '&') out += "&amp;";
    else if (ch == '<') out += "&lt;";
    else if (ch == '>') out += "&gt;";
    else if (ch == '\'') out += "&#39;";
    else if (ch == '"') out += "&quot;";
    else out += ch;
  }
  return out;
}

static const char PAGE_STYLE[] PROGMEM =
  "<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>Desktop Buddy</title><style>"
  "body{font-family:system-ui,sans-serif;background:#0a0c1c;color:#e8f6ff;margin:0 auto;padding:16px;max-width:520px}"
  "h1{color:#6ee1ff;font-size:1.5em;margin:0 0 4px}p,label{color:#8a96aa;font-size:.9em}"
  "table{width:100%;border-collapse:collapse}td,th{padding:4px 3px;text-align:left}"
  "th{color:#8a96aa;font-weight:normal;font-size:.8em}"
  "input,select{width:100%;box-sizing:border-box;padding:9px 6px;border-radius:8px;border:1px solid #2a3350;"
  "background:#141a33;color:#fff;font-size:16px;margin:4px 0 10px}"
  "td input{margin:0}td.now{font-size:.85em;white-space:nowrap}.up{color:#28e65a}.down{color:#ff4b4b}"
  "button{margin-top:14px;width:100%;padding:13px;border:0;border-radius:10px;background:#6ee1ff;"
  "color:#0a0c1c;font-size:17px;font-weight:bold}"
  "button.alt{background:#2a3350;color:#e8f6ff;font-weight:normal;font-size:14px}"
  "</style>";

// ---------- stock prices ----------

static String urlEncode(const char *s) {
  String out;
  for (; *s; s++) {
    if (*s == '^') out += "%5E";
    else if (*s == '=') out += "%3D";
    else out += *s;
  }
  return out;
}

static bool fetchQuote(const char *sym, float &price, float &prevClose, char *name, size_t nameLen) {
  const char *hosts[] = { "query1.finance.yahoo.com", "query2.finance.yahoo.com" };
  for (const char *host : hosts) {
    WiFiClientSecure client;
    client.setInsecure();   // skip certificate checks - fine for public prices
    HTTPClient http;
    http.useHTTP10(true);   // plain (not chunked) reply, so we can read it as a stream
    http.setTimeout(10000);
    String url = String("https://") + host + "/v8/finance/chart/" + urlEncode(sym) + "?range=1d&interval=1d";
    if (!http.begin(client, url)) continue;
    http.setUserAgent(BROWSER);

    int code = http.GET();
    if (code != 200) {
      Serial.printf("Yahoo %s: HTTP %d\n", sym, code);
      http.end();
      continue;
    }

    // only keep the few numbers we need from the big reply
    JsonDocument filter;
    filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
    filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;
    filter["chart"]["result"][0]["meta"]["previousClose"] = true;
    filter["chart"]["result"][0]["meta"]["shortName"] = true;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (err) {
      Serial.printf("Yahoo %s: %s\n", sym, err.c_str());
      continue;
    }

    JsonObject meta = doc["chart"]["result"][0]["meta"];
    price = meta["regularMarketPrice"] | 0.0f;
    prevClose = meta["previousClose"] | (meta["chartPreviousClose"] | 0.0f);
    strlcpy(name, meta["shortName"] | sym, nameLen);
    if (price > 0 && prevClose > 0) return true;
  }
  return false;
}

static void fetchAllStocks() {
  char syms[MAX_STOCKS][sizeof(Stock::symbol)];
  xSemaphoreTake(lock, portMAX_DELAY);
  int n = numStocks;
  for (int i = 0; i < n; i++) strlcpy(syms[i], stocks[i].symbol, sizeof syms[i]);
  xSemaphoreGive(lock);

  for (int i = 0; i < n; i++) {
    float price, prev;
    char name[sizeof(Stock::name)];
    if (!fetchQuote(syms[i], price, prev, name, sizeof name)) continue;

    xSemaphoreTake(lock, portMAX_DELAY);
    if (i < numStocks && !strcmp(stocks[i].symbol, syms[i])) {   // list may have changed meanwhile
      stocks[i].price = price;
      stocks[i].change = (price - prev) / prev * 100;
      strlcpy(stocks[i].name, name, sizeof stocks[i].name);
      stocks[i].valid = true;
    }
    xSemaphoreGive(lock);
    Serial.printf("%-6s %10.2f  %+.2f%%\n", syms[i], price, (price - prev) / prev * 100);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ---------- weather (Open-Meteo - free, no key) ----------

static volatile int weatherCode = -1;
static volatile bool weatherDay = true;
static volatile float weatherTemp = 0;

static bool fetchWeather() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(10000);
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(WEATHER_LAT, 3) +
               "&longitude=" + String(WEATHER_LON, 3) + "&current=temperature_2m,is_day,weather_code";
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != 200) {
    Serial.printf("Weather: HTTP %d\n", code);
    http.end();
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) return false;

  JsonObject now = doc["current"];
  if (now["weather_code"].isNull()) return false;
  weatherTemp = now["temperature_2m"] | 0.0f;
  weatherDay = (now["is_day"] | 1) == 1;
  weatherCode = now["weather_code"] | 0;
  Serial.printf("Weather: code %d, %.1fC, %s\n", (int)weatherCode, (float)weatherTemp, weatherDay ? "day" : "night");
  return true;
}

// Runs on core 0 - keeps prices and weather fresh without ever pausing the animation
static void netTask(void *) {
  uint32_t lastFetch = 0, lastWeather = 0;
  bool haveWeather = false;
  for (;;) {
    if (started && WiFi.status() == WL_CONNECTED) {
      if (refreshNow || millis() - lastFetch > STOCK_REFRESH_MS) {
        refreshNow = false;
        fetchAllStocks();
        lastFetch = millis();
      }
      if (!haveWeather || millis() - lastWeather > WEATHER_REFRESH_MS) {
        haveWeather = fetchWeather() || haveWeather;
        lastWeather = millis();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ---------- stocks page (http://buddy.local) ----------

static void handleRoot() {
  Stock snap[MAX_STOCKS];
  int n = netGetStocks(snap);

  String h;
  h.reserve(7000);
  h += F("<!DOCTYPE html><html><head>");
  h += FPSTR(PAGE_STYLE);
  h += F("</head><body><h1>&#128512; Desktop Buddy</h1>"
         "<p>Up to 8 tickers (AAPL, BTC-USD, ^GSPC...). Add a price to <b>buy below</b> or <b>sell above</b> "
         "and Buddy flashes when it gets there. Tap the stocks button on Buddy to see them.</p>"
         "<form method='POST' action='/save'><table>"
         "<tr><th>Ticker</th><th>Buy below</th><th>Sell above</th><th>Now</th></tr>");

  for (int i = 0; i < MAX_STOCKS; i++) {
    const Stock *s = i < n ? &snap[i] : nullptr;
    String idx(i);
    h += "<tr><td><input name='s" + idx + "' value='" + (s ? s->symbol : "") +
         "' autocapitalize='characters' autocomplete='off' maxlength='11'></td>";
    h += "<td><input name='b" + idx + "' inputmode='decimal' value='" +
         (s && s->buyBelow > 0 ? String(s->buyBelow, 2) : "") + "'></td>";
    h += "<td><input name='a" + idx + "' inputmode='decimal' value='" +
         (s && s->sellAbove > 0 ? String(s->sellAbove, 2) : "") + "'></td><td class='now'>";
    if (s && s->valid) {
      h += String("<span class='") + (s->change >= 0 ? "up'>&#9650;" : "down'>&#9660;") +
           String(s->price, 2) + "<br>" + String(s->change, 2) + "%</span>";
    }
    h += "</td></tr>";
  }

  h += F("</table><button>Save</button></form>"
         "<form method='POST' action='/forgetwifi' onsubmit=\"return confirm('Forget this WiFi? Buddy restarts "
         "offline. To set up a new one: hold STOCKS on Buddy, then tap TICKLE.')\">"
         "<button class='alt'>Change WiFi network</button></form></body></html>");
  server.send(200, "text/html", h);
}

static void handleSave() {
  String s;
  for (int i = 0; i < MAX_STOCKS; i++) {
    String idx(i);
    String sym = cleanSymbol(server.arg("s" + idx));
    if (!sym.length()) continue;
    s += sym + ":" + String(server.arg("b" + idx).toFloat(), 2) + ":" + String(server.arg("a" + idx).toFloat(), 2) + ";";
  }
  xSemaphoreTake(lock, portMAX_DELAY);
  parseStocks(s);
  xSemaphoreGive(lock);
  putSetting("stocks", s);
  refreshNow = true;
  Serial.println("Saved stocks: " + s);

  server.sendHeader("Location", "/");
  server.send(303);
}

static void handleForgetWifi() {
  server.send(200, "text/html", "<meta name='viewport' content='width=device-width'>"
                                "<body style='font-family:sans-serif'><h2>Restarting...</h2>"
                                "<p>To set up a new WiFi: hold STOCKS on Buddy, then tap TICKLE.</p></body>");
  delay(500);
  putSetting("ssid", "");
  putSetting("pass", "");
  ESP.restart();
}

// Once WiFi is connected, start everything that needs it
static void startOnline() {
  started = true;
  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());
  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");
  if (MDNS.begin("buddy")) MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/forgetwifi", HTTP_POST, handleForgetWifi);
  server.begin();
  refreshNow = true;
}

static bool joinWifi(const String &ssid, const String &pass, uint32_t timeoutMs, void (*whileWaiting)() = nullptr) {
  Serial.printf("Joining WiFi '%s'...\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    if (whileWaiting) whileWaiting();
    delay(20);
  }
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.printf("Couldn't join '%s' (status %d)\n", ssid.c_str(), WiFi.status());
  WiFi.disconnect();
  return false;
}

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      phoneJoined = true;
      Serial.println("Setup: a phone joined DesktopBuddy");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      phoneGotAddress = true;
      Serial.printf("Setup: gave the phone address %s\n", IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("Setup: a phone left DesktopBuddy");
      break;
    default:
      break;
  }
}

// ---------- public ----------

void netBegin() {
  lock = xSemaphoreCreateMutex();
  xSemaphoreTake(lock, portMAX_DELAY);
  parseStocks(getSetting("stocks", DEFAULT_STOCKS));
  xSemaphoreGive(lock);

  WiFi.persistent(false);   // we keep the WiFi details ourselves
  WiFi.onEvent(onWifiEvent);
  WiFi.mode(WIFI_STA);

  String ssid = getSetting("ssid");
  if (ssid.length() && joinWifi(ssid, getSetting("pass"), 15000)) startOnline();
  else Serial.println("No WiFi - Buddy works offline. Hold STOCKS, then tap TICKLE to set up WiFi");

  xTaskCreatePinnedToCore(netTask, "net", 16384, nullptr, 1, nullptr, 0);
}

// Phone WiFi setup. Buddy makes his own "DesktopBuddy" network with a setup page.
// tick() is called all the time with a status message - it redraws the screen,
// and returns false to cancel
bool netWifiSetup(bool (*tick)(const char *status)) {
  // look for WiFi networks first - scanning later would knock the phone off
  tick("Looking for WiFi...");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  int found = WiFi.scanNetworks();
  String options;
  int listed = 0;
  for (int i = 0; i < found && listed < 20; i++) {
    String name = WiFi.SSID(i);
    if (!name.length() || options.indexOf(">" + htmlEscape(name) + "<") >= 0) continue;   // hidden or already listed
    options += "<option>" + htmlEscape(name) + "</option>";
    listed++;
  }
  WiFi.scanDelete();
  Serial.printf("Setup: found %d WiFi networks\n", listed);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME);
  delay(200);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("Setup: join '%s' and open http://%s\n", AP_NAME, ip.toString().c_str());
  phoneJoined = false;
  phoneGotAddress = false;
  pageOpened = false;

  // answer every web address with our page, so phones pop it up by themselves
  DNSServer dns;
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", ip);

  WebServer portal(80);
  String newSsid, newPass;
  bool tryJoin = false;
  const char *problem = nullptr;

  portal.on("/", [&]() {
    pageOpened = true;
    Serial.println("Setup: phone opened the setup page");
    String h = F("<!DOCTYPE html><html><head>");
    h += FPSTR(PAGE_STYLE);
    h += F("</head><body><h1>&#128512; Desktop Buddy</h1><p>Pick your home WiFi so I can get stock prices.</p>"
           "<form method='POST' action='/join'><label>WiFi network</label><select name='ssid'>");
    h += options;
    h += F("</select><label>...or type its name (if it's not in the list)</label>"
           "<input name='other' autocomplete='off' autocapitalize='none'>"
           "<label>Password</label><input name='pass' type='password' autocomplete='off'>"
           "<button>Connect</button></form>"
           "<p>Only 2.4GHz WiFi works. If your router has two names, pick the one without 5G.</p></body></html>");
    portal.send(200, "text/html", h);
  });
  portal.on("/join", HTTP_POST, [&]() {
    newSsid = portal.arg("other");
    newSsid.trim();
    if (!newSsid.length()) newSsid = portal.arg("ssid");
    newPass = portal.arg("pass");
    tryJoin = true;
    portal.send(200, "text/html", String(F("<!DOCTYPE html><html><head>")) + FPSTR(PAGE_STYLE) +
                F("</head><body><h1>Connecting...</h1><p>Watch Buddy's screen - he'll laugh when he's online. "
                  "If he says the password is wrong, join DesktopBuddy again and have another go.</p></body></html>"));
  });
  portal.onNotFound([&]() {   // phones check a "test" address when they join - send them to the page
    portal.sendHeader("Location", String("http://") + ip.toString() + "/");
    portal.send(302, "text/plain", "");
  });
  portal.begin();

  bool ok = false;
  uint32_t start = millis();
  while (millis() - start < SETUP_TIMEOUT_MS) {
    dns.processNextRequest();
    portal.handleClient();

    if (tryJoin) {
      tryJoin = false;
      tick("Joining your WiFi...");
      delay(500);   // let the "Connecting..." page reach the phone
      WiFi.mode(WIFI_AP_STA);
      if (joinWifi(newSsid, newPass, 20000, [] { delay(1); })) {
        putSetting("ssid", newSsid);
        putSetting("pass", newPass);
        ok = true;
        break;
      }
      WiFi.mode(WIFI_AP);
      problem = "Wrong password?";
      start = millis();   // give them time to try again
    }

    const char *status = problem          ? problem
                       : pageOpened       ? "Pick WiFi on phone"
                       : phoneGotAddress  ? "Open 192.168.4.1"
                       : phoneJoined      ? "Phone joining..."
                       :                    "Waiting for phone";
    if (!tick(status)) break;
  }

  dns.stop();
  portal.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  if (ok) {
    if (!started) startOnline();
  } else {
    Serial.println("Setup: stopped without WiFi");
    String ssid = getSetting("ssid");
    if (ssid.length()) joinWifi(ssid, getSetting("pass"), 10000);   // back to the old WiFi if there was one
  }
  return ok;
}

void netLoop() {
  if (started) server.handleClient();
  else if (WiFi.status() == WL_CONNECTED) startOnline();
}

int netGetStocks(Stock *out) {
  xSemaphoreTake(lock, portMAX_DELAY);
  int n = numStocks;
  memcpy(out, stocks, n * sizeof(Stock));
  xSemaphoreGive(lock);
  return n;
}

bool netGetWeather(int &code, bool &isDay, float &tempC) {
  if (weatherCode < 0) return false;
  code = weatherCode;
  isDay = weatherDay;
  tempC = weatherTemp;
  return true;
}

bool netOnline() { return WiFi.status() == WL_CONNECTED; }

String netAddress() { return WiFi.localIP().toString(); }
