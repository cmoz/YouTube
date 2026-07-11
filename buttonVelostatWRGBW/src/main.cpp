#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// OLED settings
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// I2C pins
#define I2C_SDA 5
#define I2C_SCL 4

// LED strip settings
#define LED_PIN    3
#define NUM_LEDS   75
#define BRIGHTNESS 128
#define FILL_TIME  700  // ms

#ifndef NEO_BWGR
  #define NEO_BWGR  ((3<<NEO_RSHIFT) | (2<<NEO_GSHIFT) | (0<<NEO_BSHIFT) | (1<<NEO_WSHIFT))
#endif

// Use NEO_RGBW for Red-Green-Blue-White order
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BWGR + NEO_KHZ800);

// Velostat
int  myPin        = 1;       // GPIO2
bool touching     = false;
unsigned long touchStart = 0;

// Palette engine
enum BlendType { NOBLEND, LINEARBLEND };
uint32_t RainbowColors[16];
uint32_t RedWhiteBluePalette[16];
uint32_t *currentPalette;
BlendType currentBlending;

// Forward declarations
void displayMessage(const String &msg);
uint32_t ColorFromPalette(uint32_t *pal, uint8_t idx, uint8_t bright, BlendType blend);
void FillLEDsFromPaletteColors(uint8_t ci);
void ChangePalettePeriodically();
void fireEffect();
void colorPulseTunnel();
void lightningFlicker();
void iceFlow();
void lavaLampBlobs();

void setup() {
  Serial.begin(115200);
  delay(500);

  // OLED init
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Hello!");
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println("ESP32-C3 OLED");
  display.display();
  delay(2000);

  pinMode(myPin, INPUT_PULLDOWN);

  // NeoPixel init
  strip.begin();
  strip.setBrightness(BRIGHTNESS);


strip.begin();
strip.setBrightness(BRIGHTNESS);

strip.fill(strip.Color(0, 0, 255, 0));  // Blue on wire0
strip.show(); delay(500);
strip.fill(strip.Color(0, 0, 0, 255));  // White on wire1
strip.show(); delay(500);
strip.fill(strip.Color(0, 255, 0, 0));  // Green on wire2
strip.show(); delay(500);
strip.fill(strip.Color(255, 0, 0, 0));  // Red on wire3
strip.show(); delay(500);

strip.clear();
strip.show();

delay(500);

  // Channel‐test (uncomment to verify order)
  strip.fill(strip.Color(255,0,0,0)); strip.show(); delay(500); //r
  strip.fill(strip.Color(0,255,0,0)); strip.show(); delay(500); //g
  strip.fill(strip.Color(0,0,255,0)); strip.show(); delay(500); // b
  strip.fill(strip.Color(0,0,0,255)); strip.show(); delay(500); // w

  strip.clear();  
  strip.show();
  Serial.println("LED setup complete");

  // Build 16‐entry rainbow palette
  for (uint8_t i = 0; i < 16; i++) {
    RainbowColors[i] = strip.ColorHSV(uint32_t(i) * 65535 / 16, 255, 255);
  }

  // Build red-gray-blue-black striped palette
  for (uint8_t i = 0; i < 16; i++) {
    switch (i % 4) {
      case 0: RedWhiteBluePalette[i] = strip.Color(255,   0,   0,   0); break; // red
      case 1: RedWhiteBluePalette[i] = strip.Color(128, 128, 128,   0); break; // gray
      case 2: RedWhiteBluePalette[i] = strip.Color(  0,   0, 255,   0); break; // blue
      default:RedWhiteBluePalette[i] = strip.Color(  0,   0,   0,   0); break; // black
    }
  }

  currentPalette  = RainbowColors;
  currentBlending = LINEARBLEND;
}

