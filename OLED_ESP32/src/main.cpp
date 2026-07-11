#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float wavePhase = 0;
int flashCounter = 0;
bool flashActive = false;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void drawStarburst(int centerX, int centerY, int radius, int lines) {
  for (int i = 0; i < lines; i++) {
    float angle = i * (2 * PI / lines);
    int x = centerX + radius * cos(angle);
    int y = centerY + radius * sin(angle);
    display.drawLine(centerX, centerY, x, y, WHITE);
  }
}

void drawWave(float phase) {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    int y = SCREEN_HEIGHT / 2 + 10 * sin((x + phase) * 0.1);
    display.drawPixel(x, y, WHITE);
  }
}

void drawFlashTransition() {
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.display();
  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  display.println("Tinker\nTailor");
  display.display();
  delay(1000);
}

void loop() {
  display.clearDisplay();

  // Starburst pulse
  int radius = 10 + abs(sin(millis() * 0.002)) * 20;
  drawStarburst(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, radius, 12);

  // Wave animation
  drawWave(wavePhase);
  wavePhase += 2;

  display.display();
  delay(50);

  // Flash transition every 10 seconds
  if (millis() > flashCounter + 10000) {
    drawFlashTransition();
    flashCounter = millis();
  }
}