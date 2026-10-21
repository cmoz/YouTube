# TFT ESP32 Demo Effects

Demo effects for an ESP32 and a GC9A01 1.28" round TFT (240x240): plasma, twisting tunnel, warp starfield and string art. Each runs for 10 seconds, then the next one starts.

Built with PlatformIO and [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX).

If the screen glitches, lower the SPI speed in `setup()` from `gfx->begin(80000000)` to `gfx->begin(40000000)`.

## Need help with the wiring?

![ESP32 to GC9A01 pin diagram](Screenshot%202026-10-21%20132453.png)

The diagram doesn't show the backlight pin: if your display has a **BL** pin, connect it to 3.3V.

## Questions?

Ask on the [discussion board](https://github.com/cmoz/YouTube/discussions).
