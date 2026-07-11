/*
 * WEARABLE SIMON SAYS GAME
 * For ESP32-S3 QT Py with 5 WS2812B LEDs and Conductive Fabric Touch Sensors
 * Using built-in capacitive touch sensing
 * By CMozMaker YouTube Channel - https://www.youtube.com/@CMozMaker 
 * 
 * 2025
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

// Touch detection
const int jumpThreshold = 20000;
bool touchStates[5] = {false, false, false, false, false};

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
int sequence[31];  // Store up to 31 rounds (max binary 11111 = 31)
int sequenceLength = 0;
int currentRound = 0;
bool gameActive = false;
bool waitingForStart = true;

// Function declarations
bool checkTouch(int pinIndex);
void startGame();
void startAnimation();
void powerOnAnimation();
void playRound();
void showSequence();
bool getPlayerInput();
void gameOver();
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
  
  randomSeed(analogRead(A0));
  
  // Power-on rainbow fade animation
  powerOnAnimation();
  
  Serial.println("Touch A2 to start the game!");
}

void loop() {
  if (waitingForStart) {
    // Wait for A2 touch to start
    if (checkTouch(0)) {
      Serial.println("Game starting!");
      startGame();
      waitingForStart = false;
    }
    delay(100);
  } else if (gameActive) {
    playRound();
  }
}

bool checkTouch(int pinIndex) {
  int val = touchRead(touchPins[pinIndex]);
  
  // Detect rising edge (not touched -> touched)
  if (val > jumpThreshold && !touchStates[pinIndex]) {
    touchStates[pinIndex] = true;
    Serial.print("Touch detected on ");
    Serial.println(pinNames[pinIndex]);
    return true;
  } else if (val <= jumpThreshold) {
    touchStates[pinIndex] = false;
  }
  
  updateBuiltInBreathing();
  return false;
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
  
  delay(1000);
  
  // Get player input
  if (!getPlayerInput()) {
    // Player failed
    gameOver();
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
  int timeout = sequenceLength * 3000;  // 3 seconds per input needed
  unsigned long startTime = millis();
  int inputIndex = 0;
  
  while (inputIndex < sequenceLength) {
    // Check for timeout
    if (millis() - startTime > timeout) {
      Serial.println("Timeout!");
      return false;
    }
    
    // Check each touch sensor
    for (int i = 0; i < NUM_LEDS; i++) {
      if (checkTouch(i)) {
        // Flash the LED
        strip.setPixelColor(i, gameColors[i]);
        strip.show();
        delay(300);
        strip.setPixelColor(i, fadeColor);
        strip.show();
        
        // Check if correct
        if (i == sequence[inputIndex]) {
          Serial.println("Correct!");
          inputIndex++;
          startTime = millis();  // Reset timeout after each correct input
          break;
        } else {
          Serial.println("Wrong! Game Over!");
          return false;
        }
      }
    }
    
    delay(10);
  }
  
  Serial.println("Round complete!");
  return true;
}

void gameOver() {
  gameActive = false;
  
  Serial.print("Game Over! Final Score: ");
  Serial.println(currentRound);
  
  // Flash animation (red)
  for (int i = 0; i < 3; i++) {
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
  
  waitingForStart = true;
  Serial.println("\nTouch A2 to play again!");
}

void displayBinaryScore(int score) {
  Serial.print("Displaying score in binary: ");
  
  strip.clear();
  
  // Display from LED 4 to LED 0 (left to right as most significant to least significant)
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    if (score & (1 << i)) {
      strip.setPixelColor(NUM_LEDS - 1 - i, scoreColor);
      Serial.print("1");
    } else {
      Serial.print("0");
    }
  }
  
  Serial.println();
  strip.show();
}

void setupBuiltInBreathing() {
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