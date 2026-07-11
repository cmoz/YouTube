# RFID Tag Reactions 💳

> Tap an RFID/NFC tag on an MFRC522 reader and a 128×64 OLED reacts differently depending on whose tag it is — a pulsing heart and sparkle burst for one special UID, a dancing ASCII skeleton and sparkles for everyone else. A fun, tiny intro to combining RFID with a display on an ESP32.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

On boot the OLED prompts **"Scan a tag..."**. Tap an RFID tag or card against the MFRC522 reader and its UID prints to the Serial Monitor and the OLED. If the UID matches the one hard-coded in `main.cpp`, you get a pulsing heart animation followed by a sparkle burst; any other tag gets a dancing stick-figure skeleton and sparkles instead. After a couple of seconds it clears and waits for the next tap.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32 Dev Module (30-pin) | The standard DevKit |
| 1 | MFRC522 RFID/NFC reader module | 13.56 MHz, SPI |
| 1 | 0.96" SSD1306 OLED (128×64) | I2C |
| — | RFID tags/cards | At least two, to see both reactions |

## Wiring

| Signal | ESP32 pin |
|--------|-----------|
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| RFID SCK | GPIO 18 |
| RFID MISO | GPIO 19 |
| RFID MOSI | GPIO 23 |
| RFID CS (SDA) | GPIO 5 |
| RFID RST | GPIO 2 |
| 3V3 / GND | Both modules — MFRC522 is 3.3V only, do NOT power from 5V |

The OLED runs over I2C (address `0x3C`); the RFID reader runs over SPI, sharing the same bus pins as most other SPI peripherals on this board.

## Libraries

Installed automatically via `platformio.ini`:

- **Adafruit SSD1306** + **Adafruit GFX Library** — OLED display and animations
- **MFRC522** (miguelbalboa) — RFID reader driver

## Set up your own "special" tag

Upload the sketch, scan a tag, and read its UID off the Serial Monitor (115200 baud) or the OLED. Then update this line in `src/main.cpp` with your tag's UID:

```cpp
if (uid == "4c49055") {
```

Every other tag will fall through to the skeleton animation.

## Upload

Open this folder in PlatformIO (VS Code), plug in the ESP32, select the `esp32dev` environment, and hit Upload.

## Notes & gotchas

- **MFRC522 is 3.3V logic and power only** — connecting it to 5V will damage the module.
- **"OLED init failed" on boot** means the display isn't wired correctly or isn't at I2C address `0x3C` — double-check SDA/SCL and try an I2C scanner sketch if unsure.
- **UID comparison is a plain string match**, so it's case- and format-sensitive — make sure what you hard-code matches exactly what prints over Serial.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Add more UIDs and animations to build a whole cast of reactions, drive a WS2812B strip alongside the OLED for bigger visual payoff, or use this as the access-control base for a wearable badge that only "unlocks" for specific people. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
