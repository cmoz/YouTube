#pragma once
#include <Adafruit_GFX.h>

// 16-bit colour from red, green, blue (0-255)
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void catUpdate();                              // call every frame
void catDraw(GFXcanvas16 &c);
void catTickle();                              // jump + hearts. Lots of quick tickles = zoomies!
void catHappy();                               // a happy hop (good news)
void catSleep();                               // curl up for the night
void catWake();
bool catSleeping();
void catSay(const char *text, uint32_t ms = 2000);   // little speech bubble
void catSetNight(bool night);                  // moon in the window instead of the sun
void catSetWeather(int wmoCode, float tempC);  // real weather outside the window
void catConfetti(bool on);
