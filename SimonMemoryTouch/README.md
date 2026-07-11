# Wearable Simon Says 🎮

> A memory game you wear: five conductive fabric touch pads and five WS2812B LEDs on an Adafruit QT Py ESP32-S3, playing the classic "watch the sequence, repeat it back" game using nothing but the chip's built-in capacitive touch sensing — no extra touch IC required.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

Power it on and the built-in NeoPixel breathes blue while a rainbow fade plays across the five external LEDs — touch pad **A2** to start. Each round adds one more random colour to the sequence, flashes it out at one second per LED, then waits for you to touch the pads back in the same order. Get it right and the sequence grows by one; get it wrong (or time out) and the game ends with a red flash, a rainbow flutter, and your final score displayed in **binary** across the five LEDs before it resets and waits for A2 again.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Adafruit QT Py ESP32-S3 (N4R2) | Uses the chip's native capacitive touch — no touch sensor IC |
| 1 | WS2812B LED strip/ring (5 LEDs) | Data on pin A1 |
| 5 | Conductive fabric touch pads | Sewn with conductive thread to A2, A3, SDA, SCL, TX |

## Wiring

| QT Py pin | Connects to |
|-----------|-------------|
| A1 | WS2812B data in |
| A2 | Touch pad 0 (pink/magenta, also the "start game" pad) |
| A3 | Touch pad 1 (cyan) |
| SDA | Touch pad 2 (yellow) |
| SCL | Touch pad 3 (green) |
| TX | Touch pad 4 (orange) |

The QT Py's onboard NeoPixel (pin 39) is used automatically as a blue "breathing" status light while the game waits for input — no wiring needed for that one.

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit NeoPixel** — drives both the external strip and the onboard status pixel

## Upload

Open this folder in PlatformIO (VS Code), plug in the QT Py over USB-C, and hit Upload — the `adafruit_qtpy_esp32s3` environment is preconfigured. Open the Serial Monitor at 115200 baud to watch touch readings and round-by-round scoring.

## Tuning the touch sensitivity

Each pad is read with `touchRead()` and compared against a single fixed threshold (`jumpThreshold = 20000` in `src/main.cpp`). If pads trigger too easily or not at all:

- Raise `jumpThreshold` if pads register touches on their own (too sensitive)
- Lower it if you have to press hard to register a touch
- `workingTouchSensorReadings.cpp` in this folder is a standalone calibration sketch that logs live readings per pad over Serial — swap it into `src/` temporarily to find good baseline numbers for your fabric before tuning the main sketch

## Notes & gotchas

- **`editedForDigital.cpp` and `workingTouchSensorReadings.cpp`** are earlier experiments (a 7-pad version, and a calibration-only sketch) kept for reference — only `src/main.cpp` is part of the active PlatformIO build.
- **Score is shown in binary**, not as a number count of flashes — LED 0 is the least significant bit. A score of 5, for example, lights LEDs for `101`.
- **Timeout scales with progress:** you get 3 seconds per pad in the current sequence, reset after each correct touch, so later rounds aren't unfairly rushed.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Add more pads for a longer sequence, swap the colour palette for something that matches your outfit, or combine it with the [Touch Strip Hold & Release](../touch_strip_hold_release) project's calibration technique for more reliable fabric touch. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
