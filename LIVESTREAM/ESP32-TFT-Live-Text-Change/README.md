# ESP32 TFT Live Text Change

Type a message into the serial monitor and it shows up on an ESP32 + GC9A01 1.28" round TFT (240x240) straight away, with no re-upload. The show loops through your text, an animated face with six expressions, and a pixel-art cat.

Built with PlatformIO and [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX).

## Serial commands (115200 baud)

| Send | What happens |
| --- | --- |
| `>Some Name` | Changes the on-screen text live and jumps to it |
| `1` - `6` | Neutral, happy, sad, angry, surprised, sleepy |
| `7` | Pixel-art cat |
| `8` | Text |

The text is auto-sized and word-wrapped to fit the round screen.

## Wiring

| GC9A01 | ESP32 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO 18 |
| SDA | GPIO 23 |
| CS | GPIO 26 |
| DC | GPIO 25 |
| RST | GPIO 27 |
| BL | 3.3V |

## Customise

At the top of `src/main.cpp`: `TEXT_MESSAGE` is the startup text, and `TEXT_MS`, `FACE_MS` and `CAT_MS` set how long each part stays on screen.

`platformio.ini` has the upload port set to `COM9`. Change it to match your board.

## Questions?

Ask on the [discussion board](https://github.com/cmoz/YouTube/discussions).
