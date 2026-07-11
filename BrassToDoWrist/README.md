# Brass To-Do Wrist ⌚

> A wrist-worn brass to-do list: a 2.13" tri-colour ePaper display driven by an ESP32-C3 Super Mini, with tasks you set from your phone and tick off from a wee web page — then it deep sleeps until you need it again.

📺 **Watch the build:** [CMozMaker on YouTube](https://youtu.be/K9FJABPhAVQ?si=i8G1BlJxP9BpYZIs)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

Power it up and it either joins your home WiFi or spins up its own hotspot (**Todo-Wrist**). The ePaper screen shows a QR code — scan it with your phone, and you get a little web page where you can enter up to five tasks, toggle them complete, restyle the display, or send the device to sleep. Tasks persist in flash memory, so they survive deep sleep and power loss. Because it's ePaper, your list stays visible even when the ESP32 is asleep and sipping almost no power.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32-C3 Super Mini | Any ESP32-C3 board works with pin adjustments |
| 1 | 2.13" tri-colour ePaper display (SSD1680, 250×122) | GxEPD2_213_Z98c driver — black/white/red |
| 1 | LiPo battery + charging board | Optional, for untethered wrist wear |
| — | Brass sheet / enclosure materials | The fashion part is up to you |

## Wiring

| ESP32-C3 pin | ePaper display |
|--------------|----------------|
| GPIO 8 | CS |
| GPIO 7 | DC |
| GPIO 9 | RST |
| GPIO 2 | BUSY |
| GPIO 4 | MOSI (DIN) |
| GPIO 10 | SCK (CLK) |
| 3V3 / GND | VCC / GND |

GPIO 0 is reserved as the wake button.

## Libraries

Handled automatically by `platformio.ini`:

- **GxEPD2** (ZinggJM) — ePaper driver
- **Adafruit GFX** + **Adafruit BusIO** — graphics and fonts
- **QRCode** (ricmoo) — generates the connect-to-me QR code on the display

## Before you upload

Open `src/main.cpp` and change these near the top:

```cpp
const char *ssid = "CHANGE THIS TO YOUR WIFI";
const char *password = "CHANGE THIS TO YOUR PASSWORD";
```

If it can't reach your WiFi, it falls back to access point mode — connect to the **Todo-Wrist** network (password: `tinkertailor`) and scan the QR code on the screen.

Also update `upload_port` and `monitor_port` in `platformio.ini` to match your machine (they're set to `COM11`).

## Upload

**PlatformIO:** open this folder in VS Code, plug in the board, hit Upload. The `esp32-c3-devkitm-1` environment is preconfigured with USB CDC enabled, so serial output works over the C3's native USB.

## Web interface

Once connected, the page at the device's IP gives you:

- Five task fields with checkboxes to toggle complete
- **Reset Tasks** to clear the list
- **Sleep Now** to send it into deep sleep immediately
- A style option to change how the display renders

Tasks are stored with the ESP32 `Preferences` library, so they persist across sleep and reboots.

## Notes & gotchas

- **The onboard LED is inverted** on the C3 Super Mini — `LOW` turns it on, `HIGH` turns it off. Not a bug!
- **GPIO 2 (BUSY) and GPIO 9 (RST) are strapping pins** on the ESP32-C3. If the board won't enter bootloader mode or boots strangely, try disconnecting the display while flashing.
- **Deep sleep is disabled by default** for easier debugging — set `DEBUG_DISABLE_DEEP_SLEEP = false` in `main.cpp` to enable the 1-minute auto-sleep for real battery use.
- The QR code size is adjustable via `qrSize` (1–4) if your display or IP string needs more room.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

The brass housing is only one direction — this same code drives any wearable ePaper project. Swap the enclosure for leather, laser-cut acrylic, or sew it into a cuff with conductive fabric. Change the five tasks to affirmations, a schedule, or a secret message. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
