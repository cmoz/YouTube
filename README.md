# ⚡ CMozMaker — Project Code

**All the code from the [CMozMaker YouTube channel](https://www.youtube.com/@CMozMaker)** — wearable electronics, fashion tech, and interactive builds, one video at a time.

If you landed here from a video: welcome! Every project I build on the channel lives in this repo, organized so you can grab the code, flash your board, and start making.

---

## 🧵 What you'll find here

I build at the intersection of **fashion, electronics, and art** — think ESP32-powered garments, ePaper displays, RFID interactions, capacitive touch on conductive fabric, and LED work that actually looks good when you wear it.

Typical ingredients:

- **ESP32 family boards** (ESP32, ESP32-S3, ESP32-C3, ESP32-P4) — my daily drivers
- **Wearable-friendly boards** from Seeed Studio, DFRobot, and Adafruit
- **Conductive fabrics & threads** for soft-circuit touch sensors
- **ePaper displays, RFID/NFC, addressable LEDs (WS2812B / FastLED)**

## 📁 How this repo is organized

Each video gets its own folder, named to match the video title:

```
/project-name
  ├── src/            main code (.ino or PlatformIO src)
  ├── platformio.ini  build config (when it's a PlatformIO project)
  ├── README.md       parts list, wiring notes, video link
  └── assets/         diagrams, patterns, extras
```

Every project folder includes a link back to its video, a parts list, and any wiring or pin notes you'll need to follow along.

## 🚀 Getting started

**Option 1 — PlatformIO (recommended)**

1. Install [VS Code](https://code.visualstudio.com/) + the [PlatformIO extension](https://platformio.org/)
2. Clone this repo:
   ```bash
   git clone https://github.com/YOUR-USERNAME/cmozmaker-projects.git
   ```
3. Open the project folder you want in PlatformIO
4. Plug in your board, hit **Upload**, and you're off

**Option 2 — Arduino IDE**

Most projects also work in the Arduino IDE. Open the `.ino` file, install the libraries listed in that project's README, select your board, and upload.

## 🎥 Follow along

- **YouTube:** [CMozMaker](https://www.youtube.com/@CMozMaker) — new builds, tutorials, and the full *ESP32 for Beginners: Wearable Electronics & Fashion Tech* series
- **Shop:** [Tinker Tailor](https://tinkertailor.ca) — the components and conductive fabrics I use in these builds, shipped from Canada
- **Book:** *The Ultimate Guide to Informed Wearable Technology* (Packt, 2022)

## 🤝 Questions & contributions

Found a bug? Built a variation you're proud of? Open an issue or drop a comment on the video — I read them all. If a project won't compile, include your board, IDE/PlatformIO version, and the error message so I can help quickly.

## 📜 License

Code is released under the MIT License — use it, remix it, wear it. If you build something with it, tag me. I genuinely love seeing what people make.

---

*Made in Nova Scotia, Canada 🍁 — where the electronics meet the sewing machine.*
