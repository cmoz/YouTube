#include <Arduino.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_BW.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <qrcode.h>

Preferences prefs;

// Pin mapping for ESP32-C3 Mini
#define CS_PIN 8    // Chip Select
#define DC_PIN 7    // Data/Command
#define RST_PIN 9   // Reset
#define BUSY_PIN 2  // Busy
#define MOSI_PIN 4  // 6 SPI MOSI
#define SCK_PIN 10  // S2 10   scl   //S2 14    SPI Clock
#define WAKE_BUTTON_PIN 0

GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> *display = nullptr;
// Display class for 2.13" 250x122 SSD1680
// GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(CS_PIN, DC_PIN, RST_PIN, -1));
//GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));

const char *ssid = "Salty Dog Sync";
const char *password = "g!00scap";
// AP mode credentials
const char *ap_ssid = "Todo-Wrist";
const char *ap_password = "tinkertailor";

WebServer server(80);
String tasks[5];

int qrSize = 4;  // QR code size (1 to 4)
volatile bool sleepRequested = false;
bool completed[5] = {false, false, false, false, false};
bool isAPMode = false;  // Track current WiFi mode
unsigned long lastActivityMs = 0;
unsigned long bootMs = 0;
const unsigned long AUTO_SLEEP_TIMEOUT_MS = 1UL * 60UL * 1000UL;
const unsigned long NO_CONNECTION_SLEEP_MS = 1UL * 60UL * 1000UL;
const bool DEBUG_DISABLE_DEEP_SLEEP = true;
const bool DEBUG_SAFE_BOOT_ONLY = false;
const bool DEBUG_SKIP_DISPLAY_INIT = false;
const bool DEBUG_EARLY_INIT_ONLY = false;
const bool DEBUG_WIFI_ONLY = false;
const bool DEBUG_CORE_INIT_ONLY = false;
const bool DEBUG_SKIP_PREFS_SETUP = false;

void setupWiFi();
void handleRoot();
void handleSubmit();
void setupWebServer();
void updateDisplay();
bool tasksExist();
void showWelcomeMessage(String ip);
void drawQRCode(String ip);
void handleReset();
void IRAM_ATTR handleButtonPress();
void handleSleep();
void handleStyle();
void handleToggle();
String getCurrentIP();
void enterDeepSleep();
void touchActivity();
void handleNotFound();

RTC_DATA_ATTR uint32_t bootCount = 0;

const char* wakeCauseToString(esp_sleep_wakeup_cause_t cause) {
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0: return "EXT0";
    case ESP_SLEEP_WAKEUP_EXT1: return "EXT1";
    case ESP_SLEEP_WAKEUP_TIMER: return "TIMER";
    case ESP_SLEEP_WAKEUP_TOUCHPAD: return "TOUCH";
    case ESP_SLEEP_WAKEUP_ULP: return "ULP";
    case ESP_SLEEP_WAKEUP_GPIO: return "GPIO";
    case ESP_SLEEP_WAKEUP_UNDEFINED: return "POWER_ON_RESET";
    default: return "OTHER";
  }
}



