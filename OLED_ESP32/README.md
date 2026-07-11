# OLED Starburst & Phone Messenger 📟

> A 128×64 SSD1306 OLED on an ESP32 driving a generative starburst-and-wave animation that flashes to a branded splash screen every ten seconds — plus a bonus alternate sketch that turns the same screen into a tiny WiFi message board you can post to from your phone.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## Two sketches, pick your flavour

- **`src/main.cpp`** (default build) — the generative animation: a pulsing starburst with a sine wave scrolling behind it, flashing to a "Tinker Tailor" splash every 10 seconds.
- **`test/mainPhone.cpp`** — a WiFi access-point version: the ESP32 hosts its own hotspot and a tiny web page where you can type a message from your phone and have it appear instantly on the OLED.

PlatformIO only compiles `src/`, so to try the phone messenger, swap it in: rename or move `src/main.cpp` aside, then copy `test/mainPhone.cpp` into `src/main.cpp` and rebuild.

## What it does

**Starburst mode:** twelve lines radiate from the screen's centre, their length pulsing with a sine wave, while a second sine wave scrolls horizontally in the background. Every 10 seconds the whole screen flashes white, then settles on a brief splash reading "Tinker Tailor" before returning to the animation.

**Phone messenger mode:** on boot the ESP32 spins up its own WiFi hotspot and shows the network name and IP address on the OLED. Connect from your phone, open the IP in a browser, type a message, hit Send — and it appears on the screen immediately, no app required.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32 Dev Module (30-pin) | The standard DevKit |
| 1 | 0.96" SSD1306 OLED (128×64) | I2C |

## Wiring

| OLED pin | ESP32 pin |
|----------|-----------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC / GND | 3.3V / GND |

`src/main.cpp` uses the ESP32's default I2C pins implicitly; `test/mainPhone.cpp` sets them explicitly with `Wire.begin(21, 22)` — same wiring either way.

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit SSD1306** + **Adafruit GFX Library** — OLED display driver and drawing primitives
- **WiFi** + **WebServer** — built into the ESP32 Arduino core, only needed for the phone messenger variant

## Before you upload the phone messenger

Open `test/mainPhone.cpp` (once copied into `src/`) and set your own hotspot credentials:

```cpp
const char* ssid = "YOURNETWORKSSID";
const char* password = "PASSWORD";
```

## Upload

Open this folder in PlatformIO (VS Code), plug in the ESP32, select the `esp32dev` environment, and hit Upload.

## Notes & gotchas

- **The animation runs entirely on the OLED's own draw calls** — no external library beyond Adafruit GFX, so it's a good starting point for learning `drawLine`/`drawPixel`-based effects.
- **The phone messenger's web page has no authentication** — anyone connected to the hotspot can post a message. Fine for a party prop, not for anything sensitive.
- **Message length is capped at 30 characters** (`maxlength="30"` in the HTML form) to keep it from overflowing the tiny screen.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Swap the starburst for your own generative pattern, change the splash text to your own branding, or build out the phone messenger with a message queue so multiple guests can leave notes in sequence. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
