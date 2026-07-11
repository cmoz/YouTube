/**
 * Airport Challenge!!
 *
 * An airport countdown timer using an ESP32-S2, two capacitive touch pads,
 * an OLED display, and a 4-pixel RGB LED strip.
 *
 * - Tap touch pads to set time in 1 and 10-minute increments.
 * - Hold a touch pad to start the countdown.
 * - Touch both pads simultaneously to cancel/reset.
 * - LEDs indicate the urgency of the remaining time.
 *
 * Date: 2025-06-17
 * Author: @cmoz
 * Modified: 2025-06-18 - Added idle animations and instructions.
 * Modified: 2025-06-18 - Corrected pin conflicts and logic to match observed hardware behavior.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>

// --- OLED Display Configuration ---
#define SDA_PIN 7 //41          // Default SDA pin for QTPy ESP32-S2   7
#define SCL_PIN 6 //40          // Default SCL pin for QTPy ESP32-S    6
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- LED Strip Configuration ---
#define LED_PIN A1 // A1          // Pin for WS2812B data
#define NUM_LEDS 4
#define BRIGHTNESS 96       // Adjusted brightness for clarity
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// --- Pin Definitions (Corrected to avoid conflicts) ---
#define TOUCH_PIN_1 2      // Touch pad for 1-minute increments (GPIO 3)
#define TOUCH_PIN_2 3      // Touch pad for 10-minute increments (GPIO 2)

// --- Constants for Timing and Thresholds ---
// *** CALIBRATE THESE VALUES! *** Use Serial Monitor to find the right threshold.
// A value HIGHER than the threshold now indicates a touch.
const int TOUCH_THRESHOLD_1 = 100000; // Threshold for TOUCH_PIN_1 (e.g., A2)
const int TOUCH_THRESHOLD_2 = 100000;  // Threshold for TOUCH_PIN_2 (e.g., A1)
const unsigned long HOLD_THRESHOLD = 1000;
const unsigned long MILLIS_IN_A_MINUTE = 60000UL;
const int MAX_MINUTES = 99;

// --- LED Animation Parameters ---
const int GREEN_THRESHOLD_MINS = 40;
const int AMBER_THRESHOLD_MINS = 20;
const int RED_THRESHOLD_MINS = 10;
const unsigned int FLASH_INTERVAL_MS = 500;
const int MINUTES_PER_LED_SETUP = 15;

// --- Power Management ---
const unsigned long INACTIVITY_TIMEOUT_MS = 30000;
const int DIM_BRIGHTNESS = 10;

// --- Global State Variables ---
int minutes = 0;
bool timerRunning = false;
unsigned long timerStartMillis = 0;
unsigned long timerDurationMillis = 0;
unsigned long lastInteractionTime = 0;

// --- Variables for Display State (to update only on change) ---
int lastDisplayedMinutes = -1;
long lastDisplayedSeconds = -1;
bool lastTimerState = false;

// --- Touch Input State Variables ---
bool lastTouch1State = false;
bool lastTouch2State = false;
unsigned long touch1StartTime = 0;
unsigned long touch2StartTime = 0;
bool touch1Held = false;
bool touch2Held = false;

// --- Forward Declarations ---
void updateDisplay();
void updateLEDs();
void cancelTimer();
void startTimer();
void handleInputs();
void checkPowerSaving();

void setup() {
  Serial.begin(115200);
  delay(1000); // Give time for Serial Monitor to open

  // For QTPy ESP32-S2, default I2C pins are used.
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1) delay(100);
  }

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  lastInteractionTime = millis();
  Serial.println("Airport Challenge Timer Initialized!");

  Serial.print("Touch 1 baseline: ");
  Serial.println(touchRead(TOUCH_PIN_1));
  Serial.print("Touch 2 baseline: ");
  Serial.println(touchRead(TOUCH_PIN_2));

}

void loop() {
  unsigned long currentMillis = millis();

  handleInputs();

  if (timerRunning) {
    unsigned long elapsed = currentMillis - timerStartMillis;
    if (elapsed >= timerDurationMillis) {
      // When timer completes, reset to idle state
      timerRunning = false;
      minutes = 0;
      timerDurationMillis = 0;
    }
  }

  updateDisplay();
  updateLEDs();
  checkPowerSaving();

  FastLED.show();
  delay(20);
}

/**
 * @brief Handles reading touch inputs and gestures using analogRead().
 */