void setup() {
  Serial.begin(115200);

  bootCount++;
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.printf("Boot #%lu, wake cause: %s (%d)\n", (unsigned long)bootCount, wakeCauseToString(cause), (int)cause);
  
  delay(1000);

  Serial.println("Booting...");
  Serial.printf("FW build: %s %s\n", __DATE__, __TIME__);
  Serial.println("=== Setup started ===");

  if (DEBUG_SAFE_BOOT_ONLY) {
    Serial.println("DEBUG: Safe boot only mode active. Skipping normal setup.");
    pinMode(LED_BUILTIN, OUTPUT);
    return;
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // on for C3 super mini 
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH); // off for C3 Super mini

  // pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
  // attachInterrupt(WAKE_BUTTON_PIN, handleButtonPress, FALLING);

  SPI.begin(SCK_PIN, -1, MOSI_PIN);
  SPI.setFrequency(1000000);  // 1 MHz

  display = new GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT>(
  GxEPD2_213_Z98c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN)
);

  if (DEBUG_EARLY_INIT_ONLY) {
    Serial.println("DEBUG: Early init only mode active after LED/button/SPI init.");
    return;
  }

  if (DEBUG_WIFI_ONLY) {
    Serial.println("DEBUG: WiFi-only mode start");
    setupWiFi();
    Serial.println("DEBUG: WiFi-only mode setup complete");
    return;
  }

  Serial.println("DEBUG: setupWiFi start");
  setupWiFi();
  Serial.println("DEBUG: setupWiFi done");

  Serial.println("DEBUG: setupWebServer start");
  setupWebServer();
  Serial.println("DEBUG: setupWebServer done");

  // Ensure optional style namespace exists.
  prefs.begin("style", false);
  prefs.end();

  // TROUBLE SHOOTING TO CLEAR TASKS
  // prefs.begin("tasks", false);
  // prefs.clear();
  // prefs.end();
  // END TROUBLE SHOOTING TO CLEAR TASKS

  // Load saved tasks and completion status
  if (DEBUG_SKIP_PREFS_SETUP) {
    Serial.println("DEBUG: Skipping prefs read in setup for troubleshooting.");
  } else {
    prefs.begin("tasks", false);
    for (int i = 0; i < 5; i++) {
      tasks[i] = prefs.getString(("task" + String(i)).c_str(), "");
      completed[i] = prefs.getBool(("done" + String(i)).c_str(), false);
    }
    prefs.end();
  }

  if (!tasksExist()) {
    String ip = getCurrentIP();
    String url = "http://" + ip;
    Serial.print("Open this URL from phone: ");
    Serial.println(url);
    if (DEBUG_SKIP_DISPLAY_INIT) {
      Serial.println("DEBUG: drawQRCode skipped for troubleshooting.");
    } else {
      Serial.println("DEBUG: drawQRCode start");
      drawQRCode(url);
      Serial.println("DEBUG: drawQRCode done");
    }
  } else {
    if (DEBUG_SKIP_DISPLAY_INIT) {
      Serial.println("DEBUG: updateDisplay skipped for troubleshooting.");
    } else {
      Serial.println("DEBUG: updateDisplay start");
      updateDisplay();  // show tasks
      Serial.println("DEBUG: updateDisplay done");
    }
  }

  if (DEBUG_CORE_INIT_ONLY) {
    Serial.println("DEBUG: Core init complete (WiFi + web + prefs), display skipped.");
    return;
  }
  // Setup deep sleep wake on IO0 short press (active low with pull-up).
  if (!DEBUG_DISABLE_DEEP_SLEEP) {
    // esp_deep_sleep_enable_gpio_wakeup(1ULL << GPIO_NUM_0, ESP_GPIO_WAKEUP_GPIO_LOW);
    Serial.println("DEBUG: GPIO wakeup on IO0 temporarily disabled for upload troubleshooting.");
  } else {
    Serial.println("DEBUG: Deep sleep wakeup source setup skipped.");
  }
  bootMs = millis();
  lastActivityMs = millis();

  Serial.println("Setup complete.");
}

void loop() {
  if (DEBUG_SAFE_BOOT_ONLY) {
    static bool led = false;
    led = !led;
    digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    Serial.println("DEBUG: safe loop alive");
    delay(1000);
    return;
  }

  if (DEBUG_EARLY_INIT_ONLY) {
    static bool led = false;
    led = !led;
    digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    Serial.println("DEBUG: early-init loop alive");
    delay(1000);
    return;
  }

  if (DEBUG_WIFI_ONLY) {
    static bool led = false;
    led = !led;
    digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    Serial.println("DEBUG: wifi-only loop alive");
    delay(1000);
    return;
  }

  if (DEBUG_CORE_INIT_ONLY) {
    static bool led = false;
    led = !led;
    digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
    server.handleClient();
    Serial.println("DEBUG: core-init loop alive");
    delay(1000);
    return;
  }

  server.handleClient();

  bool hasConnection = isAPMode ? (WiFi.softAPgetStationNum() > 0) : (WiFi.status() == WL_CONNECTED);

  if (sleepRequested && (millis() - bootMs > 1500)) {
    sleepRequested = false;
    Serial.println("Sleep requested by IO0 button.");
    enterDeepSleep();
  }

  if (!hasConnection && !isAPMode &&  (millis() - bootMs > NO_CONNECTION_SLEEP_MS)) {
    Serial.println("No connection detected for 1 minute.");
    enterDeepSleep();
  }

  if (millis() - lastActivityMs > AUTO_SLEEP_TIMEOUT_MS) {
    Serial.println("Auto sleep timeout reached (1 minute inactivity).");
    enterDeepSleep();
  }

  delay(20);
}

