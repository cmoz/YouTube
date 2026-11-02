#pragma once
#include <Arduino.h>

// ══════════════════ CHANGE THESE ══════════════════

// --- Display: Adafruit 1.44" 128x128 TFT (ST7735R) ---
//   Vin -> 3V3   GND -> GND   SCK -> 18   SI (MOSI) -> 23
//   TCS -> 5     D/C -> 21    RST -> 4    Lite -> 22
//   (SO and CCS are for the SD card - leave them unconnected)
#define TFT_CS    5
#define TFT_DC    21
#define TFT_RST   4
#define TFT_LITE  22   // backlight, dims while Buddy sleeps. -1 if Lite isn't wired
#define SCREEN_ROTATION 0   // 0-3: turn the picture if the screen is mounted sideways

// --- Fabric touch buttons (must be ESP32 touch pins) ---
#define TOUCH_TICKLE  32   // tickle the face
#define TOUCH_STOCKS  33   // stock prices (hold for the phone setup address)
#define TOUCH_VIBES   27   // good vibes quotes

// Time zone for the clock, bedtime and birthday - Atlantic (Nova Scotia). Others:
//   Eastern "EST5EDT,M3.2.0,M11.1.0"  Central "CST6CDT,M3.2.0,M11.1.0"
//   Mountain "MST7MDT,M3.2.0,M11.1.0"  Pacific "PST8PDT,M3.2.0,M11.1.0"
#define TIMEZONE "AST4ADT,M3.2.0,M11.1.0"

// Where the window's weather comes from (Open-Meteo, free, no account) - Halifax, NS.
// For another town, look up its latitude/longitude on a map
const float WEATHER_LAT = 44.65;
const float WEATHER_LON = -63.57;
const uint32_t WEATHER_REFRESH_MS = 15 * 60 * 1000UL;

const int BIRTHDAY_MONTH = 11;   // e.g. 11 for November - confetti all day! 0 = off
const int BIRTHDAY_DAY   = 4;

// Secret Easter egg: 3 quick tickles, then touch and HOLD the 4th
#define SECRET_MESSAGE "You found the secret! Love you lots xx"

const int SLEEP_HOUR = 23;      // Buddy gets sleepy from this hour...
const int WAKE_HOUR  = 7;       // ...until this one

const uint32_t STOCK_REFRESH_MS = 2 * 60 * 1000UL;   // how often prices update
const float    BIG_MOVE_PERCENT = 3.0;               // flash when a stock moves this much in a day

// Touch tuning - each pad calibrates itself at power-on (hands off!)
const int TOUCH_PERCENT  = 30;  // touch = reading drops this % below the pad's resting value
const int TOUCH_MIN_DROP = 6;   // ...and by at least this much
const uint32_t SCREEN_TIMEOUT_MS = 30000;  // info screens go back to the face after this

// ══════════════════════════════════════════════════
