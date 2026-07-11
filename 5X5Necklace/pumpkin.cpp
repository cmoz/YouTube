#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    8
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

Adafruit_NeoPixel matrix(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 5x5 pixel font definitions for required letters and space:
uint8_t letters[][5][5] = {
  // I (0)
  {
    {0,1,1,1,0},
    {0,0,1,0,0},
    {0,0,1,0,0},
    {0,0,1,0,0},
    {0,1,1,1,0}
  },
  // L (1)
  {
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,0,0,0,0},
    {1,1,1,1,1}
  },
  // O (2)
  {
    {0,1,1,1,0},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {0,1,1,1,0}
  },
  // V (3)
  {
    {1,0,0,0,1},
    {1,0,0,0,1},
    {0,1,0,1,0},
    {0,1,0,1,0},
    {0,0,1,0,0}
  },
  // E (4)
  {
    {1,1,1,1,1},
    {1,0,0,0,0},
    {1,1,1,1,0},
    {1,0,0,0,0},
    {1,1,1,1,1}
  },
  // Y (5)
  {
    {1,0,0,0,1},
    {0,1,0,1,0},
    {0,0,1,0,0},
    {0,0,1,0,0},
    {0,0,1,0,0}
  },
  // U (6)
  {
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {0,1,1,1,0}
  },
  // P (7)
  {
    {1,1,1,1,0},
    {1,0,0,0,1},
    {1,1,1,1,0},
    {1,0,0,0,0},
    {1,0,0,0,0}
  },
  // Letter M (add this to your letters array)
  {
  {1,0,0,0,1},
  {1,1,0,1,1},
  {1,0,1,0,1},
  {1,0,0,0,1},
  {1,0,0,0,1}
  },
  // K (8)
  {
    {1,0,0,0,1},
    {1,0,0,1,0},
    {1,1,1,0,0},
    {1,0,0,1,0},
    {1,0,0,0,1}
  },
  // N (9)
  {
    {1,0,0,0,1},
    {1,1,0,0,1},
    {1,0,1,0,1},
    {1,0,0,1,1},
    {1,0,0,0,1}
  },
  // Space (10)
  {
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0},
    {0,0,0,0,0}
  }
};

// Map characters to the letters index above
int getLetterIndex(char c) {
  switch (c) {
    case 'I': return 0;
    case 'L': return 1;
    case 'O': return 2;
    case 'V': return 3;
    case 'E': return 4;
    case 'Y': return 5;
    case 'U': return 6;
    case 'P': return 7;
    case 'M': return 8;
    case 'K': return 9;
    case 'N': return 10;
    case ' ': return 11;
  }
  return 10; // Default to space for unknown chars
}

const char message[] = "I LOVE U PUMPKIN  ";

void setup() {
  matrix.begin();
  matrix.setBrightness(40);  // ~25% brightness
  matrix.clear();
  matrix.show();
}

void displayLetter(int index, bool isRed) {
  matrix.clear();
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (letters[index][y][x]) {
        int pixel = y * MATRIX_WIDTH + x;  // row-major mapping
        if (isRed) {
          matrix.setPixelColor(pixel, matrix.Color(255, 0, 0));  // red
        } else {
          matrix.setPixelColor(pixel, matrix.Color(0, 128, 128));  // teal
        }
      }
    }
  }
  matrix.show();
}

void loop() {
  int length = sizeof(message) - 1;  // exclude null terminator
  for (int i = 0; i < length; i++) {
    int letterIndex = getLetterIndex(message[i]);
    
    // Determine if current letter is in the word LOVE 
    bool isRed = (message[i] == 'L' || message[i] == 'O' || message[i] == 'V' || message[i] == 'E');
    
    displayLetter(letterIndex, isRed);
    delay(500);
    
    // Insert space after double L if needed (as before)
    if (message[i] == 'L' && i + 1 < length && message[i + 1] == 'L') {
      displayLetter(getLetterIndex(' '), false);
      delay(500);
    }
  }
}
