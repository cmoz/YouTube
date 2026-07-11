# 5×5 LED Matrix Necklace 💌

> A 25-pixel WS2812B matrix worn as a pendant, spelling out any message you type from your phone — the necklace hosts its own WiFi hotspot and a colour-picker web page, remembers your last message and colour through a power cycle, and pulses a little red heart between messages.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

Power it on and the necklace scrolls its own IP address across the 5×5 matrix, pulses a red heart, then starts looping: heart pulse → your saved message, letter by letter → pause → repeat. To change the message, connect to the **SecretMessage** WiFi hotspot from your phone, open the necklace's IP in a browser, and you get a page with a text field (up to 20 characters, letters/numbers/`!`/`.`/`-`) plus RGB sliders with a live colour preview. Hit Send and the matrix updates instantly.

Your message and colour are saved to flash (`Preferences`), so they survive a power cycle — the necklace always wakes up remembering what it last said. Press the onboard button any time to have it scroll its IP address again, handy if you need to reconnect later. If nobody connects to the hotspot within a minute, WiFi shuts off to save power and a gentle purple-blue "sleep" pulse plays once.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32-C3 Dev Module (e.g. DevKitM-1) | Any ESP32-C3 board works with pin adjustments |
| 25 | WS2812B LEDs, wired as a 5×5 matrix | Data on GPIO 8 |
| 1 | Push button | On GPIO 9, wired to GND (internal pull-up assumed) |
| 1 | Pendant/necklace enclosure | Diffuser material helps soften each pixel |

## Wiring

| ESP32-C3 pin | Connects to |
|--------------|-------------|
| GPIO 8 | WS2812B matrix data in |
| GPIO 9 | Button (other leg to GND) |
| 5V / GND | Matrix power — 25 LEDs at full brightness draws real current, budget accordingly |

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit NeoPixel** — drives the 5×5 matrix
- **WiFi** + **WebServer** — built into the ESP32 Arduino core, powers the hotspot and message page
- **Preferences** — built into the ESP32 Arduino core, saves your message/colour to flash

## Upload

Open this folder in PlatformIO (VS Code), plug in the ESP32-C3, select the `esp32c3` environment, and hit Upload. Open the Serial Monitor at 115200 baud to watch WiFi setup, button presses, and every character it decodes.

## The font

`src/font5x5.h` holds the full 5×5 pixel alphabet plus `getLetterIndex()`, which maps each character to its glyph. Add a symbol by appending a new `5×5` array and an entry in `getLetterIndex()`.

## Notes & gotchas

- **This folder has several standalone `.cpp` experiments** (`alphabet.cpp`, `CMozComplete.cpp`, `HelloWorld.cpp`, `other.cpp`, `pumpkin.cpp`, `CMOZ.txt`) sitting outside `src/` — leftover from building up the font and message logic. Only `src/main.cpp` is part of the active PlatformIO build.
- **Default hotspot password is `tinkertailor`** — change `apPassword` in `main.cpp` before wearing this anywhere you care about who can post messages.
- **No authentication on the web page** — anyone connected to the hotspot can change what the necklace displays.
- **Brightness is capped at 10** (out of 255) in software — enough to be readable without blinding anyone or draining a small battery too fast.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Extend the font with emoji-style icons, add a scrolling mode instead of letter-by-letter, or drive a bigger matrix for longer messages. The web-form-to-LED pattern here works for any wearable display — reuse it for a badge, a bag, or a sign. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
