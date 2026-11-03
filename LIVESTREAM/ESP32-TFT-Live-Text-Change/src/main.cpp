// Animated Face + Pixel Art Cat - ESP32 + GC9A01 1.28" round TFT (240x240)
//
// The show loops: TEXT -> FACE -> CAT -> TEXT -> ...
//
// Text: your message, auto-sized and word-wrapped to fit the round screen.
// Animated face: moves smoothly between six expressions (neutral, happy, sad,
// angry, surprised, sleepy). It blinks at random and its eyes wander around.
// Animated cat: a pixel-art cat swishes its tail, blinks, glances side to side,
// and hearts float up beside it.
//
// Serial monitor (115200):
//   >Some Name   change the on-screen text live (and jump to it), no re-upload
//   1-6          pick an expression, 7 = cat, 8 = text
//
// Frames are drawn into an off-screen canvas and sent to the screen in one go,
// so there's no flicker. Uses the GFX Library for Arduino (moononournation).

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ======== QUICK EDIT: what to show and for how long ========
const char *TEXT_MESSAGE = "Thanks for subscribing!";   // <- change this (or send >name over serial)
const unsigned long TEXT_MS = 6000;     // how long the text stays on screen
const unsigned long FACE_MS = 24000;    // how long the face runs (split evenly across its expressions)
const unsigned long CAT_MS  = 15000;    // how long the cat stays on screen
// ===========================================================

// ---- Wiring: ESP32 DevKit -> GC9A01 1.28" round TFT ----
// VCC -> 3.3V, GND -> GND, BL -> 3.3V (or set TFT_BL to a pin)
#define TFT_SCK   18   // "SCL" on most boards
#define TFT_MOSI  23   // "SDA" on most boards
#define TFT_CS    26
#define TFT_DC    25
#define TFT_RST   27
#define TFT_BL    -1

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *tft = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// Off-screen buffer covering just the face area (200x160 = 64KB), pushed to the
// screen in one go each frame so the animation doesn't flicker.
const int CANVAS_W = 200, CANVAS_H = 160;
const int CANVAS_X = 20, CANVAS_Y = 40;
Arduino_Canvas *gfx = new Arduino_Canvas(CANVAS_W, CANVAS_H, tft, CANVAS_X, CANVAS_Y);

// Face layout, in canvas coordinates
const int FACE_CX = CANVAS_W / 2;
const int EYE_Y = 55, EYE_SPACING = 45;
const int MOUTH_Y = 125;

const uint16_t BG_COLOUR    = RGB565_BLACK;
const uint16_t FACE_COLOUR  = RGB565(0, 220, 255);
const uint16_t MOUTH_INSIDE = RGB565(60, 0, 30);

// Everything that makes up an expression. All floats so we can tween between them.
struct Face {
  float eyeW, eyeH;   // eye size in px
  float lidTop;       // 0..1 of the eye hidden from the top (sleepy)
  float lidSlant;     // px; + = angry (inner corners down), - = sad (outer corners down)
  float lidBottom;    // 0..1 "smiling eyes" (bottom of the eye hidden by an arc)
  float mouthCurve;   // -1 frown .. +1 smile
  float mouthOpen;    // 0..1
  float mouthW;       // mouth width in px
};

enum Expression { NEUTRAL, HAPPY, SAD, ANGRY, SURPRISED, SLEEPY, NUM_EXPRESSIONS };

const char *EXPRESSION_NAMES[] = { "neutral", "happy", "sad", "angry", "surprised", "sleepy" };

const Face EXPRESSIONS[] = {
  //  eyeW eyeH lidTop slant lidBot curve  open  mouthW
  {   44,  56,  0.0,    0,   0.0,   0.3,  0.0,  60 },  // NEUTRAL
  {   48,  52,  0.0,    0,   0.55,  1.0,  0.3,  80 },  // HAPPY
  {   42,  50,  0.15, -14,   0.0,  -0.8,  0.0,  50 },  // SAD
  {   46,  46,  0.1,   16,   0.0,  -0.5,  0.0,  56 },  // ANGRY
  {   54,  66,  0.0,    0,   0.0,   0.0,  1.0,  30 },  // SURPRISED
  {   46,  54,  0.6,   -4,   0.0,   0.1,  0.0,  40 },  // SLEEPY
};

