#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 4 letters, each represented as a 5x5 pixel array (1=on, 0=off)
uint8_t letters[4][5][5] = {
  // Letter C
  {
    {0,1,1,1,1},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {0,1,1,1,1}
  },
  // Letter M
  {
    {1,0,1,0,1},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1}
  },
  // Letter O
  {
    {0,1,1,1,0},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {0,1,1,1,0}
  },
  // Letter Z
  {
    {1,1,1,1,1},
    {0,0,0,0,1},
    {0,0,0,1,0},
    {0,0,1,0,0},
    {1,1,1,1,1}
  }
};

void setup() {
  matrix.begin();
  matrix.setBrightness(64);  // ~25% brightness
  matrix.clear();
  matrix.show();
}

void displayLetter(int index) {
  matrix.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (letters[index][y][x]) {
        int pixel = y * MATRIX_WIDTH + x; // row-major mapping
        matrix.setPixelColor(pixel, matrix.Color(0, 128, 128));  // teal
      }
    }
  }
  matrix.show();
}

void loop() {
  for (int i = 0; i < 4; i++) {
    displayLetter(i);
    delay(500);  // hold each letter on screen for 500 milliseconds
  }
}
