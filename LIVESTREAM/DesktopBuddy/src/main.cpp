/*
 * DESKTOP BUDDY
 * An animated desk friend with stock prices and good vibes
 * ESP32 Dev Module + Adafruit 1.44" 128x128 TFT + 3 conductive fabric buttons
 * By CMozMaker YouTube Channel - https://www.youtube.com/@CMozMaker
 *
 * 2026
 *
 * How it works:
 *   - Buddy is a pixel kitten in her little room: she wanders about, sits and
 *     blinks, bats her yarn ball across the floor, stretches, washes her paw,
 *     chases her tail, climbs the walls and takes little cat naps. Ignore her
 *     for a while and she gets restless - pacing about asking to be petted.
 *     The window shows the real weather outside.
 *     Late at night she curls up asleep (screen dims) - any touch wakes her up
 *   - TICKLE button: she pounces on the yarn, hearts float up, and a bit more
 *     yarn unwinds across the room. Keep going and it gets messier and messier
 *     until the whole screen floods pink... then it's all tidied up.
 *     Five quick tickles = zoomies!
 *   - Secret: 3 quick tickles, then touch and HOLD the 4th (message in config.h)
 *   - Birthday (set in config.h): party hat, confetti and birthday wishes all day
 *   - STOCKS button: first touch shows all stocks, then each touch shows the
 *     next one big. Green = up today, red = down. Flashes on a big move, or when
 *     a stock reaches a buy/sell price set on the phone page.
 *     Hold it to see the phone page address
 *   - VIBES button: a positive quote, a new one each touch
 *   - Pick stocks from a phone: http://buddy.local (same WiFi)
 *   - WiFi setup: hold STOCKS for the phone screen, then tap TICKLE. Scan the
 *     QR code with a phone camera (or join the "DesktopBuddy" WiFi) - the setup
 *     page pops up, or open http://192.168.4.1. Pick the home WiFi (2.4GHz).
 *     Tap TICKLE to cancel
 *   - Settings (pins, time zone, birthday...) are in config.h
 *
 * Serial Monitor (115200 baud):
 *   t = show/hide live touch readings (for tuning)
 *   c = re-calibrate the touch buttons (hands off!)
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <qrcode.h>   // QR codes - built into the ESP32 core
#include "config.h"
#include "cat.h"
#include "net.h"
#include "quotes.h"

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(128, 128);   // draw each frame here, then send it in one go (no flicker)

const uint16_t BG       = rgb(8, 10, 28);
const uint16_t WHITE    = rgb(255, 255, 255);
const uint16_t GREY     = rgb(140, 150, 170);
const uint16_t CYAN     = rgb(110, 225, 255);
const uint16_t GOLD     = rgb(255, 200, 40);
const uint16_t GREEN    = rgb(40, 230, 90);
const uint16_t RED      = rgb(255, 60, 60);
const uint16_t GREEN_DK = rgb(0, 90, 30);
const uint16_t RED_DK   = rgb(110, 0, 10);

// ---------- touch buttons ----------
// On the classic ESP32 a touch makes the reading DROP

struct TouchPad {
  uint8_t pin;
  const char *name;
  int baseline, value;
  uint8_t confirm;
  bool down;
  uint32_t downAt;
  bool longDone;
};
TouchPad pads[3] = {
  { TOUCH_TICKLE, "tickle" },
  { TOUCH_STOCKS, "stocks" },
  { TOUCH_VIBES,  "vibes" },
};
enum { PAD_TICKLE, PAD_STOCKS, PAD_VIBES };
bool showReadings = false;

// ---------- state ----------
enum Screen { SCR_CAT, SCR_LIST, SCR_STOCK, SCR_VIBES, SCR_PHONE, SCR_SECRET };
Screen screen = SCR_CAT;
uint32_t lastTouch = 0;
bool night = false;

uint32_t tickleTimes[4];   // the last few tickle presses, for spotting the secret
int tickleIdx = 0;
uint32_t secretAt = 0;

Stock snap[MAX_STOCKS];   // this frame's copy of the prices
int nSnap = 0;
int stockIdx = 0;

int quoteIdx = -1;
uint32_t nextNews = 15000;   // next time the kitten reacts to the stocks

// ---------- helpers ----------

void pushCanvas() {
  tft.startWrite();
  tft.setAddrWindow(0, 0, 128, 128);
  tft.writePixels(canvas.getBuffer(), 128 * 128);
  tft.endWrite();
}

void setBacklight(uint8_t level) {
#if TFT_LITE >= 0
  static int last = -1;
  if (level != last) ledcWrite(TFT_LITE, level);
  last = level;
#endif
}

bool blinkOn() { return (millis() / 400) % 2; }

bool buyHit(const Stock &s)  { return s.valid && s.buyBelow > 0 && s.price <= s.buyBelow; }
bool sellHit(const Stock &s) { return s.valid && s.sellAbove > 0 && s.price >= s.sellAbove; }

void printAt(const char *text, int x, int y, int size, uint16_t col) {
  canvas.setTextSize(size);
  canvas.setTextColor(col);
  canvas.setCursor(x, y);
  canvas.print(text);
}
void printRight(const char *text, int xRight, int y, int size, uint16_t col) {
  printAt(text, xRight - strlen(text) * 6 * size, y, size, col);
}
void printCentre(const char *text, int y, int size, uint16_t col) {
  printAt(text, (128 - (int)strlen(text) * 6 * size) / 2, y, size, col);
}

void fmtPrice(char *buf, size_t len, float p) {
  if (p >= 10000) snprintf(buf, len, "%.0f", p);
  else if (p >= 1000) snprintf(buf, len, "%.1f", p);
  else snprintf(buf, len, "%.2f", p);
}

void drawHeader(const char *title, uint16_t col) {
  canvas.fillRect(0, 0, 128, 13, col);
  printAt(title, 3, 3, 1, BG);
  struct tm now;
  if (getLocalTime(&now, 0)) {
    char b[8];
    int h = now.tm_hour % 12;
    snprintf(b, sizeof b, "%d:%02d", h ? h : 12, now.tm_min);
    printRight(b, 125, 3, 1, BG);
  }
}

// Split text into lines that fit maxChars, breaking at spaces
int wrapText(const char *text, int maxChars, String *lines, int maxLines) {
  int n = 0;
  String line, s(text);
  int i = 0;
  while (i < (int)s.length()) {
    int sp = s.indexOf(' ', i);
    if (sp < 0) sp = s.length();
    String word = s.substring(i, sp);
    i = sp + 1;
    if (!line.length()) line = word;
    else if ((int)(line.length() + 1 + word.length()) <= maxChars) line += " " + word;
    else {
      if (n < maxLines) lines[n++] = line;
      line = word;
    }
  }
  if (line.length() && n < maxLines) lines[n++] = line;
  return n;
}

// ---------- touch ----------

int touchDrop(const TouchPad &p) { return max(TOUCH_MIN_DROP, p.baseline * TOUCH_PERCENT / 100); }

void calibrateTouch() {
  Serial.println("Calibrating touch buttons - hands off...");
  for (TouchPad &p : pads) {
    long sum = 0;
    for (int i = 0; i < 16; i++) {
      sum += touchRead(p.pin);
      delay(5);
    }
    p.baseline = sum / 16;
    p.confirm = 0;
    p.down = false;
    Serial.printf("  %-6s resting %d  (touch below %d)\n", p.name, p.baseline, p.baseline - touchDrop(p));
  }
}

// Returns true the moment a new touch starts
bool readPad(TouchPad &p) {
  p.value = touchRead(p.pin);
  int drop = touchDrop(p);
  if (!p.down) {
    if (p.value < p.baseline - drop) {
      if (++p.confirm >= 2) {          // two readings in a row, so a glitch doesn't count
        p.down = true;
        p.downAt = millis();
        p.longDone = false;
        return true;
      }
    } else {
      p.confirm = 0;
      // untouched: slowly follow small changes in the resting value
      // (ignore big jumps - WiFi can make the readings spike)
      if (abs(p.value - p.baseline) < drop / 2) p.baseline = (p.baseline * 31 + p.value) / 32;
    }
  } else if (p.value > p.baseline - drop / 2) {   // let go
    p.down = false;
    p.confirm = 0;
  } else if (millis() - p.downAt > 10000) {
    // "touched" for 10 seconds straight? Nobody holds a pad that long -
    // the reading has drifted, so take this as the new resting value
    Serial.printf("%s seemed stuck - re-calibrated (%d -> %d)\n", p.name, p.baseline, p.value);
    p.baseline = p.value;
    p.down = false;
    p.confirm = 0;
  }
  return false;
}

void handleSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == 't') showReadings = !showReadings;
    if (ch == 'c') calibrateTouch();
  }
  static uint32_t lastPrint = 0;
  if (showReadings && millis() - lastPrint > 250) {
    lastPrint = millis();
    for (const TouchPad &p : pads) {
      Serial.printf("%-6s %3d (rest %3d)%s   ", p.name, p.value, p.baseline, p.down ? "*" : " ");
    }
    Serial.println();
  }
}

// ---------- what each button does ----------

void nextStockScreen() {
  if (screen == SCR_LIST && nSnap) {
    screen = SCR_STOCK;
    stockIdx = 0;
  } else if (screen == SCR_STOCK && stockIdx + 1 < nSnap) {
    stockIdx++;
  } else {
    screen = SCR_LIST;
  }
}

void nextQuote() {
  int q;
  do q = random(NUM_QUOTES); while (q == quoteIdx && NUM_QUOTES > 1);
  quoteIdx = q;
  screen = SCR_VIBES;
}

bool setupTick(const char *status);

void handleButtons() {
  uint32_t now = millis();
  bool pressed[3];
  for (int i = 0; i < 3; i++) {
    pressed[i] = readPad(pads[i]);
    if (pressed[i]) {
      lastTouch = now;
      Serial.printf("Touch: %s\n", pads[i].name);
    }
  }

  // the first touch just wakes her up
  if (catSleeping() && (pressed[0] || pressed[1] || pressed[2])) {
    catWake();
    setBacklight(255);
    screen = SCR_CAT;
    return;
  }

  // on the phone screen with no WiFi, tickle starts the phone WiFi setup
  if (pressed[PAD_TICKLE] && screen == SCR_PHONE && !netOnline()) {
    bool ok = netWifiSetup(setupTick);
    catSay(ok ? "yay, I'm online!" : "no WiFi :(", 2500);
    if (ok) catHappy();
    screen = SCR_CAT;
    lastTouch = millis();
    return;
  }
  if (pressed[PAD_TICKLE]) {
    screen = SCR_CAT;
    catTickle();
    tickleTimes[tickleIdx] = now;
    tickleIdx = (tickleIdx + 1) % 4;
  }

  // secret: 3 quick tickles, then touch and hold the 4th
  TouchPad &tp = pads[PAD_TICKLE];
  if (tp.down && !tp.longDone && now - tp.downAt > 1000) {
    tp.longDone = true;
    uint32_t first = tickleTimes[tickleIdx];   // oldest of the last 4 presses
    if (first && tp.downAt - first < 3000) {
      screen = SCR_SECRET;
      secretAt = now;
      memset(tickleTimes, 0, sizeof tickleTimes);
      Serial.println("Secret found!");
    }
  }
  if (screen == SCR_SECRET && now - secretAt > 10000) screen = SCR_CAT;
  if (pressed[PAD_STOCKS]) nextStockScreen();
  if (pressed[PAD_VIBES]) nextQuote();

  // hold the stocks button: show the phone page address
  TouchPad &sp = pads[PAD_STOCKS];
  if (sp.down && !sp.longDone && now - sp.downAt > 1500) {
    sp.longDone = true;
    screen = SCR_PHONE;
  }

  if (screen != SCR_CAT && now - lastTouch > SCREEN_TIMEOUT_MS) screen = SCR_CAT;
}

// ---------- Buddy's personality ----------

float marketMood() {   // average % change of his stocks, NAN if we don't know yet
  float sum = 0;
  int n = 0;
  for (int i = 0; i < nSnap; i++) {
    if (snap[i].valid) { sum += snap[i].change; n++; }
  }
  return n ? sum / n : NAN;
}

void personality() {
  uint32_t now = millis();

  // bedtime: at night, after 3 minutes with no touches she curls up asleep
  struct tm tmNow;
  bool haveTime = getLocalTime(&tmNow, 0);
  night = haveTime && (tmNow.tm_hour >= SLEEP_HOUR || tmNow.tm_hour < WAKE_HOUR);
  // the window shows the real weather outside, and real day/night
  int wCode;
  bool wDay;
  float wTemp;
  if (netGetWeather(wCode, wDay, wTemp)) {
    catSetWeather(wCode, wTemp);
    catSetNight(!wDay);
  } else {
    catSetNight(haveTime && (tmNow.tm_hour >= 19 || tmNow.tm_hour < 7));
  }
  if (night && now - lastTouch > 180000) catSleep();
  setBacklight(night && catSleeping() ? 20 : 255);   // dim at night, not for daytime naps

  bool birthday = haveTime && tmNow.tm_mon + 1 == BIRTHDAY_MONTH && tmNow.tm_mday == BIRTHDAY_DAY;
  catConfetti(birthday);

  // every so often, a little news from the kitten
  if (screen != SCR_CAT || catSleeping() || now < nextNews) return;
  nextNews = now + random(15000, 35000);
  int r = random(100);

  // a stock hit one of his targets
  for (int i = 0; i < nSnap && r < 40; i++) {
    if (buyHit(snap[i]) || sellHit(snap[i])) {
      char msg[24];
      snprintf(msg, sizeof msg, "%s %s!", snap[i].symbol, buyHit(snap[i]) ? "buy zone" : "sell zone");
      catSay(msg, 2500);
      catHappy();
      return;
    }
  }
  if (birthday && r < 60) {
    catSay("HAPPY BIRTHDAY!", 2500);
    catHappy();
    return;
  }
  float mood = marketMood();
  if (!isnan(mood) && r < 30) {
    if (mood > 0.5f) { catSay("stonks up!"); catHappy(); }
    else if (mood < -0.5f) catSay("uh oh...");
  }
}

// ---------- screens ----------

void drawStockList() {
  drawHeader("MY STOCKS", GOLD);
  if (!nSnap) {
    printCentre("No stocks yet!", 50, 1, WHITE);
    printCentre("Hold this button", 70, 1, GREY);
    printCentre("to add some", 82, 1, GREY);
    return;
  }
  for (int i = 0; i < nSnap; i++) {
    const Stock &s = snap[i];
    int y = 15 + i * 14;
    bool buy = buyHit(s), sell = sellHit(s);
    bool big = s.valid && fabsf(s.change) >= BIG_MOVE_PERCENT;
    if ((buy || sell || big) && blinkOn()) {
      canvas.fillRect(0, y, 128, 13, buy ? GREEN_DK : sell ? RED_DK : s.change >= 0 ? GREEN_DK : RED_DK);
    }
    char buf[16];
    strlcpy(buf, s.symbol, 7);
    printAt(buf, 2, y + 3, 1, WHITE);
    if (!s.valid) {
      printRight(netOnline() ? "..." : "offline", 126, y + 3, 1, GREY);
      continue;
    }
    fmtPrice(buf, sizeof buf, s.price);
    printRight(buf, 86, y + 3, 1, WHITE);
    snprintf(buf, sizeof buf, "%c%.1f%%", s.change >= 0 ? 0x1E : 0x1F, fabsf(s.change));
    printRight(buf, 126, y + 3, 1, s.change >= 0 ? GREEN : RED);
  }
}

void drawOneStock() {
  if (stockIdx >= nSnap) { screen = SCR_LIST; return; }
  const Stock &s = snap[stockIdx];
  bool up = s.change >= 0;
  uint16_t col = up ? GREEN : RED;
  bool buy = buyHit(s), sell = sellHit(s);
  char buf[24];

  if (s.valid && fabsf(s.change) >= BIG_MOVE_PERCENT && blinkOn()) {
    for (int i = 0; i < 3; i++) canvas.drawRect(i, i, 128 - 2 * i, 128 - 2 * i, col);
  }

  strlcpy(buf, s.symbol, 7);
  printAt(buf, 6, 7, 3, WHITE);
  strlcpy(buf, s.name, 20);
  printAt(buf, 6, 34, 1, GREY);

  if (!s.valid) {
    printCentre(netOnline() ? "loading..." : "offline", 64, 1, GREY);
  } else {
    char p[16];
    fmtPrice(p, sizeof p, s.price);
    snprintf(buf, sizeof buf, "$%s", p);
    printCentre(buf, 50, strlen(buf) <= 10 ? 2 : 1, WHITE);

    snprintf(buf, sizeof buf, "%c %.2f%%", up ? 0x1E : 0x1F, fabsf(s.change));
    printCentre(buf, 72, 2, col);

    float prev = s.price / (1 + s.change / 100);
    snprintf(buf, sizeof buf, "%c$%.2f today", up ? '+' : '-', fabsf(s.price - prev));
    printCentre(buf, 92, 1, col);

    if (buy || sell) {
      uint16_t c = buy ? GREEN : RED;
      if (blinkOn()) canvas.fillRoundRect(6, 104, 116, 17, 5, c);
      printCentre(buy ? "BUY TARGET HIT" : "SELL TARGET HIT", 109, 1, blinkOn() ? BG : c);
    } else if (s.buyBelow > 0 || s.sellAbove > 0) {
      String t;
      if (s.buyBelow > 0) t += "buy<" + String(s.buyBelow, 0) + " ";
      if (s.sellAbove > 0) t += "sell>" + String(s.sellAbove, 0);
      printCentre(t.c_str(), 109, 1, GREY);
    }
  }

  // page dots
  for (int i = 0; i < nSnap; i++) {
    canvas.fillCircle(64 - (nSnap - 1) * 4 + i * 8, 125, 2, i == stockIdx ? WHITE : GREY);
  }
}

void drawVibes() {
  drawHeader("GOOD VIBES", GOLD);
  const Quote &q = QUOTES[quoteIdx];
  String lines[12];
  int size = 2, lineH = 18;
  int n = wrapText(q.text, 10, lines, 12);
  if (n > 5) {
    size = 1;
    lineH = 10;
    n = wrapText(q.text, 21, lines, 10);
  }
  int bottom = q.author[0] ? 112 : 126;
  int y = 15 + (bottom - 15 - n * lineH) / 2;
  for (int i = 0; i < n; i++) printCentre(lines[i].c_str(), y + i * lineH, size, WHITE);
  if (q.author[0]) {
    String a = String("- ") + q.author;
    printRight(a.substring(0, 21).c_str(), 125, 117, 1, GOLD);
  }
}

// The Easter egg: a big beating heart, hearts raining down, and a secret message
void drawSecret() {
  static const char *const HEART[] = { ".KK.KK.", "KPPKPPK", "KPPPPPK", ".KPPPK.", "..KPK..", "...K..." };
  const uint16_t PINK = rgb(255, 110, 160), DARK = rgb(150, 20, 70);
  float t = (millis() - secretAt) / 1000.0f;

  canvas.fillScreen(rgb(60, 15, 55));
  for (int i = 0; i < 12; i++) {   // falling hearts
    int hx = (i * 37 + 11) % 120;
    int hy = (int)(t * (25 + i * 3) + i * 29) % 150 - 12;
    for (int r = 0; r < 6; r++)
      for (int col = 0; col < 7; col++)
        if (HEART[r][col] == 'P') canvas.fillRect(hx + col, hy + r, 1, 1, DARK);
  }

  int s = 6 + (sinf(t * 6) > 0.6f ? 1 : 0);   // beating heart
  int hx = 64 - 7 * s / 2, hy = 10;
  for (int r = 0; r < 6; r++)
    for (int col = 0; col < 7; col++) {
      char ch = HEART[r][col];
      if (ch != '.') canvas.fillRect(hx + col * s, hy + r * s, s, s, ch == 'K' ? DARK : PINK);
    }

  String lines[6];
  int n = wrapText(SECRET_MESSAGE, 18, lines, 6);
  for (int i = 0; i < n; i++) printCentre(lines[i].c_str(), 62 + i * 11, 1, WHITE);
  if (blinkOn()) printCentre("* SECRET FOUND *", 118, 1, GOLD);
}

void drawPhoneScreen() {
  drawHeader("PHONE SETUP", CYAN);
  if (netOnline()) {
    printCentre("Pick your stocks!", 22, 1, WHITE);
    printCentre("On your phone open:", 40, 1, GREY);
    printCentre("buddy.local", 56, 1, CYAN);
    printCentre("or", 70, 1, GREY);
    printCentre(netAddress().c_str(), 84, 1, CYAN);
    printCentre("(same WiFi as me)", 104, 1, GREY);
  } else {
    printCentre("I'm not on WiFi :(", 30, 1, WHITE);
    printCentre("Tap TICKLE to set", 56, 1, CYAN);
    printCentre("up WiFi from", 68, 1, CYAN);
    printCentre("your phone", 80, 1, CYAN);
  }
}

// ---------- WiFi setup screen ----------

// QR code that joins the DesktopBuddy WiFi when scanned with a phone camera
uint8_t qrSize = 0;
bool qrDots[41][41];

void grabQr(esp_qrcode_handle_t q) {
  qrSize = min(esp_qrcode_get_size(q), 41);
  for (int y = 0; y < qrSize; y++)
    for (int x = 0; x < qrSize; x++) qrDots[y][x] = esp_qrcode_get_module(q, x, y);
}

void makeWifiQr() {
  esp_qrcode_config_t cfg = {};
  cfg.display_func = grabQr;
  cfg.max_qrcode_version = 5;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;
  esp_qrcode_generate(&cfg, "WIFI:T:nopass;S:DesktopBuddy;;");
}

void showWifiSetup(const char *status) {
  canvas.fillScreen(BG);
  bool good = !strcmp(status, "Pick WiFi on phone") || !strcmp(status, "Joining your WiFi...");
  bool bad = !strcmp(status, "Wrong password?");
  drawHeader(status, good ? GREEN : bad ? RED : CYAN);

  if (!qrSize) makeWifiQr();
  int scale = qrSize ? min(3, 78 / qrSize) : 0;
  int side = qrSize * scale;
  canvas.fillRect(2, 15, side + 6, side + 6, WHITE);   // white border so cameras can read it
  for (int y = 0; y < qrSize; y++)
    for (int x = 0; x < qrSize; x++)
      if (qrDots[y][x]) canvas.fillRect(5 + x * scale, 18 + y * scale, scale, scale, BG);

  int tx = side + 12;
  printAt("Scan", tx, 22, 1, WHITE);
  printAt("me to", tx, 32, 1, WHITE);
  printAt("join", tx, 42, 1, WHITE);
  printAt("my", tx, 52, 1, WHITE);
  printAt("WiFi", tx, 62, 1, WHITE);

  printAt("or join DesktopBuddy", 4, side + 24, 1, GREY);
  printAt("then open", 4, side + 34, 1, GREY);
  printAt("192.168.4.1", 4, side + 44, 1, CYAN);
  pushCanvas();
}

// Called constantly during WiFi setup - redraws, and tickle cancels
bool setupTick(const char *status) {
  static uint32_t lastDraw = 0;
  bool cancel = readPad(pads[PAD_TICKLE]);
  readPad(pads[PAD_STOCKS]);
  readPad(pads[PAD_VIBES]);
  if (millis() - lastDraw > 150) {
    lastDraw = millis();
    showWifiSetup(status);
  }
  return !cancel;
}

// ---------- main ----------

void setup() {
  Serial.begin(115200);

  tft.initR(INITR_144GREENTAB);
  tft.setRotation(SCREEN_ROTATION);
  tft.setSPISpeed(24000000);
  canvas.cp437(true);
#if TFT_LITE >= 0
  ledcAttach(TFT_LITE, 5000, 8);
  ledcWrite(TFT_LITE, 255);
#endif
  randomSeed(esp_random());

  catSay("connecting...", 60000);
  catUpdate();
  catDraw(canvas);
  pushCanvas();

  netBegin();
  catSay(netOnline() ? "hi! I'm online!" : "hi! (no WiFi)", 2500);
  catHappy();

  calibrateTouch();
  lastTouch = millis();
  Serial.println("Desktop Buddy ready!  (t = live touch readings, c = re-calibrate)");
}

void loop() {
  uint32_t frameStart = millis();

  netLoop();
  handleSerial();
  nSnap = netGetStocks(snap);
  handleButtons();
  personality();
  catUpdate();

  canvas.fillScreen(BG);
  switch (screen) {
    case SCR_CAT:   catDraw(canvas); break;
    case SCR_LIST:  drawStockList(); break;
    case SCR_STOCK: drawOneStock(); break;
    case SCR_VIBES: drawVibes(); break;
    case SCR_PHONE: drawPhoneScreen(); break;
    case SCR_SECRET: drawSecret(); break;
  }
  pushCanvas();

  // about 30 frames a second
  int wait = 33 - (int)(millis() - frameStart);
  if (wait > 0) delay(wait);
}