// Order of the automatic expression cycle; each gets an equal share of FACE_MS
const Expression SEQUENCE[] = { NEUTRAL, HAPPY, NEUTRAL, SURPRISED, SAD, ANGRY, NEUTRAL, SLEEPY };
const int SEQUENCE_LEN = sizeof(SEQUENCE) / sizeof(SEQUENCE[0]);
const unsigned long EXPRESSION_MS = FACE_MS / SEQUENCE_LEN;
const unsigned long MANUAL_HOLD_MS = 10000;   // pause the cycle after a serial command

Face current = EXPRESSIONS[NEUTRAL];
Face target  = EXPRESSIONS[NEUTRAL];

int sequenceIndex = 0;
unsigned long nextExpressionAt = 0;

// Blinking
unsigned long nextBlinkAt = 0, blinkStart = 0;
const unsigned long BLINK_MS = 160;
bool blinking = false;

// Eyes wander around a little
float gazeX = 0, gazeY = 0, gazeTargetX = 0, gazeTargetY = 0;
unsigned long nextGazeAt = 0;

// ---- Show mode: text, then face, then cat, then round again ----
enum Mode { TEXT_MODE, FACE_MODE, CAT_MODE };
Mode mode = TEXT_MODE;

// Text (can be replaced live over serial)
String displayText = TEXT_MESSAGE;
unsigned long textStart = 0;
const int TEXT_SAFE_W = 180, TEXT_SAFE_H = 130;   // area that stays inside the round screen

// ---- Pixel art cat, shown for CAT_MS after the face ----
const int CAT_SCALE = 7;                 // screen pixels per art pixel
const int CAT_X = 30, CAT_Y = 30;        // top-left of the sprite in the canvas

// O outline, o fur, d stripe, w white, p pink, e eye (pupils/blinks drawn on top)
const char *CAT_SPRITE[] = {
  ".OO..........OO.",
  ".OpO........OpO.",
  ".OppOOOOOOOOppO.",
  "OooooodoodoooooO",
  "OooooooooooooooO",
  "OooeeooooooeeooO",
  "OooeeooooooeeooO",
  "OwwooooppoooowwO",
  ".OwwwwwOOwwwwwO.",
  "..OOwwwwwwwwOO..",
  "..OooowwwwoooO..",
  ".OoodowwwwodooO.",
  ".OoodowwwwodooO.",
  ".OoooowwwwooooO.",
  ".OowwoooooowwoO.",
  "..OOOOOOOOOOOO..",
};
const int CAT_ROWS = sizeof(CAT_SPRITE) / sizeof(CAT_SPRITE[0]);
const int CAT_EYE_ROW = 5, CAT_EYE_COLS[] = { 3, 11 };

// Tail shape as offsets from where it joins the body; it wags side to side
const int TAIL_ROOT_COL = 13, TAIL_ROOT_ROW = 14;
const int8_t TAIL_PATH[][2] = { {1,0}, {2,0}, {3,-1}, {4,-2}, {4,-3}, {4,-4}, {4,-5}, {3,-6} };
const int TAIL_LEN = sizeof(TAIL_PATH) / sizeof(TAIL_PATH[0]);

const char *HEART_SPRITE[] = { "pp.pp", "ppppp", ".ppp.", "..p.." };
const int HEART_SCALE = 4;
const unsigned long HEART_MS = 1500;
struct Heart { bool active; int x; unsigned long start; };
Heart hearts[3];

const uint16_t CAT_OUTLINE = RGB565(70, 35, 20);
const uint16_t CAT_FUR     = RGB565(255, 150, 40);
const uint16_t CAT_STRIPE  = RGB565(200, 95, 20);
const uint16_t CAT_WHITE   = RGB565_WHITE;
const uint16_t CAT_PINK    = RGB565(255, 140, 170);
const uint16_t CAT_EYE     = RGB565(120, 230, 80);
const uint16_t CAT_PUPIL   = RGB565(20, 20, 20);
const uint16_t HEART_COLOUR = RGB565(255, 60, 100);

unsigned long catStart = 0, catNextBlink = 0, catBlinkEnd = 0, catNextLook = 0, catNextHeart = 0;
bool catLookRight = false;

void startCat();
void startText();

