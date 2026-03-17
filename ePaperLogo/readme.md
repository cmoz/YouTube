This version is for Arduino IDE.

/*
 * ====================================================================
 * CMozMaker E-Paper Display with Random Color Changes
 * ====================================================================
 * 
 * Project: WeAct 2.13" Tri-Color E-Paper Display Logo Rotator
 * Author: CMozMaker (Christine Moz)
 * Repository: https://github.com/CMozMaker/[repo-name]
 * 
 * HARDWARE SPECIFICATIONS:
 * ─────────────────────────
 * Display: WeAct 2.13" Tri-Color E-Paper Screen (250 x 122 pixels)
 * Model: GxEPD2_213_Z98c (Black, White, Red)
 * Microcontroller: ESP32 Dev Module (30-pin standard)
 * Interface: SPI (3-wire: CLK, MOSI, CS)
 * 
 * DISPLAY PIN CONNECTIONS (ESP32 30-pin DevKit):
 * ────────────────────────────────────────────────
 * WeAct Pin     ESP32 Pin    Function
 * ───────────────────────────────────────
 * CS            GPIO 5       Chip Select (SPI)
 * DC            GPIO 17      Data/Command Pin
 * RST           GPIO 16      Reset
 * BUSY          GPIO 4       Busy Status
 * CLK (SCL*)    GPIO 18      SPI Clock (labeled SCL on WeAct board)
 * MOSI (SDA*)   GPIO 23      SPI Data (labeled SDA on WeAct board)
 * GND           GND          Ground
 * 3.3V          3.3V         Power (NOT 5V - display is 3.3V logic!)
 * 
 * *Note: WeAct board labels SCL/SDA but uses them for SPI CLK/MOSI.
 *        No MISO line needed for this display (one-way communication).
 * 
 * FEATURES:
 * ─────────
 * • Displays CMozMaker logo bitmap (250x122)
 * • Random background & text color changes every 5 seconds
 * • Ensures background and text colors never match
 * • Available colors: White, Black, Red
 * • Serial debug output for color changes (115200 baud)
 * 
 * IMAGE CONVERSION:
 * ────────────────
 * Tool: https://javl.github.io/image2cpp/
 * Settings:
 *   - Image Width: 250 pixels
 *   - Image Height: 122 pixels
 *   - Color Format: Black & White (1-bit monochrome)
 *   - Output: C array
 * 
 * Generated header file: CMozLogo.h
 * Array name: CMozlogo
 * 
 * RANDOM COLOR CHANGE:
 * ──────────────────
 * • Updates every 5 seconds (5000ms)
 * • Picks random background color from [WHITE, BLACK, RED]
 * • Picks random text color that differs from background
 * • Prints current color pair to Serial Monitor for debugging
 * 
 * LIBRARIES REQUIRED:
 * ──────────────────
 * • GxEPD2 (v1.6.7+) - https://github.com/ZinggJM/GxEPD2
 * • Adafruit GFX Library
 * • Adafruit BusIO
 * 
 * COMPILATION:
 * ────────────
 * Board: ESP32 Dev Module
 * Upload Speed: 921600 (or 115200)
 * 
 * SERIAL OUTPUT:
 * ──────────────
 * Open Serial Monitor @ 115200 baud to see:
 * • Initialization messages
 * • Current color changes every 5 seconds
 * Example: "New colors - BG: 65535 | Text: 0"
 * 
 * TROUBLESHOOTING:
 * ────────────────
 * Blank screen?
 *   → Check power (3.3V, not 5V)
 *   → Check pin connections match defines above
 *   → Monitor Serial output - display may not initialize
 *   → Long press RST button on ESP32
 * 
 * Display shows artifacts/incomplete image?
 *   → Verify bitmap dimensions (250x122)
 *   → Re-convert image at https://javl.github.io/image2cpp/
 *   → Check CMozLogo.h array is complete
 * 
 * ====================================================================
 */
