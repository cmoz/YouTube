#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 5x5 pixel font for letters H, E, L, O, W, R, D
// and space (all zeros)
uint8_t letters[][5][5] = {
  // H
  {
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,1,1,1,1},
    {1,0,0,0,1},
    {1,0,0,0,1}
  },
  // E
  {
    {1,1,1,1,1},
    {1,0,0,0,0},
    {1,1,1,1,0},
    {1,0,0,0,0},
    {1,1,1,1,1}
  },
  // L
  {
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,1,1,1,1}
  },
  // O
  {
    {0,1,1,1,0},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {0,1,1,1,0}
  },
  // W
  {
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,1,0,1},
    {1,1,0,1,1},
    {1,0,0,0,1}
  },
  // R
  {
    {1,1,1,1,0},
    {1,0,0,0,1},
    {1,1,1,1,0},
    {1,0,1,0,0},
    {1,0,0,1,0}
  },
  // D
  {
    {1,1,1,0,0},
    {1,0,0,1,0},
    {1,0,0,0,1},
    {1,0,0,1,0},
    {1,1,1,0,0}
  },
  // Space (all zero pixels) 
  {
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0}
  },
    // dash
  {
    {0,0,0,0,0},
    {0,0,0,0,0},
    {1,1,1,1,1},
    {0,0,0,0,0},
    {0,0,0,0,0}
  },
};

// Map characters to letters array indexes:
char message[] = "HELLO WORLD";

void funRandomColorsAnimation(int cycles, int delayMs);

int getLetterIndex(char c) {
  switch(c) {
    case 'H': return 0;
    case 'E': return 1;
    case 'L': return 2;
    case 'O': return 3;
    case 'W': return 4;
    case 'R': return 5;
    case 'D': return 6;
    case ' ': return 7;
    case '-': return 8;
  }
  return 7; // default to blank space if unknown
}

void setup() {
  matrix.begin();
  matrix.setBrightness(35);  // value 64 is ~25% brightness
  matrix.clear();
  matrix.show();
}

void displayLetter(int index) {
  matrix.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      int pixel = y * MATRIX_WIDTH + x; // row-major mapping
      if (letters[index][y][x]) {
        // Pixel is part of the letter - teal
        matrix.setPixelColor(pixel, matrix.Color(0, 128, 128));  // teal
      } else {
        // Pixel is background - pink
        //matrix.setPixelColor(pixel, matrix.Color(255, 105, 180));  // pink (hot pink)
      }
    }
  }
  matrix.show();
}

void loop() {
  for (int i = 0; i < sizeof(message) - 1; i++) { // -1 to ignore null terminator
    int letterIndex = getLetterIndex(message[i]);
    displayLetter(letterIndex);
    delay(500);  // hold each letter for half a second

    // Insert space only if current letter is 'L' and next letter is also 'L'
    if (message[i] == 'L' && i + 1 < sizeof(message) - 1 && message[i + 1] == 'L') {
      int spaceIndex = getLetterIndex(' ');
      displayLetter(spaceIndex);
      delay(80);  // hold space to separate double L's
    }
  }
   // Run fun random colors animation 50 cycles, 100ms delay between frames
  funRandomColorsAnimation(50, 100);
}

void funRandomColorsAnimation(int cycles, int delayMs) {
  for (int i = 0; i < cycles; i++) {
    for (int pixel = 0; pixel < NUM_LEDS; pixel++) {
      // Set each pixel to a random color
      uint8_t r = random(0, 50);
      uint8_t g = random(0, 50);
      uint8_t b = random(0, 50);
      matrix.setPixelColor(pixel, matrix.Color(r, g, b));
    }
    matrix.show();
    delay(delayMs);
  }
  matrix.clear();
  matrix.show();
}