// Move every field of `from` a fraction of the way towards `to`
void tweenFace(Face &from, const Face &to, float amount) {
  float *f = (float *)&from;
  const float *t = (const float *)&to;
  for (size_t i = 0; i < sizeof(Face) / sizeof(float); i++) {
    f[i] += (t[i] - f[i]) * amount;
  }
}

void setExpression(Expression e) {
  target = EXPRESSIONS[e];
  Serial.printf("Expression: %s\n", EXPRESSION_NAMES[e]);
}

// side = -1 for the left eye, +1 for the right (used to mirror the eyelid slant)
void drawEye(int cx, int cy, float w, float h, int side) {
  int x0 = cx - w / 2, y0 = cy - h / 2;
  int r = min(w, h) / 4;
  gfx->fillRoundRect(x0, y0, w, h, r, FACE_COLOUR);

  // Top eyelid: a slanted black shape cut off the top of the eye
  float lidY = y0 + current.lidTop * h;
  float slant = current.lidSlant * side;
  int xl = x0 - 1, xr = x0 + w + 1;
  int yl = lidY + slant, yr = lidY - slant;   // inner corner is towards the face centre
  if (side < 0) { int t = yl; yl = yr; yr = t; }
  int top = y0 - 1;
  gfx->fillTriangle(xl, top, xr, top, xr, yr, BG_COLOUR);
  gfx->fillTriangle(xl, top, xr, yr, xl, yl, BG_COLOUR);

  // Bottom eyelid: an ellipse rising from below turns the eye into a happy "^"
  if (current.lidBottom > 0.01) {
    gfx->fillEllipse(cx, y0 + h * 1.15, w * 0.8, h * current.lidBottom * 1.1, BG_COLOUR);
  }
}

void drawMouth(int cx, int cy) {
  int halfW = current.mouthW / 2;
  float depth = 14 * current.mouthCurve;

  // Curved line: a parabola, ends up for a smile, ends down for a frown
  for (int i = -halfW; i <= halfW; i += 2) {
    float t = (float)i / halfW;
    int y = cy + depth * (1 - t * t) - depth / 2;
    gfx->fillCircle(cx + i, y, 3, FACE_COLOUR);
  }

  // Open mouth
  if (current.mouthOpen > 0.02) {
    int my = cy + depth / 2;
    int rx = 6 + current.mouthOpen * 14;
    int ry = 2 + current.mouthOpen * 16;
    gfx->fillEllipse(cx, my, rx, ry, FACE_COLOUR);
    if (rx > 6 && ry > 6) {
      gfx->fillEllipse(cx, my, rx - 4, ry - 4, MOUTH_INSIDE);
    }
  }
}

void drawFace() {
  gfx->fillScreen(BG_COLOUR);

  // Blink: eye height dips to nothing and back over BLINK_MS
  float blinkScale = 1.0;
  if (blinking) {
    float p = (float)(millis() - blinkStart) / BLINK_MS;
    blinkScale = fabs(1 - 2 * p);
  }
  float eyeH = max(4.0f, current.eyeH * blinkScale);

  int ex = gazeX, ey = EYE_Y + gazeY;
  drawEye(FACE_CX - EYE_SPACING + ex, ey, current.eyeW, eyeH, -1);
  drawEye(FACE_CX + EYE_SPACING + ex, ey, current.eyeW, eyeH, +1);

  drawMouth(FACE_CX + gazeX / 3, MOUTH_Y);

  gfx->flush();
}

void updateAnimation() {
  unsigned long now = millis();

  // Expression cycle; the cat gets a turn after the last one
  if (now >= nextExpressionAt) {
    sequenceIndex++;
    if (sequenceIndex >= SEQUENCE_LEN) {
      startCat();
      return;
    }
    setExpression(SEQUENCE[sequenceIndex]);
    nextExpressionAt = now + EXPRESSION_MS;
  }

  // Blinks at random intervals
  if (!blinking && now >= nextBlinkAt) {
    blinking = true;
    blinkStart = now;
  }
  if (blinking && now - blinkStart >= BLINK_MS) {
    blinking = false;
    nextBlinkAt = now + random(1500, 5000);
  }

  // Pick a new place to look every so often
  if (now >= nextGazeAt) {
    gazeTargetX = random(-12, 13);
    gazeTargetY = random(-6, 7);
    nextGazeAt = now + random(800, 2500);
  }
  gazeX += (gazeTargetX - gazeX) * 0.25;
  gazeY += (gazeTargetY - gazeY) * 0.25;

  tweenFace(current, target, 0.18);
}