void loop() {
  static uint8_t paletteStartIndex = 0;
  int sensorValue = analogRead(myPin);
  Serial.print(sensorValue); Serial.print(" - ");

  if (sensorValue > 1800) {
    if (!touching) touchStart = millis();
    touching = true;

    unsigned long holdTime = millis() - touchStart;
    float progress = float(holdTime) / FILL_TIME;
    progress = constrain(progress, 0.0, 1.0);
    int ledsToLight = progress * NUM_LEDS;

if (progress < 1.0) {
  // Progressive HSV fill
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < ledsToLight) {
      uint32_t c = strip.ColorHSV((i * 65535) / NUM_LEDS, 255, BRIGHTNESS);
      strip.setPixelColor(i, c);
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
} else {
  // Custom effects based on hold time
  unsigned long holdTime = millis() - touchStart;
  if (holdTime >= 5000 && holdTime < 10000) {
    colorPulseTunnel();
  } else if (holdTime < 18000) {
    fireEffect();
  } else if (holdTime < 24000) {
    lavaLampBlobs();
  } else if (holdTime < 30000) {
    iceFlow();
  }
}

    // OLED messages
    if      (holdTime <  500) displayMessage("Tap\n...or keep holding");
    else if (holdTime < 3000) displayMessage("Holding");
    else if (holdTime < 8000) displayMessage("...Still holding!");
    else                      displayMessage("Phew!\n\nThat was a long hold!");
  }
  else {
    // Not touching
    touching = false;
    strip.clear();
    strip.show();
    displayMessage("Tap for light");
  }

  delay(20);
}

uint32_t ColorFromPalette(uint32_t *pal, uint8_t index, uint8_t bright, BlendType blend) {
  uint8_t idx  = index >> 4;
  uint8_t frac = index & 0x0F;
  uint32_t c0 = pal[idx], c1 = pal[(idx+1)&0x0F];

  // Unpack W,R,G,B
  uint8_t w0 = c0 >> 24, r0 = (c0>>16)&0xFF, g0 = (c0>>8)&0xFF, b0 = c0&0xFF;
  uint8_t w1 = c1 >> 24, r1 = (c1>>16)&0xFF, g1 = (c1>>8)&0xFF, b1 = c1&0xFF;

  uint16_t b0fac = 16 - frac, b1fac = frac;
  uint8_t w = blend==LINEARBLEND ? (w0*b0fac + w1*b1fac)/16 : w0;
  uint8_t r = blend==LINEARBLEND ? (r0*b0fac + r1*b1fac)/16 : r0;
  uint8_t g = blend==LINEARBLEND ? (g0*b0fac + g1*b1fac)/16 : g0;
  uint8_t b = blend==LINEARBLEND ? (b0*b0fac + b1*b1fac)/16 : b0;

  // Apply brightness
  w = (uint16_t)w * bright / 255;
  r = (uint16_t)r * bright / 255;
  g = (uint16_t)g * bright / 255;
  b = (uint16_t)b * bright / 255;

  return strip.Color(r, g, b, w);
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, ColorFromPalette(currentPalette, colorIndex, 255, currentBlending));
    colorIndex += 3;
  }
}

void ChangePalettePeriodically() {
  static unsigned long last = 0;
  if (millis() - last > 10000) {
    last = millis();
    if (currentPalette == RainbowColors) {
      currentPalette  = RedWhiteBluePalette;
      currentBlending = NOBLEND;
    } else {
      currentPalette  = RainbowColors;
      currentBlending = LINEARBLEND;
    }
  }
}

void displayMessage(const String &msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("   *Tappy Rainbow*");
  display.setCursor(0,18);
  display.println(msg);
  display.display();
  delay(50);
}


void fireEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t heat = random(180, 255);
    uint32_t color = strip.ColorHSV(random(0, 5000), 255, heat); // reddish hue
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void colorPulseTunnel() {
  static uint8_t phase = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t brightness = abs((i + phase) % (NUM_LEDS * 2) - NUM_LEDS);
    uint32_t color = strip.ColorHSV((i * 65535) / NUM_LEDS, 255, brightness);
    strip.setPixelColor(i, color);
  }
  strip.show();
  phase++;
}

void lightningFlicker() {
  if (random(20) == 0) {
    uint32_t flashColor = strip.Color(random(100,255), random(100,255), 255, random(50));
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, flashColor);
    }
    strip.show();
    delay(random(50,150));
    strip.clear();
    strip.show();
  }
}

void iceFlow() {
  static uint8_t offset = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (16000 + offset + i * 5) % 65536; // icy blue range
    uint32_t color = strip.ColorHSV(hue, 180, BRIGHTNESS);
    strip.setPixelColor(i, color);
  }
  strip.show();
  offset += 2;
}

void lavaLampBlobs() {
  static float t = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    float wave = sin((i * 0.2) + t) + sin((i * 0.1) - t * 1.5);
    uint8_t brightness = constrain((wave + 2.0) * 64, 0, 255);
    uint32_t color = strip.ColorHSV((i * 65535) / NUM_LEDS, 180, brightness);
    strip.setPixelColor(i, color);
  }
  strip.show();
  t += 0.05;
}