

// added that touch needs to be registered on pin 3 for game start







#include <Arduino.h>

/*
 * WEARABLE SIMON SAYS GAME
 * For ESP32-S3 QT Py with 7 WS2812B LEDs and Conductive Fabric Touch Sensors
 * Using built-in capacitive touch sensing
 * By CMozMaker
 */

#include <Adafruit_NeoPixel.h>

// Pin Definitions
#define LED_PIN 8          // NeoPixel data pin
#define NUM_LEDS 7         // Number of LEDs/sensors

// Touch sensor pins (ESP32-S3 capacitive touch pins)
// T1-T7 correspond to GPIO pins that support touch
const int touchPins[NUM_LEDS] = {1, 2, 3, 4, 5, 6, 7}; // GPIO 1-7

// NeoPixel setup
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Game variables
int sequence[100];           // Stores the game sequence
int sequenceLength = 0;      // Current sequence length
int playerInput[100];        // Stores player's input
int playerIndex = 0;         // Current position in player input
bool gameActive = false;     // Game state
int highScore = 0;           // Best score

// Touch sensing variables
int touchThreshold[NUM_LEDS]; // Threshold for each sensor
const int TOUCH_SENSITIVITY = 20; // Lower = more sensitive (adjust 10-40)
unsigned long lastTouchTime = 0;
const int DEBOUNCE_DELAY = 600; // milliseconds

#define START_SENSOR 3 // Only sensor 3 starts the game

// Colors for each LED (rainbow pattern)
uint32_t colors[NUM_LEDS] = {
  strip.Color(255, 0, 0),     // Red
  strip.Color(255, 127, 0),   // Orange
  strip.Color(255, 255, 0),   // Yellow
  strip.Color(0, 255, 0),     // Green
  strip.Color(0, 0, 255),     // Blue
  strip.Color(75, 0, 130),    // Indigo
  strip.Color(148, 0, 211)    // Violet
};

void calibrateTouchSensors();
bool isTouched(int sensorIndex);
int checkForTouch();  
void startNewGame();
void addToSequence();
void playSequence();
void handlePlayerInput(int touchedSensor);
void gameOver();
void flashLED(int led);
void welcomeAnimation();
void idleAnimation();
void successAnimation();

void calibrateTouchSensors() {
  Serial.println("Calibrating touch sensors...");
  Serial.println("Don't touch the sensors during calibration!");
  delay(1000);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // Take multiple readings and average
    long sum = 0;
    for (int j = 0; j < 10; j++) {
      sum += touchRead(touchPins[i]);
      delay(10);
    }
    int baseline = sum / 10;
    
    // Set threshold as percentage below baseline
    touchThreshold[i] = baseline * (100 - TOUCH_SENSITIVITY) / 100;
    
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(" (GPIO ");
    Serial.print(touchPins[i]);
    Serial.print(") baseline: ");
    Serial.print(baseline);
    Serial.print(", threshold: ");
    Serial.println(touchThreshold[i]);
  }
  
  Serial.println("Calibration complete!");
}

/*
    bool isTouched(int sensorIndex) {
      int reading = touchRead(touchPins[sensorIndex]);
      
      // Capacitive touch reads LOWER when touched
      bool touched = (reading < touchThreshold[sensorIndex]);
      
      // Debug output (comment out for production)
      if (touched) {
        Serial.print("Touch detected on sensor ");
        Serial.print(sensorIndex);
        Serial.print(" - Reading: ");
        Serial.print(reading);
        Serial.print(" < Threshold: ");
        Serial.println(touchThreshold[sensorIndex]);
      }
      
      return touched;
    }
*/

bool isTouched(int sensorIndex) {
  // TTP223 pulls LOW when touched
  bool touched = (digitalRead(touchPins[sensorIndex]) == LOW);

  if (touched) {
    Serial.print("Touch detected on sensor ");
    Serial.println(sensorIndex);
  }

  return touched;
}