void handleInputs() {
  unsigned long currentMillis = millis();
  
  // Reading analog values. A value HIGHER than the threshold means a touch.
  //bool reading1 = analogRead(TOUCH_PIN_1) > TOUCH_THRESHOLD_1;
  //bool reading2 = analogRead(TOUCH_PIN_2) > TOUCH_THRESHOLD_2;

  bool reading1 = touchRead(TOUCH_PIN_1) > TOUCH_THRESHOLD_1; 
  bool reading2 = touchRead(TOUCH_PIN_2) > TOUCH_THRESHOLD_2;


  // --- Cancel Feature: Touch both pads simultaneously ---
  if (reading1 && reading2) {
    cancelTimer();
    lastInteractionTime = currentMillis;
    // Wait for release to prevent multiple triggers
    while (analogRead(TOUCH_PIN_1) > TOUCH_THRESHOLD_1 || analogRead(TOUCH_PIN_2) > TOUCH_THRESHOLD_2) {
      delay(50);
    }
    return;
  }

  // --- Handle Touch Pad 1 (1 minute) ---
  if (reading1 != lastTouch1State) {
    lastTouch1State = reading1;
    if (reading1) { // Touched
      touch1StartTime = currentMillis;
      touch1Held = false;
    } else { // Released
      if (!touch1Held && !timerRunning) {
        minutes += 1;
        if (minutes > MAX_MINUTES) minutes = MAX_MINUTES;
        lastInteractionTime = currentMillis;
      }
    }
  }

  // --- Handle Touch Pad 2 (10 minutes) ---
  if (reading2 != lastTouch2State) {
    lastTouch2State = reading2;
    if (reading2) { // Touched
      touch2StartTime = currentMillis;
      touch2Held = false;
    } else { // Released
      if (!touch2Held && !timerRunning) {
        minutes += 10;
        if (minutes > MAX_MINUTES) minutes = MAX_MINUTES;
        lastInteractionTime = currentMillis;
      }
    }
  }

  // --- Hold Gesture Detection ---
  if (reading1 && !touch1Held && (currentMillis - touch1StartTime > HOLD_THRESHOLD)) {
    touch1Held = true;
    startTimer();
    lastInteractionTime = currentMillis;
  }
  if (reading2 && !touch2Held && (currentMillis - touch2StartTime > HOLD_THRESHOLD)) {
    touch2Held = true;
    startTimer();
    lastInteractionTime = currentMillis;
  }
}

void startTimer() {
  if (minutes > 0 && !timerRunning) {
    timerRunning = true;
    timerStartMillis = millis();
    timerDurationMillis = (unsigned long)minutes * MILLIS_IN_A_MINUTE;
  }
}

void cancelTimer() {
  timerRunning = false;
  minutes = 0;
  timerDurationMillis = 0; // Ensure this is also reset
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(500);
}

void updateDisplay() {
  long remainingSeconds = 0;
  if (timerRunning) {
    unsigned long elapsed = millis() - timerStartMillis;
    remainingSeconds = (timerDurationMillis - elapsed) / 1000;
    if (remainingSeconds < 0) remainingSeconds = 0;
  }

  // Only update the display if something has changed
  if (minutes == lastDisplayedMinutes && timerRunning == lastTimerState && remainingSeconds == lastDisplayedSeconds) {
    return;
  }

  display.clearDisplay();
  
  if (timerRunning) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Time Remaining:");
    display.setTextSize(2);
    display.setCursor(20, 20);
    int displayMins = remainingSeconds / 60;
    int displaySecs = remainingSeconds % 60;
    display.printf("%02d:%02d", displayMins, displaySecs);
  } else if (minutes > 0) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Set Timer:");
    display.setTextSize(2);
    display.setCursor(35, 20);
    display.printf("%d mins", minutes);
    display.setTextSize(1);
    display.setCursor(15, 50);
    display.println("Hold to Start");
  } else {
    // Display instructions when in the idle state
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(15, 0);
    display.println("Set Timer");
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.println(" Tap L/R to add time");
    display.setCursor(0, 36);
    display.println(" Hold either to start");
    display.setCursor(0, 48);
    display.println(" Touch both to cancel");
  }
  display.display();

  lastDisplayedMinutes = minutes;
  lastTimerState = timerRunning;
  lastDisplayedSeconds = remainingSeconds;
}

void updateLEDs() {
  static uint8_t rainbowHue = 0;
  static bool flashState = false;

  EVERY_N_MILLISECONDS(FLASH_INTERVAL_MS) {
    flashState = !flashState;
  }

  if (timerRunning) {
    unsigned long elapsed = millis() - timerStartMillis;
    int remainingMinutes = (timerDurationMillis - elapsed) / MILLIS_IN_A_MINUTE;

    if (remainingMinutes > GREEN_THRESHOLD_MINS) {
      uint8_t pulse = beatsin8(15, BRIGHTNESS / 2, BRIGHTNESS);
      fill_solid(leds, NUM_LEDS, CHSV(96, 255, pulse));
    } else if (remainingMinutes > AMBER_THRESHOLD_MINS) {
      fill_solid(leds, NUM_LEDS, CRGB::Green);
    } else if (remainingMinutes > RED_THRESHOLD_MINS) {
      fill_solid(leds, NUM_LEDS, CRGB::Orange);
    } else {
      fill_solid(leds, NUM_LEDS, flashState ? CRGB::Red : CRGB::Black);
    }
  } else {
    // Timer is NOT running.
    if (minutes > 0) {
      // Show set time on LEDs (not running yet)
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      int ledsToLight = minutes / MINUTES_PER_LED_SETUP;
      if (ledsToLight > NUM_LEDS) ledsToLight = NUM_LEDS;
      for (int i = 0; i < ledsToLight; i++) {
        leds[i] = CRGB::Blue;
      }
    } else {
      // Idle state (minutes == 0), show a rainbow animation.
      fill_rainbow(leds, NUM_LEDS, rainbowHue++, 7);
    }
  }
}

void checkPowerSaving() {
  if (millis() - lastInteractionTime > INACTIVITY_TIMEOUT_MS) {
    FastLED.setBrightness(DIM_BRIGHTNESS);
  } else {
    FastLED.setBrightness(BRIGHTNESS);
  }
}