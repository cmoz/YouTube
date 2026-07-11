# Programmer's Macro Keyboard ⌨️

> A LilyGO T-Keyboard S3 Pro turned into a code-snippet macro pad: four keys, each with its own tiny 128×128 LCD showing a live press counter, type out a `for`, `if`, `if/else`, or `while` skeleton over USB HID the instant you press them — plus a rotary knob that doubles as a volume dial.

📺 **Watch the build:** [CMozMaker on YouTube](https://www.youtube.com/@CMozMaker)
🛒 **Get the parts:** [Tinker Tailor](https://tinkertailor.ca)

---

## What it does

The T-Keyboard S3 Pro is a four-key macro pad on an ESP32-S3, where each key sits under its own small IIC-addressed LCD. This sketch turns it into a coding shortcut pad: press a key and the board types the matching snippet straight into whatever editor has focus, as if from a real USB keyboard — no drivers, no companion app.

| Key | Screen label | Types |
|-----|--------------|-------|
| 1 | For | `for(int i=0;i<8;i++) {}` |
| 2 | If | `if(n==8) {}` |
| 3 | Else | `if(n==8) {} else {}` |
| 4 | While | `while(n<8){}` |

Each key's screen also tracks and displays how many times it's been pressed, with a little dot indicator that fills up and wraps every 8 presses. The rotary knob on top isn't wired to macros — it sends USB HID **volume up/down** instead, so the pad doubles as a media dial while it sits on your desk.

## Hardware

| Qty | Part | Notes |
|-----|------|-------|
| 1 | LilyGO T-Keyboard S3 Pro | ESP32-S3, 4 keys, 4× 128×128 IIC-driven LCDs, rotary encoder |

This is a single, self-contained commercial board — no external wiring required.

## Firmware structure

```
ProgramKeys/
  ProgramKeys.ino          the sketch — edit this to change the macros
  AGENCY.h, AGENCY9.h,
  AGENCY12.h, AGENCY15.h   bitmap fonts used on the key LCDs
  libraries/                vendor libraries this sketch depends on
    T-Keyboard-S3-Pro_Drive  LilyGO's IIC driver for the per-key LCDs & keys
    Mylibrary                 pin_config.h — all board pin definitions
    Arduino_DriveBus          IIC bus abstraction used by the keyboard driver
    Arduino_GFX                graphics library driving the LCD sprites
    USB                        ESP32-S3 native USB HID (keyboard + consumer control)
```

## Libraries

Copy the folders inside `ProgramKeys/libraries/` into your Arduino `libraries/` directory (or open the sketch with the `libraries` folder alongside it, which the Arduino IDE also picks up automatically). These are LilyGO's vendor-specific drivers for this board and aren't available through the Library Manager.

## Upload

Open `ProgramKeys/ProgramKeys.ino` in the Arduino IDE, select an **ESP32-S3** board with **USB CDC on boot** and **USB Mode: USB-OTG (TinyUSB)** enabled (required for the HID keyboard to enumerate), and upload. On first boot the sketch scans the IIC bus for the keyboard's driver chip — watch the Serial Monitor at the default baud rate for "IIC_Bus initialization successfully" and the firmware version readout.

## Customize the macros

Edit these four strings near the top of `ProgramKeys.ino`:

```cpp
String firstKey  = "for(int i=0;i<8;i++) {}";
String secondKey = "if(n==8) {}";
String thirdKey  = "if(n==8) {} else {}";
String forthKey  = "while(n<8){}";
```

Update the matching `lbls[]` and `lbls2[]` arrays too if you want the on-screen labels to match your new snippets.

## Notes & gotchas

- **This folder contains a duplicate nested copy** (`ProgrammerKeyboard-main/ProgramKeys/`) left over from a zip download — the one to open is the top-level `ProgramKeys/ProgramKeys.ino`.
- **If the board won't enumerate as a keyboard**, double-check the USB Mode board setting — it must be TinyUSB, not the default Hardware CDC/JTAG mode.
- **Keys only trigger on a fresh press**, not on hold — each key has its own lock flag so a snippet won't repeat while you're still pressing.
- **Stuck, or built something cool?** Join us in [Discussions](https://github.com/cmoz/YouTube/discussions) — ask questions, share your builds, and suggest future videos.

## Make it yours

Swap the four snippets for your own most-typed boilerplate — git commands, HTML tags, whatever you retype all day. The per-key LCDs make it easy to relabel each key visually so you always know what it does. Tag [@CMozMaker](https://www.youtube.com/@CMozMaker) if you build one!
