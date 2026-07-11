#include <Arduino.h>
#include <WiFi.h>
#include <GxEPD2_BW.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#define LED_PIN    8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define WAKE_BUTTON 0
#define INFO_BUTTON 1

#define EPD_CS   9
#define EPD_DC   10
#define EPD_RST  11
#define EPD_BUSY 12
GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> epaper(GxEPD2_213_B72(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

const char* ssid = "Camp Clavie";
const char* password = "45campClavie";

void displayToDos();
void showStartupAnimation();
void blinkError();
void animateCMOZ(void *pvParameters);

void setup() {
  Serial.begin(115200);
  delay(500); // let serial initialize
  pinMode(WAKE_BUTTON, INPUT_PULLUP);
  pinMode(INFO_BUTTON, INPUT_PULLUP);

  matrix.begin();
  matrix.clear();
  matrix.show();

  epaper.init();

  // Wi-Fi connection with timeout
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(250); // short delay
    yield();    // let watchdog breathe
  }

  if (WiFi.status() == WL_CONNECTED) {
    showStartupAnimation();
    displayToDos();
  } else {
    Serial.println("Wi-Fi failed to connect.");
    blinkError();
  }

  // freeRTOS
  /*
    a real-time operating system that lets you run multiple tasks 
    (like mini-programs) at the same time. Instead of everything 
    happening in , you can split your logic into independent tasks 
    that run concurrently.
  */
  xTaskCreate(
    animateCMOZ,       // Task function
    "LED Animation",   // Name
    2048,              // Stack size
    NULL,              // Parameters
    1,                 // Priority
    NULL               // Task handle
  );

}

void loop() {
  if (digitalRead(WAKE_BUTTON) == LOW) {
    displayToDos();
  }

  if (digitalRead(INFO_BUTTON) == LOW) {
    matrix.clear();
    matrix.setPixelColor(0, matrix.Color(0, 0, 255)); // Blue flash for info
    matrix.show();
    delay(1000);
  }

  delay(100);
}

void animateCMOZ(void *pvParameters) {
  while (true) {
    showStartupAnimation(); // your animation function
    vTaskDelay(10000 / portTICK_PERIOD_MS); // wait 10 seconds
  }
}


void displayToDos() {
  epaper.setRotation(1);
  epaper.setTextColor(GxEPD_BLACK);
  epaper.setFont(&FreeMonoBold9pt7b);
  epaper.setCursor(10, 30);
  epaper.println("1. Call Mom");
  epaper.setCursor(10, 60);
  epaper.println("2. Finish prototype");
  epaper.setCursor(10, 90);
  epaper.println("3. Email Christine");
  epaper.display();

  matrix.clear();
  matrix.setPixelColor(0, matrix.Color(255, 0, 0)); // Red for urgency
  matrix.show();
}

void showStartupAnimation() {
  // Simple pixel font for "CMOZ" on 8x8 matrix
  const uint8_t CMOZ[4][8] = {
    {0b00111100, 0b01000010, 0b01000000, 0b01000000, 0b01000000, 0b01000010, 0b00111100, 0b00000000}, // C
    {0b01111100, 0b01000100, 0b01000100, 0b01111100, 0b01000100, 0b01000100, 0b01111100, 0b00000000}, // M
    {0b01111100, 0b01000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01111110, 0b00000000}, // O
    {0b01111110, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01111110, 0b00000000}  // Z
  };

  for (int letter = 0; letter < 4; letter++) {
    matrix.clear();
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        if (CMOZ[letter][y] & (1 << (7 - x))) {
          int pixel = y * MATRIX_WIDTH + x;
          matrix.setPixelColor(pixel, matrix.Color(0, 255, 0)); // Green
        }
      }
    }
    matrix.show();
    delay(600);
  }

  matrix.clear();
  matrix.show();
}

void blinkError() {
  for (int i = 0; i < 5; i++) {
    matrix.fill(matrix.Color(255, 0, 0)); // red flash
    matrix.show();
    delay(200);
    matrix.clear();
    matrix.show();
    delay(200);
  }
}