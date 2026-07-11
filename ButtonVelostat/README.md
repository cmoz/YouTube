# Tappy Rainbow Velostat Button 👜

> A soft-circuit pressure button made from Velostat, mounted on a handbag, that fills a 75-pixel LED strip one pixel at a time the longer you hold it — reach full brightness and it breaks into a continuous rainbow chase, while an OLED narrates your commitment level from "Tap..." to "Phew! That was a long hold!"

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

The OLED idles on **"Tap for light."** Press the Velostat button and the LED strip starts filling with a rainbow gradient, one pixel further for every fraction of the 5-second fill time — a satisfying analog "charge up" rather than an instant on/off. Keep holding past the 5 seconds and, once the strip is full, it switches to a continuously animated rainbow chase instead of just sitting lit. The OLED updates in step with how long you've been holding: *Tap... or keep holding* → *Holding* → *...Still holding!* → *Phew! That was a long hold!* Release, and everything goes dark and resets.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32-C3 Dev Module (DevKitM-1) | Native USB CDC |
| 1 | 0.96" SSD1306 OLED (128×64) | I2C |
| 1 | SK6812 RGBW LED strip (75 LEDs) | Data on GPIO 3 |
| 1 | Velostat pressure sensor "button" | Sandwiched between two conductive fabric pads, read as an analog voltage divider on GPIO 1 |

## Wiring

| ESP32-C3 pin | Connects to |
|--------------|-------------|
| GPIO 5 | OLED SDA |
| GPIO 4 | OLED SCL |
| GPIO 3 | LED strip data in |
| GPIO 1 | Velostat sensor (pulled down internally; press raises the reading) |

Build the Velostat button as a simple pressure sensor: a square of Velostat between two conductive fabric or copper tape pads, wired as one leg of a voltage divider into GPIO 1.

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit SSD1306** + **Adafruit GFX Library** — OLED display
- **FastLED** — drives the SK6812 RGBW strip

## Upload

Open this folder in PlatformIO (VS Code), plug in the ESP32-C3, select the `esp32-c3-devkitm-1` environment, and hit Upload — native USB CDC is preconfigured so Serial works without extra setup. Open the Serial Monitor at 115200 baud to watch raw sensor readings, which is the fastest way to calibrate your own Velostat button.

## Calibrating the Velostat sensor

Velostat's resistance varies a lot by brand, thickness, and how it's sandwiched. Watch the Serial Monitor while pressing your button and adjust this line in `main.cpp` to sit between your resting and pressed readings:

```cpp
if (sensorValue > 1800) {
```

## Notes & gotchas

- **`other.cpp` and `otherCode.txt`** in this folder are unrelated FastLED reference snippets (a color palette demo) kept for reference — only `src/main.cpp` is part of the active PlatformIO build.
- **This strip is RGBW (SK6812), not plain RGB (WS2812B)** — if you swap in a different strip, update both `LED_TYPE` and `COLOR_ORDER` in `main.cpp` or colours will come out wrong.
- **Velostat is pressure-sensitive, not touch-sensitive** — it needs to be physically squeezed, which is what makes it a good fit for a handbag button that won't false-trigger from nearby fabric.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Retune `FILL_TIME` for a faster or slower charge-up, swap the rainbow for a colour that matches your bag, or use the same Velostat-button-plus-progressive-fill pattern to build a squeeze-to-reveal secret pocket light. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
