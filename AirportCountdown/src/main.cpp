/**
 * Airport Challenge!!
 *
 * An airport countdown timer using an ESP32-S2, two capacitive touch pads,
 * an OLED display, and a 4-pixel RGB LED strip.
 *
 * - Power on: each touch pad calibrates to its own resting value (hands off!)
 * - Tap the A2 pad to add 1 minute, the A3 pad to add 10 minutes
 *   (while you touch a pad, its half of the LED strip glows white)
 * - Hold either pad for a second to start the countdown
 * - Touch both pads together to cancel/reset
 * - LEDs indicate the urgency of the remaining time
 * - When time runs out the screen and LEDs flash "TIME'S UP!" - touch a pad to clear
 *
 * Serial Monitor (115200 baud):
 *   t = show/hide live touch readings (for tuning)
 *   c = re-calibrate the pads (hands off!)
 *
 * Date: 2025-06-17
 * Author: @cmoz
 * Modified: 2025-06-18 - Added idle animations and instructions.
 * Modified: 2025-06-18 - Corrected pin conflicts and logic to match observed hardware behavior.
 * Modified: 2026-10-30 - 2026 update:
 *     - Touch pads moved to A2/A3 (GPIO 2/3 aren't on the QT Py's pins)
 *     - Per-pad touch calibration instead of one fixed threshold
 *     - Touch feedback: a touched pad lights its half of the strip white
 *     - Reliable cancel: hold both pads 0.3s, then nothing counts until hands off
 *     - "TIME'S UP!" alert when the countdown ends
 *     - Live touch readings / re-calibrate from the Serial Monitor
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

// --- Pin Definitions ---
#define TOUCH_PIN_1 A2     // Touch pad for 1-minute increments (GPIO 9)
#define TOUCH_PIN_2 A3     // Touch pad for 10-minute increments (GPIO 8)

// --- Touch Tuning ---
// Each pad is calibrated at power-on, so these are relative to its own resting value.
const uint32_t TOUCH_PERCENT  = 20;    // touch = reading rises this % above the pad's resting value
const uint32_t TOUCH_MIN_JUMP = 2000;  // ...and by at least this much (stops noise counting as a touch)
const uint8_t  TOUCH_CONFIRM  = 2;     // readings in a row needed before a touch counts
const unsigned long CANCEL_HOLD_MS = 300;  // both pads touched this long = cancel

// --- Constants for Timing ---
const unsigned long HOLD_THRESHOLD = 1000;
const unsigned long MILLIS_IN_A_MINUTE = 60000UL;
const int MAX_MINUTES = 99;
const unsigned long TIMES_UP_MS = 30000;   // how long "TIME'S UP!" flashes if nobody touches a pad

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
bool timesUp = false;
unsigned long timesUpAt = 0;
bool flashState = false;

// --- Variables for Display State (to update only on change) ---
int lastDisplayedMinutes = -1;
long lastDisplayedSeconds = -1;
bool lastTimerState = false;
bool lastTimesUpFlash = false;

// --- Touch Input State ---
struct TouchPad {
  uint8_t pin;
  const char* name;
  int addMinutes;
  uint32_t baseline;       // resting (untouched) reading
  uint32_t value;          // latest reading
  uint8_t confirm;         // readings in a row above the threshold
  bool down;               // being touched right now
  unsigned long downAt;
  bool held;               // this touch became a hold (or a cancel), so letting go isn't a tap
};
TouchPad pads[2] = {
  { TOUCH_PIN_1, "A2", 1 },
  { TOUCH_PIN_2, "A3", 10 },
};
bool ignoreUntilReleased = false;  // after a cancel, wait for hands off before reading taps again
unsigned long bothSince = 0;
bool showReadings = false;
unsigned long lastReadingsPrint = 0;

// --- Forward Declarations ---
void updateDisplay();
void updateLEDs();
void cancelTimer();
void startTimer();
void handleInputs();
void checkPowerSaving();
void calibrateTouch();
void readPad(TouchPad &p);
void handleSerial();
void printReadings();

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

  calibrateTouch();

  lastInteractionTime = millis();
  Serial.println("Airport Challenge Timer Initialized!  (t = live touch readings, c = re-calibrate)");
}

void loop() {
  unsigned long currentMillis = millis();

  EVERY_N_MILLISECONDS(FLASH_INTERVAL_MS) {
    flashState = !flashState;
  }

  handleInputs();
  handleSerial();
  printReadings();

  if (timerRunning) {
    unsigned long elapsed = currentMillis - timerStartMillis;
    if (elapsed >= timerDurationMillis) {
      // Time's up! Flash until someone touches a pad
      timerRunning = false;
      minutes = 0;
      timerDurationMillis = 0;
      timesUp = true;
      timesUpAt = currentMillis;
      lastInteractionTime = currentMillis;
      Serial.println("TIME'S UP!");
    }
  }
  if (timesUp && currentMillis - timesUpAt > TIMES_UP_MS) timesUp = false;

  updateDisplay();
  updateLEDs();
  checkPowerSaving();

  FastLED.show();
  delay(20);
}

// Measure each pad's resting value. Averaged so one noisy reading doesn't throw it off.
void calibrateTouch() {
  Serial.println("Calibrating touch pads - hands off...");
  for (TouchPad &p : pads) {
    uint64_t sum = 0;
    for (int s = 0; s < 16; s++) {
      sum += touchRead(p.pin);
      delay(5);
    }
    p.baseline = sum / 16;
    p.confirm = 0;
    p.down = false;
    Serial.printf("  %s resting %lu  (touch above %lu)\n", p.name, (unsigned long)p.baseline,
                  (unsigned long)(p.baseline + max(TOUCH_MIN_JUMP, p.baseline * TOUCH_PERCENT / 100)));
  }
}

uint32_t touchJump(const TouchPad &p) {
  return max(TOUCH_MIN_JUMP, p.baseline * TOUCH_PERCENT / 100);
}

// Read one pad and update whether it's being touched
void readPad(TouchPad &p) {
  p.value = touchRead(p.pin);
  uint32_t jump = touchJump(p);

  if (!p.down) {
    if (p.value > p.baseline + jump) {
      if (++p.confirm >= TOUCH_CONFIRM) {
        p.down = true;
        p.downAt = millis();
        p.held = false;
      }
    } else {
      p.confirm = 0;
      // untouched: slowly follow changes in the resting value (how it's worn, damp fabric...)
      if (p.value < p.baseline + jump / 2) p.baseline = (p.baseline * 31 + p.value) / 32;
    }
  } else if (p.value < p.baseline + jump / 2) {
    // released once it drops back below half way (so a wobbly touch doesn't count twice)
    p.down = false;
    p.confirm = 0;
  }
}

/**
 * @brief Handles touch inputs: taps add time, a hold starts, both pads cancel.
 */
