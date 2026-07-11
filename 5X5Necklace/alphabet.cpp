#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "font5x5.h"  // Include your font file

#define LED_PIN 8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char *message = "HELLO WORLD 123";

void displayLetter(int index, uint32_t color) {
  matrix.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (letters[index][y][x]) {
        int pixel = y * MATRIX_WIDTH + x;
        matrix.setPixelColor(pixel, color);
      }
    }
  }
  matrix.show();
}

void setup() {
  matrix.begin();
  matrix.setBrightness(64);
  matrix.clear();
  matrix.show();
}

void loop() {
  for (int i = 0; message[i] != '\0'; i++) {
    int index = getLetterIndex(toupper(message[i]));
    uint32_t color =  matrix.Color(0, 128, 128);
    displayLetter(index, color);
    delay(500);
  }
}
