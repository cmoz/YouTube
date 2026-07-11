# Tappy Rainbow Velostat Button — RGBW Effects Edition 👜🔥

> The evolved sibling of the [ButtonVelostat](../ButtonVelostat) handbag button: same Velostat pressure sensor and OLED narration, but rewired for RGBW (SK6812-style) LEDs with a proper custom colour order, a much faster charge-up, and a chain of hold-triggered effects — fire, an ice flow, lava lamp blobs, and a pulsing tunnel — that cycle the longer you keep holding.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

Press the Velostat button and the 75-pixel strip fills with a rainbow gradient in well under a second (700 ms, versus 5 seconds in the original ButtonVelostat). Keep holding and the strip cycles through a sequence of generative effects instead of just sitting on rainbow:

| Hold time | Effect |
|-----------|--------|
| 0–700 ms | Progressive rainbow fill |
| 5–10 s | Colour pulse tunnel |
| 10–18 s | Fire |
| 18–24 s | Lava lamp blobs |
| 24–30 s | Ice flow |

The OLED narrates alongside it, same as the original: *Tap...or keep holding* → *Holding* → *...Still holding!* → *Phew! That was a long hold!* Release and everything clears back to **"Tap for light."**

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32-C3 Dev Module (DevKitM-1) | |
| 1 | 0.96" SSD1306 OLED (128×64) | I2C |
| 1 | SK6812 RGBW LED strip (75 LEDs) | Data on GPIO 3 — see colour order note below |
| 1 | Velostat pressure sensor "button" | Sandwiched between two conductive fabric pads, read as an analog voltage divider on GPIO 1 |

## Wiring

| ESP32-C3 pin | Connects to |
|--------------|-------------|
| GPIO 5 | OLED SDA |
| GPIO 4 | OLED SCL |
| GPIO 3 | LED strip data in |
| GPIO 1 | Velostat sensor (pulled down internally; press raises the reading) |

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit NeoPixel** — drives the RGBW strip (this version uses NeoPixel instead of FastLED, for direct control over the channel order)
- **Adafruit SSD1306** + **Adafruit GFX Library** — OLED display

## Getting the RGBW colour order right

This strip needs a custom channel order, defined at the top of `main.cpp`:

```cpp
#define NEO_BWGR ((3<<NEO_RSHIFT) | (2<<NEO_GSHIFT) | (0<<NEO_BSHIFT) | (1<<NEO_WSHIFT))
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_BWGR + NEO_KHZ800);
```

On startup the sketch runs a channel test — flashing blue, white, green, then red across the whole strip — so you can visually confirm the order is correct before the button logic starts. If colours come out wrong on your strip, this is the constant to adjust.

## Upload

Open this folder in PlatformIO (VS Code), plug in the ESP32-C3, select the `esp32-c3-devkitm-1` environment, and hit Upload. Open the Serial Monitor at 115200 baud to watch raw Velostat readings for calibration.

## Calibrating the Velostat sensor

Same approach as the original ButtonVelostat: watch raw readings on Serial while pressing, then adjust the threshold in `main.cpp` to sit between resting and pressed values:

```cpp
if (sensorValue > 1800) {
```

## Notes & gotchas

- **`lightningFlicker()` is defined but not currently wired into the hold-time chain** — it's there to drop into the `else if` ladder in `loop()` if you want a fifth effect.
- **This strip is RGBW, not plain RGB** — plugging in a standard WS2812B strip here will need the colour order (and the `Adafruit_NeoPixel` constructor flags) changed back to a 3-channel order.
- **The palette engine (`ColorFromPalette`, `RainbowColors`, `RedWhiteBluePalette`) is built but not currently called from `loop()`** — it's scaffolding for adding palette-based effects alongside the procedural ones.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Wire `lightningFlicker()` into the hold-time chain, add your own generative effect function alongside `fireEffect()`/`iceFlow()`/`lavaLampBlobs()`, or hook the unused palette engine up to a mode you can cycle with a second tap. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