void handleInputs() {
  unsigned long currentMillis = millis();
  bool wasDown[2] = { pads[0].down, pads[1].down };
  readPad(pads[0]);
  readPad(pads[1]);

  // After a cancel (or clearing "time's up"), wait for both hands off
  if (ignoreUntilReleased) {
    if (!pads[0].down && !pads[1].down) ignoreUntilReleased = false;
    return;
  }

  // Any touch clears the "time's up" alert
  if (timesUp && (pads[0].down || pads[1].down)) {
    timesUp = false;
    ignoreUntilReleased = true;
    lastInteractionTime = currentMillis;
    return;
  }

  // --- Cancel Feature: Touch both pads together ---
  if (pads[0].down && pads[1].down) {
    pads[0].held = pads[1].held = true;   // letting go of these isn't a tap
    if (!bothSince) bothSince = currentMillis;
    if (currentMillis - bothSince >= CANCEL_HOLD_MS) {
      cancelTimer();
      ignoreUntilReleased = true;
      lastInteractionTime = currentMillis;
    }
    return;
  }
  bothSince = 0;

  for (int i = 0; i < 2; i++) {
    TouchPad &p = pads[i];

    if (p.down && !wasDown[i]) {           // Touched
      lastInteractionTime = currentMillis;
    }

    if (!p.down && wasDown[i] && !p.held && !timerRunning) {   // Released = tap
      minutes += p.addMinutes;
      if (minutes > MAX_MINUTES) minutes = MAX_MINUTES;
      lastInteractionTime = currentMillis;
      Serial.printf("+%d min -> %d mins\n", p.addMinutes, minutes);
    }

    // --- Hold Gesture Detection ---
    if (p.down && !p.held && currentMillis - p.downAt > HOLD_THRESHOLD) {
      p.held = true;
      startTimer();
      lastInteractionTime = currentMillis;
    }
  }
}

void startTimer() {
  if (minutes > 0 && !timerRunning) {
    timerRunning = true;
    timerStartMillis = millis();
    timerDurationMillis = (unsigned long)minutes * MILLIS_IN_A_MINUTE;
    Serial.printf("Countdown started: %d mins\n", minutes);
  }
}

void cancelTimer() {
  timerRunning = false;
  minutes = 0;
  timerDurationMillis = 0; // Ensure this is also reset
  Serial.println("Cancelled");
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(500);
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 't') showReadings = !showReadings;
    if (c == 'c') calibrateTouch();
  }
}

// Live readings for tuning: reading and how far over the threshold it is
void printReadings() {
  if (!showReadings || millis() - lastReadingsPrint < 250) return;
  lastReadingsPrint = millis();
  for (const TouchPad &p : pads) {
    long rise = (long)p.value - (long)p.baseline;
    Serial.printf("%s %7lu (%+4ld%%)%s   ", p.name, (unsigned long)p.value,
                  rise * 100 / (long)touchJump(p), p.down ? "*" : " ");
  }
  Serial.println("  100% = touch");
}

void updateDisplay() {
  long remainingSeconds = 0;
  if (timerRunning) {
    unsigned long elapsed = millis() - timerStartMillis;
    remainingSeconds = (timerDurationMillis - elapsed) / 1000;
    if (remainingSeconds < 0) remainingSeconds = 0;
  }
  bool timesUpFlash = timesUp && flashState;

  // Only update the display if something has changed
  if (minutes == lastDisplayedMinutes && timerRunning == lastTimerState &&
      remainingSeconds == lastDisplayedSeconds && timesUpFlash == lastTimesUpFlash) {
    return;
  }

  display.clearDisplay();
  display.invertDisplay(timesUpFlash);

  if (timesUp) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(3);
    display.setCursor(10, 8);
    display.println("TIME'S");
    display.setCursor(37, 36);
    display.println("UP!");
  } else if (timerRunning) {
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
  lastTimesUpFlash = timesUpFlash;
}

void updateLEDs() {
  static uint8_t rainbowHue = 0;

  if (timesUp) {
    fill_solid(leds, NUM_LEDS, flashState ? CRGB::Red : CRGB::White);
  } else if (timerRunning) {
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

  // Touch feedback: a touched pad lights its half of the strip white
  if (!timesUp) {
    if (pads[0].down) { leds[0] = CRGB::White; leds[1] = CRGB::White; }
    if (pads[1].down) { leds[2] = CRGB::White; leds[3] = CRGB::White; }
  }
}

void checkPowerSaving() {
  if (millis() - lastInteractionTime > INACTIVITY_TIMEOUT_MS) {
    FastLED.setBrightness(DIM_BRIGHTNESS);
  } else {
    FastLED.setBrightness(BRIGHTNESS);
  }
}
