#include <Arduino.h>

#define TOUCH_PIN_A2  9
#define TOUCH_PIN_A3  8
#define TOUCH_PIN_SDA 7
#define TOUCH_PIN_SCL 6
#define TOUCH_PIN_TX  5

// Arrays for convenience
const int touchPins[] = {TOUCH_PIN_A2, TOUCH_PIN_A3, TOUCH_PIN_SDA, TOUCH_PIN_SCL, TOUCH_PIN_TX};
const char* pinNames[] = {"A2", "A3", "SDA", "SCL", "TX"};

int baselines[5];
int thresholds[5];
const int thresholdMargin = 500;  // Adjust based on your environment

const int jumpThreshold = 20000;  // Threshold for jump detection

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("QT Py ESP32-S3 Touch Sensor Test");
  delay(1000);

  // Calibrate baseline and thresholds for all pins
  for (int i = 0; i < 5; i++) {
    baselines[i] = touchRead(touchPins[i]);
    thresholds[i] = baselines[i] - thresholdMargin;
    Serial.print(pinNames[i]);
    Serial.print(" baseline: ");
    Serial.println(baselines[i]);
  }
  Serial.println();
}

void loop() {
  for (int i = 0; i < 5; i++) {
    int val = touchRead(touchPins[i]);
    Serial.print(pinNames[i]);
    Serial.print(": ");
    Serial.print(val);
    if (val > jumpThreshold) {
      Serial.print(" - Touch detected on ");
      Serial.print(pinNames[i]);
    }
    Serial.println();
  }
  Serial.println();
  delay(1000);
}
