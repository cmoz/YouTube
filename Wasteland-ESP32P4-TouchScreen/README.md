# Wasteland Salvage Terminal ☢️

> A diegetic prop-style device built on the Elecrow CrowPanel Advanced 7" ESP32-P4 — a 1024×600 touchscreen that boots into a scratched, dormant "STANDBY" face and hides a full retro instrument UI underneath: a Geiger-style dial driven by a real ultrasonic sensor, a salvage log, component scanner tools, and glitch effects that pulse an external LED strip.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## ⚠️ This one is different

Unlike the other projects in this repo, this is an **ESP-IDF project** (v5.5.3), not Arduino or PlatformIO. The ESP32-P4 with a MIPI-DSI display needs ESP-IDF's display stack — see the build section below if you're new to it.

## What it does

The device presents as a battered, found-object terminal. On boot: a brief splash, then it settles into a dormant screen — dim amber "STANDBY" text, hairline scratches drawn across the glass. Underneath is the **Salvage Terminal** shell: a dark bezel with a nav row for four modes, plus a couple of hidden ones.

| Mode | What it does |
|------|--------------|
| **GEIGER** | An analog instrument dial with a live needle, tick marks, and audio clicks — driven by a real HC-SR04 ultrasonic sensor (distance as "radiation"), falling back to simulation if no sensor is attached. Readings feed a log and pulse the LED strip. |
| **LOG** | The salvage log — a record of readings and events |
| **PLAY** | Deals random component cards with creative prompts — a build-idea generator in the wasteland fiction |
| **SCAN** | Tools tabs, including a resistor tool and tutorial list |
| **CMOZ** | Generates QR codes on the display |
| **DEMO** | Effects showcase |

A glitch timer periodically corrupts the UI for a moment — with a callback hook that fires the external WS2812B strip in sync, so the whole prop twitches, not just the screen.

## Hardware

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Elecrow CrowPanel Advanced 7" ESP32-P4 | 1024×600 MIPI-DSI (EK79007), GT911 capacitive touch |
| 1 | HC-SR04 ultrasonic sensor | Drives the Geiger needle |
| 1 | WS2812B strip (18 LEDs) | Glitch pulses and reading effects |
| 1 | Speaker | On-board I2S amp on the CrowPanel |

### External connections

| Signal | GPIO |
|--------|------|
| HC-SR04 TRIG | 4 |
| HC-SR04 ECHO | 5 |
| WS2812B data | 3 |

Audio runs on I2S (LRCLK 21, BCLK 22, SDATA 23) with the amp enable on GPIO 30 — note the amp enable is **active LOW**. The MIPI-DSI panel is powered via the P4's internal LDO channels 3 (2.5 V) and 4 (3.3 V), handled in `main.c`.

Both the sensor and LED strip are optional: the firmware degrades gracefully (simulated readings, dark strip) if init fails.

## Project structure

```
main/
  main.c          bring-up: LDO, I2C, touch, display, audio, sensor, LEDs
  ui/ui_shell.*   persistent terminal chrome, nav, glitch timer
  modes/          one file per mode (geiger, log, play, tools, cmoz, demo,
                  splash, dormant) — each builds an LVGL tree and hands it
                  to the shell
  audio/          I2S tone/click playback
  sensors/        HC-SR04 driver
  leds/           WS2812B via RMT
  data/           component cards, creative prompts, tutorial list
  lib/            QR code generation
```

Modes are cleanly decoupled from the shell: each one creates its content inside `ui_shell_get_content_area()` and the shell owns swapping and cleanup. Adding your own mode means one new `.c/.h` pair, an enum entry, and a branch in `on_nav_mode()` — a nice codebase to learn LVGL 9 architecture from.

## Dependencies

Pulled automatically by the IDF component manager (`main/idf_component.yml`):

- lvgl/lvgl ^9.2
- espressif/esp_lvgl_port
- espressif/esp_lcd_ek79007 (display)
- espressif/esp_lcd_touch_gt911 (touch)
- espressif/led_strip

## Build & flash

With ESP-IDF v5.5.3 installed and exported:

```
idf.py set-target esp32p4
idf.py build
idf.py -p COM13 flash monitor
```

(Replace `COM13` with your port.) On Windows there's a convenience wrapper, `idf.ps1`, that sets `IDF_PATH`, runs the export script, and passes arguments through to `idf.py` — edit the `$idfRoot` path inside it to match your ESP-IDF install location.

## Notes & gotchas

- **Chip revision errors on flash?** Some ESP32-P4 modules report a silicon revision newer than the default minimum. Fix it in `idf.py menuconfig` → Component config → Hardware Settings → Chip revision, and set the minimum to match your chip.
- **Windows path length limit** can break ESP-IDF builds in deeply nested folders. Keep the project close to the drive root (e.g. `C:\proj\wasteland`) or enable long paths in Windows.
- **No sound?** Remember the amp enable on GPIO 30 is active LOW. Also check the on-screen mute toggle — muting is wired through the shell.
- **`sdkconfig.defaults`** carries the display and PSRAM settings this board needs — if you delete your `sdkconfig` to start fresh, those defaults regenerate it correctly.

## Make it yours

The Salvage Terminal shell is a reusable skeleton for any diegetic device: swap the modes for your own fiction — a starship console, a field research instrument, a haunted radio. The dormant screen is the trick that sells it: a device that pretends to be dead junk until it isn't. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
