# ePaper Logo Rotator 🖼️

> A WeAct 2.13" tri-colour ePaper display on an ESP32 that shows the CMozMaker logo and shuffles its background and text colours every five seconds — the perfect first ePaper project, and a great way to test that your display wiring works.

📺 **Watch the build:** [CMozMaker on YouTube](https://youtu.be/Ww5av-e-W50?si=3ih6utYX0S91tld-)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## Two versions, pick your flavour

- **`/ESP-ePaper`** (this folder) — PlatformIO project
- **`/ESP-ePaper_ArduinoIDE`** — the same project as a single `.ino` sketch for the Arduino IDE

Same code, same wiring — just choose the folder that matches how you program your boards.

## What it does

The ESP32 draws a 250×122 bitmap logo on the tri-colour display, then every 5 seconds picks a fresh random background and text colour from white, black, and red — always making sure they never match, so the logo stays readable. Colour changes are logged to the Serial Monitor at 115200 baud so you can watch it think.

Swap in your own logo and this becomes a low-power badge, a shop sign, or the display test for a bigger wearable project.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32 Dev Module (30-pin) | The standard DevKit |
| 1 | WeAct 2.13" tri-colour ePaper (250×122, SSD1680) | Black/white/red, GxEPD2_213_Z98c driver |
| — | Jumper wires | Display is 3.3V logic — do NOT power from 5V |

## Wiring

| WeAct pin | ESP32 pin | Function |
|-----------|-----------|----------|
| CS | GPIO 5 | Chip select |
| DC | GPIO 17 | Data/command |
| RST | GPIO 16 | Reset |
| BUSY | GPIO 4 | Busy status |
| SCL | GPIO 18 | SPI clock |
| SDA | GPIO 23 | SPI data (MOSI) |
| 3.3V / GND | 3.3V / GND | Power — 3.3V only! |

Note the WeAct board labels its SPI pins **SCL/SDA** like I2C — they're actually CLK and MOSI. No MISO needed; the display only listens.

## Libraries

- **GxEPD2** (ZinggJM) — installed automatically via `platformio.ini`, or via Library Manager in the Arduino IDE (it will pull in Adafruit GFX and BusIO)

## Use your own logo

The logo lives in `CMozLogo.h` as a C array. To replace it with your own:

1. Go to [image2cpp](https://javl.github.io/image2cpp/)
2. Upload a **250 × 122** image
3. Set output to a black & white (1-bit) C array
4. Paste the result into `CMozLogo.h`, keeping the array name

## Upload

**PlatformIO:** open this folder, board is preset to `esp32dev`, hit Upload.

**Arduino IDE:** open `ESP-ePaper_ArduinoIDE/ePaperLogo.ino`, select **ESP32 Dev Module**, install GxEPD2, upload.

## Notes & gotchas

- **Blank screen?** Check you're on 3.3V (not 5V), verify the pin table above, and watch the Serial Monitor for init messages. A long press on the ESP32's RST button often kicks it into life.
- **Artifacts or a half-drawn image?** Your bitmap probably isn't exactly 250×122 — re-convert it at image2cpp.
- **Tri-colour ePaper refreshes slowly** (a few seconds, with flashing). That's normal for this display technology, not a bug.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Point the random colours at your own artwork, slow the interval down and run it on a battery as a brooch or bag badge, or graduate to the [Brass To-Do Wrist](../BrassToDoWrist) project which drives the same display family with WiFi and deep sleep. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