void setupWiFi() {
  Serial.println("Attempting to connect to home WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Try to connect for 15 seconds
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to home WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isAPMode = false;
  } else {
    Serial.println();
    Serial.println("Failed to connect to home WiFi. Starting AP mode...");

    // Start AP mode
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(ap_ssid, ap_password);

    if (apStarted) {
      Serial.println("AP mode started successfully!");
      Serial.print("AP SSID: ");
      Serial.println(ap_ssid);
      Serial.print("AP Password: ");
      Serial.println(ap_password);
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
      isAPMode = true;
    } else {
      Serial.println("Failed to start AP mode!");
    }
  }
}

String getCurrentIP() {
  if (isAPMode) {
    return WiFi.softAPIP().toString();
  } else {
    return WiFi.localIP().toString();
  }
}

void handleRoot() {
  touchActivity();

  prefs.begin("style", true);
  String bg = prefs.getString("bg", "white");
  String text = prefs.getString("text", "black");
  prefs.end();

  String statusColor = isAPMode ? "#fa9d45ff" : "#4CAF50";
  String statusText = isAPMode
    ? "AP Mode - Connect to: " + String(ap_ssid)
    : "Connected to: " + String(ssid);

  // Build the dynamic task rows as one small string
  String taskRows = "";
  for (int i = 0; i < 5; i++) {
    taskRows += "Task " + String(i + 1) + ": <input type='text' name='task" +
                String(i) + "' value='" + tasks[i] + "' maxlength='15'><br>";
  }

  // Build the toggle dropdown as one small string
  String toggleOptions = "";
  for (int i = 0; i < 5; i++) {
    String label = tasks[i].length() == 0 ? "(empty)" : tasks[i];
    String check = completed[i] ? " checkmark" : "";
    toggleOptions += "<option value='" + String(i) + "'>" + label + check + "</option>";
  }

  // Send in chunks — no single giant string ever exists in RAM
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(F("<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"));
  server.sendContent(F("<style>"));
  server.sendContent(F("body { background-color: #F0EEE9; color: #000000; font-family: Arial, sans-serif; padding: 20px; }"));
  server.sendContent(F("h1 { font-size: 28px; margin-bottom: 10px; }"));
  server.sendContent(F("p { font-size: 16px; margin-bottom: 20px; }"));
  server.sendContent(".status { background-color: " + statusColor + "; color: white; padding: 8px; border-radius: 4px; margin-bottom: 15px; }");
  server.sendContent(F(".reset { background-color: #f790e4ff; }"));
  server.sendContent(F(".update { background-color: #7d7d7dff; }"));
  server.sendContent(F("input[type='text'] { width: 15ch; padding: 10px; font-size: 16px; margin-bottom: 10px; }"));
  server.sendContent(F("input[type='submit'] { padding: 10px 20px; font-size: 16px; margin: 10px 5px 10px 0; }"));
  server.sendContent(F("select { padding: 8px; font-size: 16px; margin-bottom: 10px; }"));
  server.sendContent(F("form.inline { display: inline-block; margin-right: 10px; }"));
  server.sendContent(F("</style></head><body>"));

  server.sendContent("<div class='status'>" + statusText + "</div>");

  server.sendContent(F("<h1>Top Five for Today</h1>"));
  server.sendContent(F("<p>Focus on what matters most. One step at a time.</p>"));

  server.sendContent(F("<form action='/submit' method='POST'>"));
  server.sendContent(taskRows);
  server.sendContent(F("<input type='submit' class='update' value='Update Tasks'></form>"));

  server.sendContent(F("<div style='display: flex; gap: 10px; margin-top: 20px;'>"));
  server.sendContent(F("<form class='inline' action='/reset' method='POST'><input type='submit' class='reset' value='Reset Tasks'></form>"));
  server.sendContent(F("<form class='inline' action='/sleep' method='POST'><input type='submit' class='update' value='Sleep Now'></form>"));
  server.sendContent(F("</div>"));

  server.sendContent(F("<form action='/style' method='POST' style='margin-top: 30px;'>"));
  server.sendContent(F("Background Color: <select name='bg'>"));
  server.sendContent("<option value='white'" + String(bg == "white" ? " selected" : "") + ">White</option>");
  server.sendContent("<option value='black'" + String(bg == "black" ? " selected" : "") + ">Black</option>");
  server.sendContent("<option value='red'"   + String(bg == "red"   ? " selected" : "") + ">Red</option>");
  server.sendContent(F("</select><br>"));
  server.sendContent(F("Text Color: <select name='text'>"));
  server.sendContent("<option value='white'" + String(text == "white" ? " selected" : "") + ">White</option>");
  server.sendContent("<option value='black'" + String(text == "black" ? " selected" : "") + ">Black</option>");
  server.sendContent("<option value='red'"   + String(text == "red"   ? " selected" : "") + ">Red</option>");
  server.sendContent(F("</select><br>"));
  server.sendContent(F("<input type='submit' class='update' value='Update Style'></form>"));

  server.sendContent(F("<form action='/toggle' method='POST' style='margin-top: 30px;'>"));
  server.sendContent(F("Mark task as done: <select name='task'>"));
  server.sendContent(toggleOptions);
  server.sendContent(F("</select>"));
  server.sendContent(F("<input type='submit' class='update' value='Toggle Completion'></form>"));

  server.sendContent("<p style='margin-top: 30px; font-size: 12px; color: #ffffffff;'>Current IP: " + getCurrentIP() + "</p>");
  server.sendContent(F("</body></html>"));

  // Signal end of chunked response
  server.sendContent("");
}

