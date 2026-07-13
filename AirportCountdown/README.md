# Airport Challenge Timer ⏱️

> A touch-set countdown timer for the classic "beat the clock through security" airport game: two capacitive touch pads on an Adafruit QT Py ESP32-S2 set the time, an OLED shows what's left, and a 4-pixel LED strip shifts from a calm pulse to a frantic red flash as your deadline closes in.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor Kit for this project](https://www.tinkertailor.ca/products/flight-glove-video-kit-everything-used-in-the-build))

---

## What it does

At idle, the OLED shows on-screen instructions and the LEDs cycle a slow rainbow. **Tap** the left pad to add 1 minute, the right pad to add 10 — the LEDs light up in blocks of blue (one LED per 15 minutes set) so you can see your target time build without reading the screen. **Hold** either pad for a second to start the countdown. **Touch both pads at once** at any time to cancel and reset to zero.

While running, the OLED counts down in `MM:SS` and the LEDs signal urgency automatically:

| Time remaining | LED behaviour |
|-----------------|----------------|
| > 40 min | Slow, gentle green-cyan pulse |
| 20–40 min | Solid green |
| 10–20 min | Solid orange |
| < 10 min | Flashing red |

The whole board also power-saves: after 30 seconds with no touch input, LED brightness drops automatically until you interact again.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Adafruit QT Py ESP32-S2 | Native USB, built-in capacitive touch |
| 1 | 0.96" SSD1306 OLED (128×64) | I2C |
| 1 | WS2812B LED strip (4 LEDs) | Data on pin A1 |
| 2 | Conductive fabric or metal touch pads | Wired to GPIO 2 and GPIO 3 |

## Wiring

| QT Py pin | Connects to |
|-----------|-------------|
| GPIO 7 (SDA) | OLED SDA |
| GPIO 6 (SCL) | OLED SCL |
| A1 | WS2812B data in |
| GPIO 2 | Touch pad 1 — taps add 1 minute |
| GPIO 3 | Touch pad 2 — taps add 10 minutes |

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit SSD1306** + **Adafruit GFX Library** + **Adafruit BusIO** — OLED display
- **FastLED** — drives the WS2812B strip and its animations

## Upload

Open this folder in PlatformIO (VS Code), plug in the QT Py over USB-C, and hit Upload — the `adafruit_qtpy_esp32s2` environment is preconfigured with native USB CDC enabled so Serial works without a reset. Open the Serial Monitor at 115200 baud to see touch baselines at startup, useful for calibration.

## Calibrating the touch pads

Both pads share one threshold each, set in `main.cpp`:

```cpp
const int TOUCH_THRESHOLD_1 = 100000;
const int TOUCH_THRESHOLD_2 = 100000;
```

On boot, the Serial Monitor prints each pad's untouched baseline reading. If a pad won't register touches or triggers on its own, adjust its threshold to sit between the untouched baseline and a touched reading — the code treats a reading *higher* than the threshold as a touch.

## Notes & gotchas

- **This runs on the QT Py's native capacitive touch**, not an external touch IC — pad size and material affect sensitivity, so recalibrate if you change either.
- **A hold is detected after 1 second** (`HOLD_THRESHOLD`) — a quick tap always adds time, never starts the timer, so you won't accidentally start with 0 minutes set.
- **Cancelling blocks in a `while` loop** until both pads are released, to avoid re-triggering — this is intentional, not a hang.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Retheme it for any race-the-clock game — a scavenger hunt, an escape room, a chores timer for kids. Swap the urgency colours, add a buzzer for the final ten seconds, or extend `MAX_MINUTES` for longer countdowns. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
