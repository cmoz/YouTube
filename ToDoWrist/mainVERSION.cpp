#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <WiFi.h>
#include <WebServer.h>


// Pin mapping for ESP32-C3 Mini
#define CS_PIN    8  // Chip Select
#define DC_PIN    7  // Data/Command
#define RST_PIN   9  // Reset
#define BUSY_PIN  2  // Busy
#define MOSI_PIN  4  // 6 SPI MOSI 
#define SCK_PIN   10 // S2 10   scl   //S2 14    SPI Clock

// Display class for 2.13" 250x122 SSD1680
//GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(CS_PIN, DC_PIN, RST_PIN, -1));
GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));

const char* ssid = "XXXXXX";
const char* password = "XXXXXX"; // Replace with your actual password

WebServer server(80);
String tasks[5];

void setupWiFi();
void handleRoot();
void handleSubmit();
void setupWebServer();
void updateDisplay();
bool tasksExist();
void showWelcomeMessage(String ip);

void setup() {
  Serial.begin(115200);
  delay(500);
  SPI.begin(SCK_PIN, -1, MOSI_PIN);
  setupWiFi();
  setupWebServer();
  if (!tasksExist()) {
    String ip = WiFi.localIP().toString();
    showWelcomeMessage(ip);
    } else {
      updateDisplay(); // show tasks
    }
  esp_deep_sleep_start();
}

void loop() {
  // Do nothing or put the ESP32 into deep sleep to save power
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void handleRoot() {
  String html = "<html><body><form action='/submit' method='POST'>";
  for (int i = 0; i < 5; i++) {
    html += "Task " + String(i + 1) + ": <input name='task" + String(i) + "'><br>";
  }
  html += "<input type='submit'></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSubmit() {
  for (int i = 0; i < 5; i++) {
    tasks[i] = server.arg("task" + String(i));
  }
  server.send(200, "text/html", "<html><body><h1>Tasks Updated!</h1></body></html>");
  updateDisplay();
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();
}

void updateDisplay() {
  display.init();
  display.setRotation(1);
  display.setFont(&FreeMonoBold12pt7b);
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    for (int i = 0; i < 5; i++) {
      display.setCursor(10, 20 + i * 20);
      display.print(String(i + 1) + ". " + tasks[i]);
    }
  } while (display.nextPage());

  display.hibernate();
}

bool tasksExist() {
  for (int i = 0; i < 5; i++) {
    if (tasks[i].length() > 0) return true;
  }
  return false;
}

void showWelcomeMessage(String ip) {
  display.init();
  display.setRotation(1);
  display.setFont(&FreeMonoBold12pt7b);
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 30);
    display.print("Welcome!");

    display.setCursor(10, 60);
    display.print("Go to:");

    display.setCursor(10, 90);
    display.print(ip);

    display.setCursor(10, 120);
    display.print("to enter tasks.");
  } while (display.nextPage());

  display.hibernate();
}