void handleSubmit() {
  touchActivity();
  Serial.println("handleSubmit triggered");

  prefs.begin("tasks", false);
  for (int i = 0; i < 5; i++) {
    String newTask = server.arg("task" + String(i));
    String oldTask = prefs.getString(("task" + String(i)).c_str(), "");

    tasks[i] = newTask;
    prefs.putString(("task" + String(i)).c_str(), newTask);

    // Only reset completion if the task text changed
    if (newTask != oldTask) {
      prefs.putBool(("done" + String(i)).c_str(), false);
      completed[i] = false;
    }
  }
  prefs.end();

  updateDisplay();

  server.send(200, "text/html", R"rawliteral(
  <html>
    <head>
      <meta http-equiv="refresh" content="2;url=/" />
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body {
          background-color: #F0EEE9;
          color: #000000;
          font-family: Arial, sans-serif;
          text-align: center;
          padding-top: 50px;
        }
        h1 {
          font-size: 28px;
          margin-bottom: 10px;
        }
        p {
          font-size: 16px;
          margin-bottom: 20px;
        }
        .checkmark {
          width: 56px;
          height: 56px;
          border-radius: 50%;
          display: inline-block;
          border: 4px solid #4caf50;
          position: relative;
          animation: pop 0.3s ease-out forwards;
          margin-bottom: 20px;
        }
        .checkmark::after {
          content: '';
          position: absolute;
          left: 14px;
          top: 18px;
          width: 14px;
          height: 28px;
          border-right: 4px solid #4caf50;
          border-bottom: 4px solid #4caf50;
          transform: rotate(45deg);
          opacity: 0;
          animation: draw 0.5s ease-out 0.3s forwards;
        }
        @keyframes pop {
          0% { transform: scale(0); opacity: 0; }
          100% { transform: scale(1); opacity: 1; }
        }
        @keyframes draw {
          0% { opacity: 0; transform: rotate(45deg) scale(0); }
          100% { opacity: 1; transform: rotate(45deg) scale(1); }
        }
      </style>
    </head>
    <body>
      <div class="checkmark"></div>
      <h1>Tasks Updated!</h1>
      <p>Redirecting back to the task page...</p>
    </body>
  </html>
  )rawliteral");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/reset", handleReset);
  server.on("/sleep", handleSleep);
  server.on("/style", HTTP_POST, handleStyle);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.onNotFound(handleNotFound);
  server.begin();
}

