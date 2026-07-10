# Touch Strip: Hold & Release 🌈

> Six conductive fabric touch pads on a sleeve, each with its own LED animation and musical tone. Hold a pad and its animation plays; release and it stops; press a new pad mid-animation and it takes over instantly — with a proper "hold stack" so releasing falls back to whatever you're still touching.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the kit:** [Open Universal Input Sleeve (OUIS) at Tinker Tailor](https://www.tinkertailor.ca/products/open-universal-input-sleeve-ouis)

---

## What it does

This sketch powers the **OUIS kit** — a wearable sleeve with capacitive touch inputs made from conductive fabric. Running on a **Circuit Playground Classic**, each of the six touch pads triggers its own effect on an external WS2812B LED strip, plus a tone on the onboard speaker:

| Pad (pin) | Animation | Tone |
|-----------|-----------|------|
| 10 | Rainbow | C5 (523 Hz) |
| 9 | Sparkle | D5 (587 Hz) |
| 6 | Blue pulse | E5 (659 Hz) |
| 12 | Theater chase | F5 (698 Hz) |
| 0 | Red wipe | G5 (784 Hz) |
| 3 | Green wipe | A5 (880 Hz) |

The interaction model is what makes it feel good to wear:

- **Touch and hold** → that pad's animation plays continuously
- **Release** → it stops, or falls back to another pad you're still holding
- **Press a new pad while holding one** → the new animation interrupts immediately

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Circuit Playground Classic | Code adapts easily to a Flora or ESP32 |
| 1 | WS2812B LED strip (38 LEDs) | Data on pin 2; [strips at Tinker Tailor](https://www.tinkertailor.ca/collections/light) |
| 6 | Conductive fabric touch pads | Sewn with conductive thread to pads 10, 9, 6, 12, 0, 3 |
| 1 | Sleeve / garment base | See `Sleeve.pdf` in this folder for the pattern |
| 1 | Battery pack | For untethered wear |

## Wiring & construction

- LED strip **data in → pin 2** (this pin is reserved — it's deliberately excluded from the touch pads to avoid a pinMode conflict)
- Conductive fabric pads connect to pins **10, 9, 6, 12, 0, 3** with conductive thread
- `Sleeve.pdf` in this folder has the sleeve layout

## Libraries

Install via the Arduino Library Manager:

- **Adafruit Circuit Playground** — board support and `readCap()` touch sensing
- **FastLED** — drives the external strip

The sketch `#undef`s `ENCODER` after including the Circuit Playground library — that's intentional, to dodge a naming clash with FastLED.

## Upload

Open `touch_strip_hold_release.ino` in the Arduino IDE, select **Adafruit Circuit Playground** as the board, and upload. Open the Serial Monitor at 115200 baud to watch calibration and touch events.

## How the touch sensing stays reliable

Worth reading even if you just want to use it — these techniques transfer to any fabric touch project:

- **Baseline calibration at startup:** the sketch averages 20 readings per pad while untouched. Keep your hands off the pads for the first second after power-up!
- **Smoothing:** readings are low-pass filtered so fabric noise doesn't cause flicker
- **Hysteresis:** a press must rise 60 counts above baseline, but only counts as released when it drops below 30 — so a finger resting right at the threshold doesn't stutter on and off
- **Stuck-reading guard:** saturated readings are treated as invalid rather than as touches

## Notes & gotchas

- **Restart after wearing it differently.** Baselines are captured at power-up, so if the sleeve shifts against your skin significantly, a quick reset recalibrates everything.
- **Tune to your fabric:** if your pads are larger, smaller, or a different conductive fabric, adjust `ON_DELTA` and `OFF_DELTA` in the sketch.
- **Change `STRIP_COUNT`** to match your LED strip length, and mind your power budget — 38 LEDs at full white is more than a small battery likes. `BRIGHTNESS` is capped at 140 for a reason.

## Make it yours

Remap the animations, swap the tones for a pentatonic scale so anything you play sounds good, or port it to an ESP32 for WiFi tricks. The hold-stack interaction works for any touch-driven wearable — it's a lovely pattern to reuse. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!


This sketch is part of the OUIS kit https://www.tinkertailor.ca/products/open-universal-input-sleeve-ouis?_pos=1&_psq=ouis&_ss=e&_v=1.0 The effects are that using a Circui playground Classic in my example (you can modify code to work with a Flora, or ESP32) it will have a tone on touch, and in my example i added a strip of WS2812b lights so that it can also have fun lighting effects when you touch! https://www.tinkertailor.ca/collections/light