void startCat() {
  unsigned long now = millis();
  mode = CAT_MODE;
  catStart = now;
  catNextBlink = now + 1200;
  catNextLook = now + 800;
  catNextHeart = now + 1000;
  for (Heart &h : hearts) h.active = false;
  Serial.println("Cat!");
}

void endCat() {
  startText();
}

void startFace() {
  mode = FACE_MODE;
  sequenceIndex = 0;
  setExpression(SEQUENCE[0]);
  nextExpressionAt = millis() + EXPRESSION_MS;
}

void startText() {
  mode = TEXT_MODE;
  textStart = millis();
  Serial.printf("Text: %s\n", displayText.c_str());
}

// Split text into lines of at most maxChars, breaking at spaces.
// Returns the line count, or -1 if a word is too long or there are too many lines.
const int MAX_LINES = 12;
int wrapText(const String &text, int maxChars, String lines[]) {
  int n = 0;
  String line = "";
  int pos = 0, len = text.length();
  while (pos < len) {
    int end = text.indexOf(' ', pos);
    if (end < 0) end = len;
    String word = text.substring(pos, end);
    pos = end + 1;
    if (word.length() == 0) continue;
    if ((int)word.length() > maxChars) return -1;
    if (line.length() == 0) {
      line = word;
    } else if ((int)(line.length() + 1 + word.length()) <= maxChars) {
      line += " " + word;
    } else {
      if (n >= MAX_LINES) return -1;
      lines[n++] = line;
      line = word;
    }
  }
  if (line.length()) {
    if (n >= MAX_LINES) return -1;
    lines[n++] = line;
  }
  return n;
}

// Centre the message, using the biggest text size that fits
void drawText() {
  gfx->fillScreen(BG_COLOUR);

  String lines[MAX_LINES];
  int n = 0, size = 1;
  for (size = 6; size >= 1; size--) {
    n = wrapText(displayText, TEXT_SAFE_W / (6 * size), lines);
    if (n > 0 && n * 10 * size <= TEXT_SAFE_H) break;   // 8px glyph + 2px line gap
  }
  if (n <= 0) {   // nothing fits even at size 1: hard-cut what we can
    n = 1;
    lines[0] = displayText.substring(0, TEXT_SAFE_W / 6);
  }

  gfx->setTextSize(size);
  gfx->setTextColor(FACE_COLOUR);
  int lineH = 10 * size;
  int y = (CANVAS_H - n * lineH) / 2;
  for (int i = 0; i < n; i++) {
    int w = lines[i].length() * 6 * size;
    gfx->setCursor((CANVAS_W - w) / 2, y + i * lineH);
    gfx->print(lines[i]);
  }

  gfx->flush();
}

void updateText() {
  if (millis() - textStart >= TEXT_MS) startFace();
}

void catPixel(int col, int row, uint16_t colour) {
  gfx->fillRect(CAT_X + col * CAT_SCALE, CAT_Y + row * CAT_SCALE, CAT_SCALE, CAT_SCALE, colour);
}

void updateCat() {
  unsigned long now = millis();
  if (now - catStart >= CAT_MS) {
    endCat();
    return;
  }

  if (now >= catNextBlink) {
    catBlinkEnd = now + 150;
    catNextBlink = now + random(1500, 4000);
  }
  if (now >= catNextLook) {
    catLookRight = random(2);
    catNextLook = now + random(700, 2000);
  }

  // Float a heart up beside the cat's head now and then
  if (now >= catNextHeart) {
    for (Heart &h : hearts) {
      if (!h.active) {
        h.active = true;
        h.x = random(2) ? random(25, 45) : random(150, 165);
        h.start = now;
        break;
      }
    }
    catNextHeart = now + random(600, 1500);
  }
  for (Heart &h : hearts) {
    if (h.active && now - h.start >= HEART_MS) h.active = false;
  }
}

