# To-Do Wrist ⌚

> The base build behind [Brass To-Do Wrist](../BrassToDoWrist): a 2.13" tri-colour ePaper display on an ESP32-C3 Super Mini showing your top five tasks, set and ticked off from a phone web page, with a nicer web UI (style picker, animated checkmarks, a branded hero logo) than the brass-enclosure version and the same deep-sleep-until-touched power behaviour.

📺 **Watch the build:** [CMozMaker To-Do Wrist](https://youtu.be/dYDikwG_Oho) Brass version: [Brass Bracelet on YouTube](https://youtu.be/K9FJABPhAVQ)
🛒 **Get the parts:** [Tinker Tailor](https://www.tinkertailor.ca/products/e-paper-to-do-list-video-kit-everything-used-in-the-build)

---

## What it does

Power it up and it tries your home WiFi first, falling back to its own **Todo-Wrist** hotspot if it can't connect. The ePaper screen shows a QR code — scan it, and you land on a mobile-friendly page with five task fields, a background/text colour picker (white/black/red), a dropdown to mark any task complete with an animated checkmark, and buttons to reset all tasks or put the device straight to sleep. Everything — tasks, completion state, and your chosen colours — is saved to flash, so it all survives a deep sleep cycle or a full power loss. The screen only redraws when something actually changes, keeping ePaper refreshes to a minimum.

## Parts list

| Qty | Part | Notes |
|-----|------|-------|
| 1 | ESP32-C3 Super Mini | Any ESP32-C3 board works with pin adjustments |
| 1 | 2.13" tri-colour ePaper display (SSD1680, 250×122) | GxEPD2_213_Z98c driver — black/white/red |
| 1 | LiPo battery + charging board | Optional, for untethered wrist wear |

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

⚠️ If it can't reach your WiFi, it falls back to access point mode — connect to the **Todo-Wrist** network (password: `tinkertailor`) and scan the QR code on the screen.

Also update `upload_port` and `monitor_port` in `platformio.ini` to match your machine (they're set to `COM11`), and swap `webLogoUrl` for your own hosted image if you want a different hero logo on the web page.

## Upload

**PlatformIO:** open this folder in VS Code, plug in the board, hit Upload. The `esp32-c3-devkitm-1` environment is preconfigured with USB CDC enabled, so serial output works over the C3's native USB.

## Debugging boot issues

`main.cpp` has a ladder of `DEBUG_*` flags near the top (`DEBUG_SAFE_BOOT_ONLY`, `DEBUG_EARLY_INIT_ONLY`, `DEBUG_WIFI_ONLY`, `DEBUG_CORE_INIT_ONLY`, `DEBUG_SKIP_DISPLAY_INIT`, `DEBUG_SKIP_PREFS_SETUP`) that let you bring the board up in stages — handy if a change causes a silent boot failure and you need to isolate whether WiFi, the display, or preferences is at fault.

## Notes & gotchas

- **The onboard LED is inverted** on the C3 Super Mini — `LOW` turns it on, `HIGH` turns it off. Not a bug!
- **GPIO 2 (BUSY) and GPIO 9 (RST) are strapping pins** on the ESP32-C3. If the board won't enter bootloader mode or boots strangely, try disconnecting the display while flashing.
- **Deep sleep is disabled by default** (`DEBUG_DISABLE_DEEP_SLEEP = true`) for easier debugging — set it to `false` in `main.cpp` to enable the 1-minute auto-sleep for real battery use.
- **`main.cpp` and `mainVERSION.cpp` at the project root, plus `thankYouSubs.cpp`,** are earlier prototypes kept for reference — only `src/main.cpp` is part of the active PlatformIO build.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

This is the same firmware family as [Brass To-Do Wrist](../BrassToDoWrist) — pick whichever enclosure suits you, brass or otherwise, and the code carries over unchanged. Try more than five tasks, add per-task colours, or repurpose the style picker for a themed display that matches your outfit. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
