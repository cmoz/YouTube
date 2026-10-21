# TFT Face Animations

An animated face for an ESP32 and a GC9A01 1.28" round TFT (240x240). It changes between six expressions: neutral, happy, sad, angry, surprised and sleepy. It also blinks and looks around. After each run through the expressions, a pixel-art cat appears for 10 seconds: its tail sways, it blinks, and hearts float up beside it.

Built with PlatformIO and [GFX Library for Arduino](https://github.com/moononournation/Arduino_GFX).

## Wiring

| TFT pin    | ESP32 DevKit pin |
|------------|------------------|
| VCC        | 3.3V             |
| GND        | GND              |
| SCL (SCK)  | 18               |
| SDA (MOSI) | 23               |
| CS         | 26               |
| DC         | 25               |
| RST        | 27               |
| BL         | 3.3V             |

## Serial commands

Open the serial monitor at 115200 and type a number:

| Key | Shows     |
|-----|-----------|
| 1   | Neutral   |
| 2   | Happy     |
| 3   | Sad       |
| 4   | Angry     |
| 5   | Surprised |
| 6   | Sleepy    |
| 7   | Cat       |

Picking an expression pauses the automatic cycle for 10 seconds.

## Customising

- **New expression:** add a row to `EXPRESSIONS` (eye size, eyelids, mouth curve, mouth opening, blush) and put it in `SEQUENCE`.
- **Colours:** change `FACE_COLOUR`, `BLUSH_COLOUR` and the `CAT_*` constants.
- **Redrawing the cat:** edit `CAT_SPRITE`. Each character is one pixel: `O` outline, `o` fur, `d` stripe, `w` white, `p` pink, `e` eye. Keep every row 16 characters wide.
