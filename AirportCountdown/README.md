# Airport Challenge Timer ⏱️

> A touch-set countdown timer for the classic "beat the clock through security" airport game: two capacitive touch pads on an Adafruit QT Py ESP32-S2 set the time, an OLED shows what's left, and a 4-pixel LED strip shifts from a calm pulse to a frantic red flash as your deadline closes in.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor Kit for this project](https://www.tinkertailor.ca/products/flight-glove-video-kit-everything-used-in-the-build))

---

## What it does

At power-on each touch pad calibrates itself — keep your hands off for a second. At idle, the OLED shows on-screen instructions and the LEDs cycle a slow rainbow. **Tap** the A2 pad to add 1 minute, the A3 pad to add 10 — while you touch a pad, its half of the LED strip glows white so you know the touch registered, and the LEDs light up in blocks of blue (one LED per 15 minutes set) so you can see your target time build without reading the screen. **Hold** either pad for a second to start the countdown. **Touch both pads together** at any time to cancel and reset to zero.

When the countdown hits zero, the OLED flashes **TIME'S UP!** and the LEDs flash red and white until you touch a pad (or for 30 seconds).

**2026 update:** touch now works reliably — the pads moved to A2/A3, each pad calibrates itself instead of using one fixed threshold, touches light up instantly, cancel no longer adds time back when you let go, and there's a "time's up" alert.

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
| 2 | Conductive fabric or metal touch pads | Wired to A2 and A3 |

## Wiring

| QT Py pin | Connects to |
|-----------|-------------|
| GPIO 7 (SDA) | OLED SDA |
| GPIO 6 (SCL) | OLED SCL |
| A1 | WS2812B data in |
| A2 (GPIO 9) | Touch pad 1 — taps add 1 minute |
| A3 (GPIO 8) | Touch pad 2 — taps add 10 minutes |

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit SSD1306** + **Adafruit GFX Library** + **Adafruit BusIO** — OLED display
- **FastLED** — drives the WS2812B strip and its animations

## Upload

Open this folder in PlatformIO (VS Code), plug in the QT Py over USB-C, and hit Upload — the `adafruit_qtpy_esp32s2` environment is preconfigured with native USB CDC enabled so Serial works without a reset. Open the Serial Monitor at 115200 baud to see each pad's resting value at startup, plus taps, starts and cancels as they happen.

## Tuning the touch pads

At power-on each pad measures its own resting value with `touchRead()`. A touch counts when a pad rises `TOUCH_PERCENT` (20%) above that, and the resting value slowly follows changes in how the pads are worn.

Open the Serial Monitor at 115200 baud and use:

- `t` — show/hide live readings. Each pad shows as a % of its touch threshold — **100% = touch**, and `*` marks a pad that's being touched
- `c` — re-calibrate the pads (hands off!)

The settings are at the top of `src/main.cpp`:

- Lower `TOUCH_PERCENT` (e.g. 10) if a light touch doesn't reach 100%
- Raise it if pads trigger on their own
- `CANCEL_HOLD_MS` sets how long both pads must be touched to cancel (0.3 seconds)

## Notes & gotchas

- **This runs on the QT Py's native capacitive touch**, not an external touch IC — pad size and material affect sensitivity, so recalibrate if you change either.
- **A hold is detected after 1 second** (`HOLD_THRESHOLD`) — a quick tap always adds time, never starts the timer, so you won't accidentally start with 0 minutes set.
- **After a cancel, nothing counts until both hands are off the pads** — so letting go of one pad a moment before the other doesn't add time back.
- **GPIO 2 and 3 aren't on the QT Py ESP32-S2's pins** — earlier versions of this code read those, which is why touches didn't work. Use A2/A3 (or another touch pin: SDA, SCL, TX).
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Retheme it for any race-the-clock game — a scavenger hunt, an escape room, a chores timer for kids. Swap the urgency colours, add a buzzer for the final ten seconds, or extend `MAX_MINUTES` for longer countdowns. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
