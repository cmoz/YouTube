/**
 * Velostat Button with ESP32-C3 and OLED
 * This example demonstrates how to use a Velostat button with an ESP32-C3
 * and display messages on an OLED screen using the Adafruit SSD1306 library.
 * The button with velostat allows a user to tap or hold to trigger actions,
 * and the OLED displays messages based on the button state.
 * 
 * There is also an LED strip that can be controlled based on the button state.
 * 
 * This board was mounted on a handbag. 
 * Date: 2025-05-01
 * Author: @cmoz
 * Modified: 2025-06-18 - Fixed rainbow animation to continuously loop
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>

// Adjust these if your OLED is a different size
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// If your OLED has no RESET pin, set to -1
#define OLED_RESET    -1

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define your I2C pins (adjust if needed for your board)
#define I2C_SDA 5 //5
#define I2C_SCL 4 //4

#define LED_PIN     3
#define NUM_LEDS    75
#define BRIGHTNESS  128  // Half of max (0-255)
#define LED_TYPE    SK6812 //WS2812B
#define COLOR_ORDER RGBW    //GRB

CRGB leds[NUM_LEDS];
#define FILL_TIME 5000  // Time in ms to fill all LEDs

int myPin = 1; // GPIO2 (pin 8 on DevKitM-1)
bool touching = false;
unsigned long touchStart = 0;

String message = "Hello! ESP32-C3 OLED";
void displayMessage(String message);

void setup() {
  Serial.begin(115200);
  delay(1000);
  // Initialize I2C with custom pins for ESP32-C3
  Wire.begin(I2C_SDA, I2C_SCL);
  //Wire.begin(); // 

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C check your I2C address
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // Don't proceed, loop forever
  }
  Serial.println(F("SSD1306 allocation successful"));
  //display.display(); // Show initial splash screen
  delay(2000); // Pause for 2 seconds
  pinMode(myPin, INPUT_PULLDOWN); // Prevents random fluctuations
  // Set up the display
  Serial.println("OLED Init success");
  display.clearDisplay();
  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Hello!");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("ESP32-C3 OLED");
  display.display(); // Actually display all of the above

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("LED setup complete."); // Debug message
  delay(500); // Wait for a second before starting the loop
}

void loop() {
  static uint8_t startIndex = 0;
  static uint8_t rainbowOffset = 0;
  startIndex++; // Slightly change starting point for rainbow

  // Read the analog value from the touch pin
  // The value will be between 0 and 4095  
  int sensorValue = analogRead(myPin);
  
  // Debug first!
  Serial.print(sensorValue);
  Serial.print(" - ");
  
  if (sensorValue > 1800) { // Adjust after seeing raw values 1500
    if (!touching) touchStart = millis();
    touching = true;
    
    unsigned long holdTime = millis() - touchStart;
    float progress = (float)holdTime / FILL_TIME;
    progress = constrain(progress, 0.0, 1.0);
    float ledsToLight = progress * NUM_LEDS;

    if (progress < 1.0) {
      // Fill LEDs progressively during the first 3 seconds
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < (int)ledsToLight) {
          leds[i] = CHSV((i * 255) / NUM_LEDS, 255, BRIGHTNESS);
        } else if (i == (int)ledsToLight && ledsToLight < NUM_LEDS) {
          // Partially light the next LED based on fractional progress
          uint8_t partialBright = (ledsToLight - (int)ledsToLight) * BRIGHTNESS;
          leds[i] = CHSV((i * 255) / NUM_LEDS, 255, partialBright);
        } else {
          leds[i] = CRGB::Black;
        }
      }
    } else {
      // After 3 seconds, continuously animate the rainbow pattern
      // Increment the rainbow offset for continuous animation
      rainbowOffset += 2; // Speed of rainbow animation (higher = faster)
      
      // Apply the rainbow pattern to all LEDs
      for (int i = 0; i < NUM_LEDS; i++) {
        // The modulo 256 ensures the hue value stays within 0-255 range
        leds[i] = CHSV((rainbowOffset + i * 255 / NUM_LEDS) % 256, 255, BRIGHTNESS);
      }
    }

    FastLED.show();
    
    // OLED display logic
    if (holdTime < 500) {
      displayMessage("Tap\n...or keep holding");
    } else if (holdTime < 3000) {
      displayMessage("Holding");
    } else if (holdTime < 8000) {
      displayMessage("...Still holding!");
    } else {
      displayMessage("Phew!\n\nThat was a long hold!");
    }
  } else { // Button released
    touching = false;
    // Turn off all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    displayMessage("Tap for light");
  }
  delay(20);
}
  
void displayMessage(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("   *Tappy Rainbow*");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 18);
  display.println(message);
  display.display();
  delay(50); // Reduced from 500ms to make the rainbow animation smoother
}
