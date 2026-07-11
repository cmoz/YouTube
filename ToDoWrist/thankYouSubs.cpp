#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <WiFi.h>


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

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("e-Paper Display Test");
  SPI.begin(SCK_PIN, -1, MOSI_PIN);
  display.init();
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE);

    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(25, 30);
    display.print("THANK YOU!");                                                                      

    display.setFont(&FreeMonoBold24pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(20, 68);
    display.print("10,000");

    display.setFont(&FreeMonoBold18pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(10, 98);
    display.print("Subscribers");
  } while (display.nextPage());

  display.hibernate();
}

void loop() {
  // Do nothing or put the ESP32 into deep sleep to save power
  delay(1000);
}