void drawCat() {
  unsigned long now = millis();
  gfx->fillScreen(BG_COLOUR);

  // Tail goes behind the body: outline blocks first, then the fur down the middle
  float phase = (now - catStart) * 0.006;
  int tailCol[TAIL_LEN], tailRow[TAIL_LEN];
  for (int i = 0; i < TAIL_LEN; i++) {
    float sway = sin(phase - i * 0.6) * (i + 1) / TAIL_LEN * 2;
    tailCol[i] = TAIL_ROOT_COL + TAIL_PATH[i][0] + lround(sway);
    tailRow[i] = TAIL_ROOT_ROW + TAIL_PATH[i][1];
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
        catPixel(tailCol[i] + dx, tailRow[i] + dy, CAT_OUTLINE);
  }
  for (int i = 0; i < TAIL_LEN; i++) {
    catPixel(tailCol[i], tailRow[i], i % 2 ? CAT_STRIPE : CAT_FUR);
  }

  for (int row = 0; row < CAT_ROWS; row++) {
    for (int col = 0; CAT_SPRITE[row][col]; col++) {
      uint16_t colour;
      switch (CAT_SPRITE[row][col]) {
        case 'O': colour = CAT_OUTLINE; break;
        case 'o': colour = CAT_FUR; break;
        case 'd': colour = CAT_STRIPE; break;
        case 'w': colour = CAT_WHITE; break;
        case 'p': colour = CAT_PINK; break;
        case 'e': colour = CAT_EYE; break;
        default: continue;   // transparent
      }
      catPixel(col, row, colour);
    }
  }

  // Eyes: shut while blinking, otherwise pupils glance left or right
  for (int eyeCol : CAT_EYE_COLS) {
    if (now < catBlinkEnd) {
      catPixel(eyeCol, CAT_EYE_ROW, CAT_FUR);
      catPixel(eyeCol + 1, CAT_EYE_ROW, CAT_FUR);
      catPixel(eyeCol, CAT_EYE_ROW + 1, CAT_OUTLINE);
      catPixel(eyeCol + 1, CAT_EYE_ROW + 1, CAT_OUTLINE);
    } else {
      int pupilCol = eyeCol + (catLookRight ? 1 : 0);
      catPixel(pupilCol, CAT_EYE_ROW, CAT_PUPIL);
      catPixel(pupilCol, CAT_EYE_ROW + 1, CAT_PUPIL);
    }
  }

  for (Heart &h : hearts) {
    if (!h.active) continue;
    int y = 60 - (int)((now - h.start) * 70 / HEART_MS);
    y -= y % HEART_SCALE;   // move in whole art pixels so it stays chunky
    for (int row = 0; row < 4; row++)
      for (int col = 0; col < 5; col++)
        if (HEART_SPRITE[row][col] == 'p')
          gfx->fillRect(h.x + col * HEART_SCALE, y + row * HEART_SCALE, HEART_SCALE, HEART_SCALE, HEART_COLOUR);
  }

  gfx->flush();
}

// ">text" sets the message live; 1-6 pick an expression, 7 the cat, 8 the text
void handleSerial() {
  static String typed;
  static bool typing = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (typing) {
      if (c == '\n' || c == '\r') {
        typing = false;
        typed.trim();
        if (typed.length()) {
          displayText = typed;
          startText();
        }
      } else {
        typed += c;
      }
      continue;
    }
    int e = c - '1';
    if (c == '>') {
      typing = true;
      typed = "";
    } else if (e >= 0 && e < NUM_EXPRESSIONS) {
      mode = FACE_MODE;
      setExpression((Expression)e);
      nextExpressionAt = millis() + MANUAL_HOLD_MS;
    } else if (c == '7') {
      startCat();
    } else if (c == '8') {
      startText();
    }
  }
}

void setup() {
  Serial.begin(115200);
  if (TFT_BL >= 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
  if (!tft->begin()) {
    Serial.println("tft->begin() failed!");
  }
  tft->fillScreen(BG_COLOUR);
  if (!gfx->begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.println("Canvas allocation failed!");
  }

  randomSeed(esp_random());
  Serial.println(">text = set message; 1 neutral, 2 happy, 3 sad, 4 angry, 5 surprised, 6 sleepy, 7 cat, 8 text");
  nextBlinkAt = millis() + 2000;
  startText();
}

void loop() {
  handleSerial();
  if (mode == TEXT_MODE) {
    updateText();
    if (mode == TEXT_MODE) drawText();
  } else if (mode == CAT_MODE) {
    updateCat();
    if (mode == CAT_MODE) drawCat();
  } else {
    updateAnimation();
    if (mode == FACE_MODE) drawFace();
  }
  delay(16);
}
