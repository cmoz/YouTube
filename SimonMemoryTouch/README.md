# Wearable Simon Says 🎮

> A memory game you wear: five conductive fabric touch pads and five WS2812B LEDs on an Adafruit QT Py ESP32-S3, playing the classic "watch the sequence, repeat it back" game using nothing but the chip's built-in capacitive touch sensing — no extra touch IC required.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

Power it on and the built-in NeoPixel breathes blue while a rainbow fade plays across the five external LEDs — keep your hands off the pads, because each one calibrates itself right after the rainbow. Touch pad **A2** to start. Each round adds one more random colour to the sequence, flashes it out at one second per LED, then waits for you to touch the pads back in the same order — each pad's LED lights the moment your touch registers. Get it right and the sequence grows by one; get it wrong (or time out) and the game ends with a red flash, a rainbow flutter, and your score (rounds completed) displayed in **binary** across the five LEDs before it resets and waits for A2 again. Remember all 31 and you win!

**2026 update:** touch is now much more reliable on fabric — per-pad calibration instead of one fixed threshold, the strongest touch wins when pads nudge each other, instant LED feedback, more time between touches, and a binary score that reads the right way round on the wrist.

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

At power-on each pad measures its own resting value with `touchRead()`. A touch counts when a pad rises `TOUCH_PERCENT` (20%) above that, and the resting value slowly follows changes in how the fabric is worn. If several pads read high at once (touching one fabric pad can nudge the others), the one that rose the most wins.

Open the Serial Monitor at 115200 baud and use:

- `t` — show/hide live readings. Each pad shows as a % of its touch threshold — **100% = touch**, and `*` marks the pad being counted
- `c` — re-calibrate the pads (hands off the fabric!)

The settings are at the top of `src/main.cpp`:

- Lower `TOUCH_PERCENT` (e.g. 10) if a light touch doesn't reach 100%
- Raise it if pads trigger on their own
- `TURN_TIMEOUT_MS` sets how long you get between touches (6 seconds)

## Notes & gotchas

- **`editedForDigital.cpp` and `workingTouchSensorReadings.cpp`** are earlier experiments (a 7-pad version, and a calibration-only sketch) kept for reference — only `src/main.cpp` is part of the active PlatformIO build.
- **Score is shown in binary**, not as a number count of flashes — LED 0 is the least significant bit, so on the wrist it reads like normal binary: `00011` = 3, `00101` = 5.
- **Holding a pad doesn't use up your time** — the 6-second timer only counts the gap between touches, and resets after each correct one.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Add more pads for a longer sequence, swap the colour palette for something that matches your outfit, or combine it with the [Touch Strip Hold & Release](../touch_strip_hold_release) project's calibration technique for more reliable fabric touch. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
