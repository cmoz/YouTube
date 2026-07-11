#include <Arduino.h>

#define LED_PIN 2  // Built-in LED on ESP32-C3 Mini super mini might be pin 8

void setup() {
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);  // Turn LED on
    delay(500);                   // Wait 500 ms
    digitalWrite(LED_PIN, LOW);   // Turn LED off
    delay(500);                   // Wait 500 ms
}