int checkForTouch() {
  // Debouncing
  if (millis() - lastTouchTime < DEBOUNCE_DELAY) {
    return -1;
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    if (isTouched(i)) {
      lastTouchTime = millis();
      flashLED(i);
      return i;
    }
  }
  
  return -1; // No touch detected
}

void startNewGame() {
  Serial.println("\n=== NEW GAME STARTED ===");
  gameActive = true;
  sequenceLength = 0;
  playerIndex = 0;
  
  // Clear display
  strip.clear();
  strip.show();
  delay(500);
  
  // Add first item to sequence
  addToSequence();
  playSequence();
}

void addToSequence() {
  sequence[sequenceLength] = random(NUM_LEDS);
  sequenceLength++;
  Serial.print("Sequence length: ");
  Serial.println(sequenceLength);
}

void playSequence() {
  Serial.println("Playing sequence...");
  delay(500);
  
  for (int i = 0; i < sequenceLength; i++) {
    int led = sequence[i];
    
    // Light up LED
    strip.setPixelColor(led, colors[led]);
    strip.show();
    delay(500);
    
    // Turn off LED
    strip.setPixelColor(led, 0);
    strip.show();
    delay(200);
  }
  
  Serial.println("Your turn!");
  playerIndex = 0;
}

void handlePlayerInput(int touchedSensor) {
  Serial.print("Player touched: ");
  Serial.println(touchedSensor);
  
  // Check if input matches sequence
  if (touchedSensor == sequence[playerIndex]) {
    Serial.println("Correct!");
    playerIndex++;
    
    // Check if player completed the sequence
    if (playerIndex == sequenceLength) {
      Serial.println("Sequence complete!");
      successAnimation();
      delay(500);
      
      // Add new element and play next round
      addToSequence();
      playSequence();
    }
  } else {
    // Wrong input - game over
    Serial.println("WRONG!");
    gameOver();
  }
}

void gameOver() {
  Serial.println("\n=== GAME OVER ===");
  Serial.print("Your score: ");
  Serial.println(sequenceLength - 1);
  
  if (sequenceLength - 1 > highScore) {
    highScore = sequenceLength - 1;
    Serial.print("NEW HIGH SCORE: ");
    Serial.println(highScore);
  }
  
  gameActive = false;
  
  // Game over animation
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
  
  Serial.println("\nTouch any sensor to play again...");
  delay(1000);
}

void flashLED(int led) {
  strip.setPixelColor(led, colors[led]);
  strip.show();
  delay(200);
  strip.setPixelColor(led, 0);
  strip.show();
}

void welcomeAnimation() {
  // Rainbow chase
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, colors[i]);
      strip.show();
      delay(100);
    }
    strip.clear();
    strip.show();
    delay(100);
  }
}

void idleAnimation() {
  // Gentle breathing effect on all LEDs
  static unsigned long lastUpdate = 0;
  static int brightness = 0;
  static int fadeAmount = 5;
  
  if (millis() - lastUpdate > 30) {
    brightness += fadeAmount;
    
    if (brightness <= 0 || brightness >= 100) {
      fadeAmount = -fadeAmount;
    }
    
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, brightness));
    }
    strip.show();
    
    lastUpdate = millis();
  }
}

void successAnimation() {
  // Quick sparkle effect
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  delay(300);
  strip.clear();
  strip.show();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Initialize NeoPixels
  strip.begin();
  strip.setBrightness(50); // Set brightness (0-255)
  strip.show();
  
    // Set up digital touch pins
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(touchPins[i], INPUT);
  }

  // Calibrate touch sensors
  // calibrateTouchSensors();
  
  // Seed random number generator
  randomSeed(analogRead(0));
  
  // Welcome animation
  welcomeAnimation();
  
  Serial.println("Wearable Simon Says Ready!");
  Serial.println("Touch any sensor to start...");
}

void loop() {
  if (!gameActive && isTouched(START_SENSOR)) {
  startNewGame();
  delay(1000);
} else {
    // Game is active - wait for player input
    int touchedSensor = checkForTouch();
    
    if (touchedSensor >= 0) {
      handlePlayerInput(touchedSensor);
    }
  }
}
