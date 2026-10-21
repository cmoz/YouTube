#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ---- Wiring: ESP32 DevKit -> GC9A01 1.28" round TFT ----
// VCC -> 3.3V, GND -> GND, BL -> 3.3V (or set TFT_BL to a pin)
#define TFT_SCK   18   // "SCL" on most boards
#define TFT_MOSI  23   // "SDA" on most boards
#define TFT_CS    26
#define TFT_DC    25
#define TFT_RST   27
#define TFT_BL    -1

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

const int W = 240, H = 240, CX = 120, CY = 120;
const int STRIP = 24;                 // rows rendered per SPI transfer
const uint32_t EFFECT_MS = 10000;     // time per effect

uint16_t strip[W * STRIP];            // small line buffer, no full framebuffer needed
uint16_t palette[256];
uint8_t palR[256], palG[256], palB[256];
int8_t sinTab[256];

// Lookup tables for one quadrant; the other three are mirrored
uint8_t distLUT[CY][CX];   // distance from centre
uint8_t angLUT[CY][CX];    // angle, 256 steps = full circle
uint8_t depthLUT[CY][CX];  // 1/distance, for the tunnel

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void buildTables() {
  for (int i = 0; i < 256; i++) {
    float a = i * TWO_PI / 256.0f;
    sinTab[i] = (int8_t)(127 * sinf(a));
    palR[i] = 128 + 127 * sinf(a);
    palG[i] = 128 + 127 * sinf(a + TWO_PI / 3);
    palB[i] = 128 + 127 * sinf(a + 2 * TWO_PI / 3);
    palette[i] = rgb565(palR[i], palG[i], palB[i]);
  }
  for (int y = 0; y < CY; y++) {
    for (int x = 0; x < CX; x++) {
      float fx = x + 0.5f, fy = y + 0.5f;
      float d = sqrtf(fx * fx + fy * fy);
      distLUT[y][x] = (uint8_t)min(d, 255.0f);
      angLUT[y][x] = (uint8_t)(atan2f(fy, fx) * 128.0f / PI);
      depthLUT[y][x] = (uint8_t)((int)(2048.0f / d) & 0xFF);
    }
  }
}

// Map a screen coordinate to its quadrant-table index
static inline int quadIndex(int v, int c) { return v < c ? c - 1 - v : v - c; }

// ---------------- Effect 1: Plasma ----------------
void effectPlasma(uint32_t f) {
  for (int y0 = 0; y0 < H; y0 += STRIP) {
    uint16_t *p = strip;
    for (int y = y0; y < y0 + STRIP; y++) {
      int iy = quadIndex(y, CY);
      int sy = sinTab[(uint8_t)(y * 2 + f * 3)];
      for (int x = 0; x < W; x++) {
        int ix = quadIndex(x, CX);
        int v = sinTab[(uint8_t)(x * 2 + f * 2)] + sy
              + sinTab[(uint8_t)(x + y + f * 5)]
              + sinTab[(uint8_t)(distLUT[iy][ix] * 4 - f * 4)];
        *p++ = palette[(uint8_t)((v >> 2) + f)];
      }
    }
    gfx->draw16bitRGBBitmap(0, y0, strip, W, STRIP);
  }
}

// ---------------- Effect 2: Twisting tunnel ----------------
void effectTunnel(uint32_t f) {
  uint8_t spin = f * 2, move = f * 4;
  for (int y0 = 0; y0 < H; y0 += STRIP) {
    uint16_t *p = strip;
    for (int y = y0; y < y0 + STRIP; y++) {
      int iy = quadIndex(y, CY);
      bool top = y < CY;
      for (int x = 0; x < W; x++) {
        int ix = quadIndex(x, CX);
        uint8_t a = angLUT[iy][ix];
        if (x < CX) a = 128 - a;
        if (top) a = -a;
        uint8_t depth = depthLUT[iy][ix];
        uint8_t u = depth + move;
        uint8_t v = a + spin + (depth >> 2);   // twist increases with depth
        uint8_t c = (u ^ v) + f;
        uint16_t shade = min(distLUT[iy][ix] * 3, 255);  // centre fades into the distance
        *p++ = rgb565(palR[c] * shade >> 8, palG[c] * shade >> 8, palB[c] * shade >> 8);
      }
    }
    gfx->draw16bitRGBBitmap(0, y0, strip, W, STRIP);
  }
}

// ---------------- Effect 3: Warp starfield ----------------
struct Star { float x, y, z; int16_t sx, sy; };
const int NUM_STARS = 160;
Star stars[NUM_STARS];

void resetStar(Star &s, float z) {
  s.x = random(-1000, 1000);
  s.y = random(-1000, 1000);
  s.z = z;
  s.sx = -1;
}

void effectStars(uint32_t f) {
  if (f == 0) {
    for (auto &s : stars) resetStar(s, random(50, 1000));
  }
  for (auto &s : stars) {
    if (s.sx >= 0) gfx->fillRect(s.sx, s.sy, 2, 2, 0);
    s.z -= 10;
    if (s.z < 10) { resetStar(s, 1000); continue; }
    int sx = CX + s.x * 60 / s.z;
    int sy = CY + s.y * 60 / s.z;
    int dx = sx - CX, dy = sy - CY;
    if (dx * dx + dy * dy > 117 * 117) { resetStar(s, 1000); continue; }
    uint8_t b = 255 - s.z * 255 / 1000;
    gfx->fillRect(sx, sy, s.z < 300 ? 2 : 1, s.z < 300 ? 2 : 1, rgb565(b, b, b));
    s.sx = sx;
    s.sy = sy;
  }
}

// ---------------- Effect 4: String art ----------------
void effectStringArt(uint32_t f) {
  static float m2;
  uint32_t k = f % 400;
  if (k == 0) {
    gfx->fillScreen(0);
    m2 = random(2, 8) + random(0, 2) * 0.5f;   // ratio sets the pattern
  }
  float a1 = k * 0.05f, a2 = a1 * m2;
  const int R = 118;
  gfx->drawLine(CX + R * cosf(a1), CY + R * sinf(a1),
                CX + R * cosf(a2), CY + R * sinf(a2),
                palette[(uint8_t)(k * 2)]);
}

// ---------------- Effect sequencer ----------------
typedef void (*EffectFn)(uint32_t frame);
struct Effect { const char *name; EffectFn fn; };
Effect effects[] = {
  {"Plasma", effectPlasma},
  {"Tunnel", effectTunnel},
  {"Starfield", effectStars},
  {"String art", effectStringArt},
};
const int NUM_EFFECTS = sizeof(effects) / sizeof(effects[0]);

uint8_t current = 0;
uint32_t frame = 0, effectStart = 0;

void setup() {
  Serial.begin(115200);
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
  if (!gfx->begin(80000000)) {   // drop to 40000000 if you see glitches
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(0);
  randomSeed(esp_random());
  buildTables();
  effectStart = millis();
  Serial.println(effects[current].name);
}

void loop() {
  if (millis() - effectStart > EFFECT_MS) {
    current = (current + 1) % NUM_EFFECTS;
    frame = 0;
    effectStart = millis();
    gfx->fillScreen(0);
    Serial.println(effects[current].name);
  }
  uint32_t t0 = millis();
  effects[current].fn(frame++);
  uint32_t dt = millis() - t0;
  if (dt < 16) delay(16 - dt);   // cap at ~60 fps
}