void handleNotFound() {
  // Browsers commonly request /favicon.ico; return no content to keep logs clean.
  if (server.uri() == "/favicon.ico") {
    server.send(204, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void updateDisplay() {
  String ipAddress = getCurrentIP();

  prefs.begin("style", true);
  String bg = prefs.getString("bg", "white");
  String text = prefs.getString("text", "black");
  prefs.end();

  // Map string values to GxEPD2 color constants
  uint16_t bgColor = GxEPD_WHITE;
  uint16_t textColor = GxEPD_BLACK;

  if (bg == "black")
    bgColor = GxEPD_BLACK;
  else if (bg == "red")
    bgColor = GxEPD_RED;

  if (text == "white")
    textColor = GxEPD_WHITE;
  else if (text == "red")
    textColor = GxEPD_RED;
  else if (text == "black")
    textColor = GxEPD_BLACK;

  display->init();
  display->setRotation(1);
  display->setFont(&FreeMonoBold9pt7b);
  display->setFullWindow();

  display->firstPage();
  do {
    display->fillScreen(bgColor);
    display->setTextColor(textColor);

    for (int i = 0; i < 5; i++) {
      int x = 2;
      int y = 15 + i * 20;

      display->setCursor(x, y);
      display->print(i + 1);
      display->print('.');
      display->print(completed[i] ? "[X]" : "[ ]");
      display->print(tasks[i]);

      if (completed[i]) {
        // Draw strikethrough line
        int lineStartX = x + 40;  // Rough offset after "1.[X]"
        int lineEndX = display->width() - 5;
        int lineY = y - 6;  // Slightly above baseline
        display->drawLine(lineStartX, lineY, lineEndX, lineY, textColor);
      }
    }

    // Display connection status and IP
    display->setFont(NULL);
    display->setCursor(2, 102);
    display->print(isAPMode ? "AP:" : "IP:");
    display->setCursor(22, 102);
    display->print(ipAddress);

    display->setCursor(2, 112);
    display->print(isAPMode ? String("SSID:") + ap_ssid : "Connected to WiFi");

  } while (display->nextPage());

  display->hibernate();
}

bool tasksExist() {
  for (int i = 0; i < 5; i++) {
    if (tasks[i].length() > 0) return true;
  }
  return false;
}

void showWelcomeMessage(String ip) {
  display->init();
  display->setRotation(1);
  display->setFont(&FreeMonoBold12pt7b);
  display->setFullWindow();

  display->firstPage();
  do {
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setCursor(10, 30);
    display->print("Welcome!");

    display->setCursor(10, 60);
    display->print("Go to:");

    display->setCursor(10, 90);
    display->print(ip);

    display->setCursor(10, 120);
    display->print("to enter tasks.");
  } while (display->nextPage());

  display->hibernate();
}

void drawQRCode(String ip) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(4)];
  qrcode_initText(&qrcode, qrcodeData, 4, ECC_LOW, ip.c_str());

  display->init();
  display->setRotation(1);
  display->setFullWindow();
  display->firstPage();
  do {
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);

    int maxScaleByHeight = (display->height() - 6) / qrcode.size;  // keep room for 2px border + margins
    if (maxScaleByHeight < 1) {
      maxScaleByHeight = 1;
    }
    int scale = min(qrSize, maxScaleByHeight);
    int qrSizePx = qrcode.size * scale;
    int xOffset = display->width() - qrSizePx - 2;
    int yOffset = (display->height() - qrSizePx) / 2;

    for (int y = 0; y < qrcode.size; y++) {
      for (int x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          display->fillRect(xOffset + x * scale, yOffset + y * scale, scale, scale, GxEPD_BLACK);
        }
      }
    }

    // Keep the QR on the right and all helper text on the left panel.
    display->drawRect(xOffset - 2, yOffset - 2, qrSizePx + 4, qrSizePx + 4, GxEPD_BLACK);

    display->setFont(NULL);
    display->setCursor(8, 48);
    display->print("Welcome to");
    display->setCursor(8, 60);
    display->print("cmoz.fashion");

    if (isAPMode) {
      display->setCursor(8, 74);
      display->print("WiFi");
      display->setCursor(8, 85);
      display->print(ap_ssid);
      display->setCursor(8, 96);
      display->print("PW");
      display->setCursor(8, 107);
      display->print(ap_password);
    } else {
      display->setCursor(8, 76);
      display->print("WiFi");
      display->setCursor(8, 88);
      display->print(ssid);
      display->setCursor(8, 106);
      display->print("Scan QR to open");
    }

  } while (display->nextPage());

  display->hibernate();
}

