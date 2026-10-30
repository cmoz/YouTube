/*
 * WEARABLE SIMON SAYS GAME
 * For ESP32-S3 QT Py with 5 WS2812B LEDs and Conductive Fabric Touch Sensors
 * Using built-in capacitive touch sensing
 * By CMozMaker YouTube Channel - https://www.youtube.com/@CMozMaker
 *
 * 2025 - updated 2026
 *
 * How it works:
 *   - Power on: a rainbow fade plays, then each fabric pad is calibrated to its
 *     own resting value (keep hands off the pads during the rainbow!)
 *   - Touch A2 to start. Each round adds one colour to the sequence - watch it,
 *     then touch the pads back in the same order
 *   - A touch counts when a pad rises TOUCH_PERCENT above its resting value. The
 *     pad's LED lights the moment it registers. If touching one pad nudges the
 *     others, the pad that rose the most wins
 *   - You get TURN_TIMEOUT_MS between touches (holding a pad doesn't use it up)
 *   - Wrong pad or too slow = game over. Your score (rounds completed) is shown
 *     in binary, LED 0 = 1, so on the wrist it reads like normal binary: 00011 = 3
 *   - Remember all 31 and you win!
 *
 * 2026 update: per-pad calibration instead of one fixed threshold, strongest-pad
 * detection, instant touch feedback, more time per touch, win at 31 rounds.
 *
 * Serial Monitor (115200 baud):
 *   t = show/hide live touch readings (for tuning)
 *   c = re-calibrate the pads (hands off the fabric!)
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions
#define LED_PIN A1
#define NUM_LEDS 5

#define TOUCH_PIN_A2  9
#define TOUCH_PIN_A3  8
#define TOUCH_PIN_SDA 7
#define TOUCH_PIN_SCL 6
#define TOUCH_PIN_TX  5

// Arrays for convenience
const int touchPins[] = {TOUCH_PIN_A2, TOUCH_PIN_A3, TOUCH_PIN_SDA, TOUCH_PIN_SCL, TOUCH_PIN_TX};
const char* pinNames[] = {"A2", "A3", "SDA", "SCL", "TX"};

// ══════════════════ TOUCH TUNING ══════════════════
// Each pad is calibrated at power-on, so these are relative to its own resting value.
const uint32_t TOUCH_PERCENT   = 20;    // touch = reading rises this % above the pad's resting value
const uint32_t TOUCH_MIN_JUMP  = 2000;  // ...and by at least this much (stops noise counting as a touch)
const uint8_t  TOUCH_CONFIRM   = 2;     // readings in a row needed before a touch counts
const unsigned long TURN_TIMEOUT_MS = 6000;  // time allowed for each touch on your turn
// ══════════════════════════════════════════════════

uint32_t baseline[NUM_LEDS];      // each pad's resting (untouched) reading
uint32_t lastVal[NUM_LEDS];       // latest reading of each pad
uint8_t  confirmCount[NUM_LEDS];  // readings in a row above the threshold
int      heldPad = -1;            // pad being touched right now, -1 = none
bool     showReadings = false;
unsigned long lastReadingsPrint = 0;

// NeoPixel setup
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
const int BRIGHTNESS = 20;

#define BUILTIN_NEOPIXEL_PIN 39
Adafruit_NeoPixel builtinPixel(1, BUILTIN_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint8_t breathBrightness = 0;
int breathStep = 5;
unsigned long lastUpdate = 0;
const unsigned long breathInterval = 30;

// Game colors (strong, vibrant colors)
const uint32_t gameColors[] = {
  strip.Color(255, 0, 144),    // Pink/Magenta
  strip.Color(0, 255, 255),    // Cyan
  strip.Color(255, 255, 0),    // Yellow
  strip.Color(0, 255, 0),      // Green
  strip.Color(255, 85, 0)      // Orange
};

const uint32_t fadeColor = strip.Color(0, 0, 0);  // Off between inputs
const uint32_t scoreColor = strip.Color(0, 50, 255);  // Blue for score display

// Game variables
const int MAX_ROUNDS = 31;  // max binary 11111 = 31
int sequence[MAX_ROUNDS];
int sequenceLength = 0;
int currentRound = 0;
bool gameActive = false;
bool waitingForStart = true;

// Function declarations
void calibrateTouch();
int pollTouch();
void waitForRelease();
void handleSerial();
void printReadings();
void startGame();
void startAnimation();
void powerOnAnimation();
void playRound();
void showSequence();
bool getPlayerInput();
void gameOver(bool won);
void displayBinaryScore(int score);
void setupBuiltInBreathing();
void updateBuiltInBreathing();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Simon Memory Game - QT Py ESP32-S3");

  setupBuiltInBreathing();

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  randomSeed(esp_random());

  // Power-on rainbow fade animation (keep hands off the pads - we calibrate right after)
  powerOnAnimation();
  calibrateTouch();

  Serial.println("Touch A2 to start the game!  (t = live touch readings, c = re-calibrate)");
}

void loop() {
  if (waitingForStart) {
    // Wait for A2 touch to start
    if (pollTouch() == 0) {
      Serial.println("Game starting!");
      startGame();
      waitingForStart = false;
    }
    delay(10);
  } else if (gameActive) {
    playRound();
  }
}

// Measure each pad's resting value. Averaged so one noisy reading doesn't throw it off.
void calibrateTouch() {
  Serial.println("Calibrating touch pads - hands off...");
  for (int i = 0; i < NUM_LEDS; i++) {
    uint64_t sum = 0;
    for (int s = 0; s < 16; s++) {
      sum += touchRead(touchPins[i]);
      delay(5);
    }
    baseline[i] = sum / 16;
    confirmCount[i] = 0;
    Serial.printf("  %-3s resting %lu  (touch above %lu)\n", pinNames[i],
                  (unsigned long)baseline[i],
                  (unsigned long)(baseline[i] + max(TOUCH_MIN_JUMP, baseline[i] * TOUCH_PERCENT / 100)));
  }
  heldPad = -1;
}

uint32_t touchJump(int i) {
  return max(TOUCH_MIN_JUMP, baseline[i] * TOUCH_PERCENT / 100);
}

// Reads every pad and works out which one is being touched.
// Returns the pad number the moment a NEW touch starts, otherwise -1.
// Touching one fabric pad can nudge the others up too, so when several pads
// read high we pick the one that rose the most.
int pollTouch() {
  int best = -1;
  float bestRise = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t v = touchRead(touchPins[i]);
    lastVal[i] = v;
    uint32_t jump = touchJump(i);

    if (v > baseline[i] + jump) {
      if (confirmCount[i] < 255) confirmCount[i]++;
      float rise = float(v - baseline[i]) / jump;
      if (confirmCount[i] >= TOUCH_CONFIRM && rise > bestRise) {
        bestRise = rise;
        best = i;
      }
    } else {
      confirmCount[i] = 0;
      // untouched: slowly follow changes in the resting value (how it's worn, damp fabric...)
      if (v < baseline[i] + jump / 2) baseline[i] = (baseline[i] * 31 + v) / 32;
    }
  }

  updateBuiltInBreathing();
  handleSerial();
  printReadings();

  if (heldPad == -1 && best != -1) {
    heldPad = best;
    Serial.print("Touch detected on ");
    Serial.println(pinNames[best]);
    return best;
  }
  // released once it drops back below half way (so a wobbly touch doesn't count twice)
  if (heldPad != -1 && lastVal[heldPad] < baseline[heldPad] + touchJump(heldPad) / 2) {
    heldPad = -1;
  }
  return -1;
}

// Wait until no pad has been touched for a moment, so a finger left on a pad
// doesn't count as the first move of your turn
void waitForRelease() {
  unsigned long clearSince = millis();
  unsigned long giveUp = millis() + 3000;
  while (millis() - clearSince < 150 && millis() < giveUp) {
    pollTouch();
    if (heldPad != -1) clearSince = millis();
    delay(5);
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 't') showReadings = !showReadings;
    if (c == 'c') calibrateTouch();
  }
}

// Live readings for tuning: reading / resting value and how far over the threshold it is
void printReadings() {
  if (!showReadings || millis() - lastReadingsPrint < 250) return;
  lastReadingsPrint = millis();
  for (int i = 0; i < NUM_LEDS; i++) {
    long rise = (long)lastVal[i] - (long)baseline[i];
    Serial.printf("%-3s %6lu (%+4ld%%)%s  ", pinNames[i], (unsigned long)lastVal[i],
                  rise * 100 / (long)touchJump(i), i == heldPad ? "*" : " ");
  }
  Serial.println("   100% = touch");
}

void startGame() {
  sequenceLength = 0;
  currentRound = 0;
  gameActive = true;

  // Starting animation
  startAnimation();

  delay(1000);
}

void startAnimation() {
  // Rainbow wave
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, gameColors[i]);
      strip.show();
      delay(100);
    }
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, 0);
      strip.show();
      delay(100);
    }
  }

  delay(500);
}

void powerOnAnimation() {
  // Rainbow fade on startup
  for (int brightness = 0; brightness < 255; brightness += 5) {
    for (int i = 0; i < NUM_LEDS; i++) {
      // Create rainbow effect
      int hue = (i * 65536L / NUM_LEDS);
      uint32_t color = strip.gamma32(strip.ColorHSV(hue, 255, brightness));
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(20);
  }

  // Hold at full brightness
  delay(500);

  // Fade out
  for (int brightness = 255; brightness >= 0; brightness -= 5) {
    for (int i = 0; i < NUM_LEDS; i++) {
      int hue = (i * 65536L / NUM_LEDS);
      uint32_t color = strip.gamma32(strip.ColorHSV(hue, 255, brightness));
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(20);
  }

  strip.clear();
  strip.show();
  delay(500);
}

void playRound() {
  if (sequenceLength >= MAX_ROUNDS) {
    Serial.println("You remembered all 31 - you win!");
    gameOver(true);
    return;
  }

  // Add new color to sequence
  sequence[sequenceLength] = random(0, NUM_LEDS);
  sequenceLength++;
  currentRound++;

  Serial.print("Round ");
  Serial.print(currentRound);
  Serial.print(" - Sequence length: ");
  Serial.println(sequenceLength);

  delay(1000);

  // Show the sequence
  showSequence();

  // Get player input
  if (!getPlayerInput()) {
    // Player failed - the score is the rounds fully completed
    currentRound--;
    gameOver(false);
  }
}

void showSequence() {
  Serial.println("Showing sequence...");

  for (int i = 0; i < sequenceLength; i++) {
    int led = sequence[i];

    Serial.print("LED ");
    Serial.print(led);
    Serial.print(" (");
    Serial.print(pinNames[led]);
    Serial.println(")");

    // Light up LED
    strip.setPixelColor(led, gameColors[led]);
    strip.show();
    delay(1000);  // On for 1 second

    // Turn off
    strip.setPixelColor(led, fadeColor);
    strip.show();
    delay(300);
  }

  Serial.println("Your turn!");
}

bool getPlayerInput() {
  waitForRelease();

  int inputIndex = 0;
  int litPad = -1;                 // LED showing the pad you're touching
  unsigned long litAt = 0;
  unsigned long lastAction = millis();

  while (inputIndex < sequenceLength) {
    int pad = pollTouch();

    // Holding a pad doesn't use up your time - only the gap between touches does
    if (heldPad != -1) lastAction = millis();
    if (millis() - lastAction > TURN_TIMEOUT_MS) {
      Serial.println("Timeout!");
      strip.clear();
      strip.show();
      return false;
    }

    // Turn the LED off once the finger lifts (lit for at least a moment, so quick taps still show)
    if (litPad != -1 && heldPad != litPad && millis() - litAt > 250) {
      strip.setPixelColor(litPad, fadeColor);
      strip.show();
      litPad = -1;
    }

    if (pad != -1) {
      // Light it straight away so you can see the touch registered
      if (litPad != -1) strip.setPixelColor(litPad, fadeColor);
      strip.setPixelColor(pad, gameColors[pad]);
      strip.show();
      litPad = pad;
      litAt = millis();

      // Check if correct
      if (pad == sequence[inputIndex]) {
        Serial.println("Correct!");
        inputIndex++;
      } else {
        Serial.println("Wrong! Game Over!");
        delay(300);
        return false;
      }
    }

    delay(5);
  }

  // let the last touch finish before the next round starts
  waitForRelease();
  delay(250);
  strip.clear();
  strip.show();

  Serial.println("Round complete!");
  return true;
}

void gameOver(bool won) {
  gameActive = false;

  Serial.print("Game Over! Final Score: ");
  Serial.println(currentRound);

  // Flash animation (red) - skipped if you won
  for (int i = 0; i < 3 && !won; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      strip.setPixelColor(j, strip.Color(255, 0, 0));
    }
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(200);
  }

  delay(500);

  // Rainbow flutter
  for (int cycle = 0; cycle < 10; cycle++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, gameColors[i]);
    }
    strip.show();
    delay(100);
    strip.clear();
    strip.show();
    delay(100);
  }

  delay(1000);

  // Display score in binary
  displayBinaryScore(currentRound);

  delay(5000);

  // Clear and wait for restart
  strip.clear();
  strip.show();

  waitForRelease();
  waitingForStart = true;
  Serial.println("\nTouch A2 to play again!");
}

void displayBinaryScore(int score) {
  Serial.print("Displaying score in binary: ");

  strip.clear();

  // LED 0 is the least significant bit, so on the wrist it reads like normal binary
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    if (score & (1 << i)) {
      strip.setPixelColor(i, scoreColor);
      Serial.print("1");
    } else {
      Serial.print("0");
    }
  }

  Serial.println();
  strip.show();
}

void setupBuiltInBreathing() {
#ifdef NEOPIXEL_POWER
  // the QT Py's onboard pixel has its own power switch - turn it on
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif
  builtinPixel.begin();
  builtinPixel.show();
}

void updateBuiltInBreathing() {
  unsigned long now = millis();
  if (now - lastUpdate > breathInterval) {
    breathBrightness += breathStep;
    if (breathBrightness == 0 || breathBrightness == 255) breathStep = -breathStep;

    // Breathe in blue
    builtinPixel.setPixelColor(0, builtinPixel.Color(0, 0, breathBrightness));
    builtinPixel.show();

    lastUpdate = now;
  }
}