void handleReset() {
  touchActivity();
  prefs.begin("tasks", false);
  for (int i = 0; i < 5; i++) {
    String taskKey = "task" + String(i);
    String doneKey = "done" + String(i);
    prefs.remove(taskKey.c_str());
    prefs.remove(doneKey.c_str());
    tasks[i] = "";
    completed[i] = false;
  }
  prefs.end();

  String ip = getCurrentIP();
  drawQRCode("http://" + ip);

  server.send(200, "text/html", R"rawliteral(
  <html>
    <head>
      <meta http-equiv="refresh" content="2;url=/" />
      <style>
        body {
          font-family: Arial, sans-serif;
          text-align: center;
          padding-top: 50px;
          background-color: #F0EEE9;
          color: #000000;
        }
        h1 {
          font-size: 28px;
          margin-bottom: 10px;
        }
        p {
          font-size: 16px;
          margin-bottom: 20px;
        }
        .progress-container {
          width: 80%;
          background-color: #d7d7d7ff;
          border-radius: 20px;
          margin: 20px auto;
          height: 20px;
          overflow: hidden;
        }
        .progress-bar {
          height: 100%;
          width: 0%;
          background-color: #4caf50;
          animation: fill 2s linear forwards;
        }
        @keyframes fill {
          from { width: 0%; }
          to { width: 100%; }
        }
      </style>
    </head>
    <body>
      <h1>Tasks cleared!</h1>
      <p>Redirecting back to the task page...</p>
      <div class="progress-container">
        <div class="progress-bar"></div>
      </div>
    </body>
  </html>
)rawliteral");
}

void IRAM_ATTR handleButtonPress() {
  sleepRequested = true;
}

void handleSleep() {
  touchActivity();
  server.send(200, "text/html",
              "<html><head><meta http-equiv='refresh' content='2;url=/' /><style>body { font-family: Arial, "
              "sans-serif; text-align: center; padding-top: 50px; background-color: #F0EEE9; color: #000000; } h1 { "
              "font-size: 28px; margin-bottom: 10px; } p { font-size: 16px; }</style></head><body><h1>Going to "
              "sleep...</h1><p>Press IO0 to wake up</p></body></html>");
  delay(500);
  enterDeepSleep();
}

void enterDeepSleep() {
  if (DEBUG_DISABLE_DEEP_SLEEP) {
    Serial.println("DEBUG: Deep sleep call blocked for troubleshooting.");
    return;
  }
  Serial.println("Entering deep sleep now. Wake source: IO0.");
  delay(100);
  WiFi.mode(WIFI_OFF);
  esp_deep_sleep_start();
}

void handleStyle() {
  touchActivity();
  String bg = server.arg("bg");
  String text = server.arg("text");

  prefs.begin("style", false);
  prefs.putString("bg", bg);
  prefs.putString("text", text);
  prefs.end();

  updateDisplay();

  server.send(200, "text/html",
              "<html><head><meta http-equiv='refresh' content='2;url=/' /><style>body { font-family: Arial, "
              "sans-serif; text-align: center; padding-top: 50px; background-color: #F0EEE9; color: #000000; } h1 { "
              "font-size: 28px; margin-bottom: 10px; } a { color: #ffffffff; text-decoration: none; font-weight: bold; "
              "} a:hover { text-decoration: underline; }</style></head><body><h1>Style Updated!</h1><p>Redirecting "
              "back to the task page...</p></body></html>");
}

void handleToggle() {
  touchActivity();
  int index = server.arg("task").toInt();
  if (index >= 0 && index < 5) {
    completed[index] = !completed[index];
    prefs.begin("tasks", false);
    prefs.putBool(("done" + String(index)).c_str(), completed[index]);
    prefs.end();
    updateDisplay();
  }
  server.send(200, "text/html",
              "<html><head><meta http-equiv='refresh' content='2;url=/' /><style>body { font-family: Arial, "
              "sans-serif; text-align: center; padding-top: 50px; background-color: #F0EEE9; color: #000000; } h1 { "
              "font-size: 28px; margin-bottom: 10px; } a { color: #ffffffff; text-decoration: none; font-weight: bold; "
              "} a:hover { text-decoration: underline; }</style></head><body><h1>Task toggled!</h1><p>Redirecting back "
              "to the task page...</p></body></html>");
}

void touchActivity() { lastActivityMs = millis(); }