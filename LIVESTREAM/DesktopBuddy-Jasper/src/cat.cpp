/*
 * Buddy the pixel kitten - wanders her little room, sits, blinks, bats her
 * yarn ball around and curls up to sleep at night. Tickle her and she jumps
 * with hearts; tickle her a lot and she gets the zoomies.
 *
 * The kitten is drawn from tiny pictures made of letters (one letter = one
 * pixel, drawn 3x bigger on the screen):
 *   K outline   O ginger fur   o stripes   W white   P pink   E eye   . see-through
 */

#include <Arduino.h>
#include "cat.h"

static const int S = 3;          // each kitten pixel is 3x3 screen pixels
static const int CAT_W = 16 * S, CAT_H = 12 * S;
static const int FLOOR_Y = 110;  // top of the floor
static const int MAX_X = 128 - CAT_W;

// Facing right. Flipped when she walks left
static const char *const WALK1[12] = {
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  "K........KOWPWK.",
  "KO.......KWWWWK.",
  ".KOKKKKKKKOOOOK.",
  "..KOOOOOOOOOOOK.",
  "..KOoOoOoOoOOOK.",
  "..KOOOOOOOWWWOK.",
  "..KOK..KOK.KOK..",
  "..KWK..KWK.KWK..",
};
static const char *const WALK2[12] = {
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  "K........KOWPWK.",
  "KO.......KWWWWK.",
  ".KOKKKKKKKOOOOK.",
  "..KOOOOOOOOOOOK.",
  "..KOoOoOoOoOOOK.",
  "..KOOOOOOOWWWOK.",
  "...KOK.KOK..KOK.",
  "...KWK.KWK..KWK.",
};
static const char *const SIT[12] = {
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  ".........KOWPWK.",
  ".........KWWWWK.",
  "......KKKKOOOOK.",
  ".....KOOOOOOWK..",
  "....KOoOoOOOWK..",
  "KK..KOOOOOOOWK..",
  ".KKKKOOOOOOOWK..",
  "....KKWWKKKWWK..",
};
static const char *const JUMP[12] = {
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  "K........KOWPWK.",
  "KO.......KWWWWK.",
  ".KOKKKKKKKOOOOK.",
  "..KOOOOOOOOOOOK.",
  "..KOoOoOoOoOOOK.",
  "..KOOOOOOOWWWOK.",
  ".KOK........KOK.",
  "KWK..........KWK",
};
static const char *const SLEEP[12] = {   // curled up, tail wrapped round the front
  "................",
  "................",
  "................",
  "................",
  "..........K..K..",
  ".....KKKKKPKKPK.",
  "...KKOOoOoKOOOK.",
  "..KOOoOoOOKOOOK.",
  ".KOOOOOOOOKKOKK.",
  ".KOoOoOOOOOWPWK.",
  ".KOOOOKKKKKKKKK.",
  "..KKKKoOoOoOoK..",
};
static const char *const STRETCH[12] = {   // big cat stretch: bum up, paws out front
  "................",
  "K...............",
  "KO..............",
  ".KO.............",
  "..KKKK..........",
  ".KOOOOKKK.......",
  ".KOoOoOOOKK.K.K.",
  ".KOOOOOOOOOKPKPK",
  ".KOOoOoOOOOKOOOK",
  ".KOKKKKKKOOKEOEK",
  ".KOK....KOOKWPWK",
  ".KWK....KWWWWWWK",
};
static const char *const GROOM1[12] = {   // sitting, licking a paw
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  ".........KOWPWKK",
  ".........KWWWWKW",
  "......KKKKOOOOKW",
  ".....KOOOOOOWKW.",
  "....KOoOoOOOWK..",
  "KK..KOOOOOOOWK..",
  ".KKKKOOOOOOOWK..",
  "....KKWWKKKWWK..",
};
static const char *const GROOM2[12] = {
  "..........K..K..",
  ".........KPKKPK.",
  ".........KOOOOK.",
  ".........KOEOEK.",
  ".........KOWPWKP",
  ".........KWWWWKW",
  "......KKKKOOOOKW",
  ".....KOOOOOOWKW.",
  "....KOoOoOOOWK..",
  "KK..KOOOOOOOWK..",
  ".KKKKOOOOOOOWK..",
  "....KKWWKKKWWK..",
};
static const char *const HEART[4] = { "PP.PP", "PPPPP", ".PPP.", "..P.." };

static const uint16_t WALL    = rgb(42, 32, 64);
static const uint16_t FLOOR   = rgb(120, 78, 48);
static const uint16_t FLOOR_D = rgb(90, 56, 34);
static const uint16_t FLOOR_L = rgb(160, 110, 70);
static const uint16_t FRAME   = rgb(150, 110, 80);
static const uint16_t DAY_SKY = rgb(120, 190, 255);
static const uint16_t NIGHT   = rgb(14, 18, 55);
static const uint16_t SUN     = rgb(255, 215, 60);
static const uint16_t MOON    = rgb(240, 240, 210);
static const uint16_t YARN    = rgb(255, 100, 150);
static const uint16_t YARN_D  = rgb(190, 50, 100);
static const uint16_t WHITE   = rgb(255, 255, 255);
static const uint16_t INK     = rgb(30, 22, 40);

// mix two colours: f = 0 gives a, f = 1 gives b
static uint16_t blend(uint16_t a, uint16_t b, float f) {
  int r1 = a >> 11, g1 = (a >> 5) & 63, b1 = a & 31;
  int r2 = b >> 11, g2 = (b >> 5) & 63, b2 = b & 31;
  return ((int)(r1 + (r2 - r1) * f) << 11) | ((int)(g1 + (g2 - g1) * f) << 5) | (int)(b1 + (b2 - b1) * f);
}

static uint16_t colourOf(char ch) {
  switch (ch) {
    case 'K': return rgb(45, 28, 30);
    case 'O': return rgb(255, 150, 50);
    case 'o': return rgb(205, 100, 30);
    case 'W': return rgb(255, 240, 220);
    case 'P': return rgb(255, 140, 170);
    case 'E': return rgb(20, 20, 20);
    default:  return 0;
  }
}

enum Mode { M_SIT, M_WALK, M_PLAY, M_JUMP, M_ZOOM, M_SLEEP,
            M_STRETCH, M_GROOM, M_SPIN, M_TOWALL, M_CLIMB };
static Mode mode = M_SIT;

// climbing the wall: up, cling on for a moment, then slide back down
static float climbH = 0, climbTarget = 0;
static int climbPhase = 0;      // 0 up, 1 clinging, 2 sliding down
static int climbSide = 1;       // 1 = right wall, -1 = left wall
static int paceLeft = 0;        // restless laps still to do
static uint32_t lastAttention = 0, lastSpinFlip = 0;
static float x = 40, height = 0, vy = 0;   // height = how far above the floor
static int dir = 1;                        // 1 = facing right
static float targetX = 40;
static uint32_t modeUntil = 2000, zoomUntil = 0;
static uint32_t sleepUntil = 0;            // nap end time, 0 = asleep for the night
static float stepTime = 0;
static bool stepFrame = false;
static uint32_t nextBlink = 3000, blinkUntil = 0;
static uint32_t tapTimes[5];
static int tapIdx = 0;

static float ballX = 95, ballV = 0, ballSpin = 0;
static bool night = false;

// weather outside the window
enum Weather { W_CLEAR, W_PARTLY, W_CLOUDY, W_FOG, W_RAIN, W_SNOW, W_STORM };
static Weather weather = W_CLEAR;
static float tempC = 0;
static bool haveTemp = false;

// Every tickle unwinds a bit more yarn across the room. Each strand fades away
// after a while, so it takes LOTS of quick tickling to make a total mess -
// then the whole screen floods pink... and it all gets tidied up
static const int MESS_MAX = 30;
static const uint32_t STRAND_HOLD_MS = 6000, STRAND_FADE_MS = 4000;
struct Strand { int8_t x0, y0, cx, cy, x1, y1; bool dark; uint32_t born; };
static Strand strands[MESS_MAX];
static int mess = 0;
enum MessPhase { MP_NONE, MP_FLOOD, MP_CLEAR };
static MessPhase messPhase = MP_NONE;
static uint32_t messAt = 0;
static const uint32_t FLOOD_MS = 1500, PINK_MS = 1500, CLEAR_MS = 1000;

struct Heart { float x, y, life; };
static Heart hearts[8];

static char sayText[24];
static uint32_t sayUntil = 0;

struct Bit { float x, y, speed; uint16_t col; };
static Bit confetti[20];
static bool confettiOn = false;

static float t = 0;
static uint32_t lastMs = 0;

static void addHeart() {
  for (Heart &h : hearts) {
    if (h.life <= 0) {
      h = { x + CAT_W / 2 + (float)random(-12, 13), (float)(FLOOR_Y - CAT_H - height - 4), 1.2f };
      return;
    }
  }
}

static void sitFor(uint32_t ms) {
  mode = M_SIT;
  modeUntil = millis() + ms;
}

static void walkTo(float tx, Mode m = M_WALK) {
  targetX = constrain(tx, 0.0f, (float)MAX_X);
  dir = targetX >= x ? 1 : -1;
  mode = m;
}

static void jump(float speed) {
  if (mode == M_SLEEP) return;
  if (mode == M_CLIMB) {   // startled off the wall
    climbPhase = 2;
    return;
  }
  if (height <= 0) vy = speed;
  if (mode != M_ZOOM) mode = M_JUMP;
}

// A new strand of yarn from the ball to somewhere random in the room
static void addStrand() {
  if (messPhase != MP_NONE || mess >= MESS_MAX) return;
  int bx = ballX, by = FLOOR_Y - 5;
  int ex = random(0, 128), ey = random(4, 126);
  strands[mess++] = { (int8_t)(bx / 2), (int8_t)(by / 2),
                      (int8_t)(random(0, 128) / 2), (int8_t)(random(0, 128) / 2),
                      (int8_t)(ex / 2), (int8_t)(ey / 2), (bool)random(2), millis() };
  if (mess >= MESS_MAX) {   // total mess - flood the screen!
    messPhase = MP_FLOOD;
    messAt = millis();
  }
}

void catTickle() {
  uint32_t now = millis();
  lastAttention = now;
  paceLeft = 0;
  tapTimes[tapIdx] = now;
  tapIdx = (tapIdx + 1) % 5;
  addHeart();
  addHeart();

  uint32_t oldest = tapTimes[tapIdx];
  if (oldest && now - oldest < 3000 && mode != M_ZOOM) {   // 5 quick tickles = zoomies!
    memset(tapTimes, 0, sizeof tapTimes);
    zoomUntil = now + 4000;
    walkTo(dir > 0 ? MAX_X : 0, M_ZOOM);
    return;
  }

  // pounce! the yarn goes flying and unwinds a bit more
  jump(150);
  ballV = (random(2) ? 1 : -1) * random(60, 120);
  addStrand();
}

void catHappy() {
  jump(130);
  addHeart();
}

void catSleep() {
  mode = M_SLEEP;
  sleepUntil = 0;
  height = 0;
}

void catWake() {
  vy = 120;   // startled little hop
  mode = M_JUMP;
}

bool catSleeping() { return mode == M_SLEEP; }

void catSay(const char *text, uint32_t ms) {
  strlcpy(sayText, text, sizeof sayText);
  sayUntil = millis() + ms;
}

void catSetNight(bool n) { night = n; }

// WMO weather codes, as used by Open-Meteo
void catSetWeather(int code, float t) {
  tempC = t;
  haveTemp = true;
  if (code <= 1) weather = W_CLEAR;
  else if (code == 2) weather = W_PARTLY;
  else if (code == 3) weather = W_CLOUDY;
  else if (code == 45 || code == 48) weather = W_FOG;
  else if ((code >= 71 && code <= 77) || code == 85 || code == 86) weather = W_SNOW;
  else if (code >= 95) weather = W_STORM;
  else weather = W_RAIN;   // drizzle, rain, showers
}

void catConfetti(bool on) {
  if (on && !confettiOn) {
    const uint16_t cols[] = { rgb(255, 80, 140), rgb(255, 210, 40), rgb(80, 220, 255), rgb(120, 255, 120), rgb(200, 120, 255) };
    for (Bit &b : confetti) b = { (float)random(128), (float)random(128), (float)random(15, 40), cols[random(5)] };
  }
  confettiOn = on;
}

void catUpdate() {
  uint32_t now = millis();
  float dt = min(0.1f, (now - lastMs) / 1000.0f);
  lastMs = now;
  t += dt;

  // jumping (also works mid-zoomies)
  if (height > 0 || vy > 0) {
    height += vy * dt;
    vy -= 450 * dt;
    if (height <= 0) {
      height = 0;
      vy = 0;
      if (mode == M_JUMP) sitFor(1500);
    }
  }

  switch (mode) {
    case M_SIT:
      if (now >= modeUntil) {
        // nobody's played with her for a while? She gets restless
        bool restless = now - lastAttention > 90000;
        int r = random(restless ? 21 : 17);
        if (r < 4) walkTo(random(0, MAX_X + 1));
        else if (r < 7) walkTo(ballX < x + CAT_W / 2 ? ballX - 6 : ballX - CAT_W + 6, M_PLAY);   // go play with the yarn
        else if (r < 8) sitFor(random(2000, 4000));
        else if (r < 9 && !restless) {   // a little cat nap
          mode = M_SLEEP;
          sleepUntil = now + random(10000, 20000);
        } else if (r < 10) {             // a big stretch
          mode = M_STRETCH;
          modeUntil = now + random(1500, 2500);
        } else if (r < 12) {             // wash a paw
          mode = M_GROOM;
          modeUntil = now + random(2000, 3500);
        } else if (r < 13) {             // chase her tail
          mode = M_SPIN;
          modeUntil = now + 2000;
        } else if (r < 17) {             // climb the wall
          walkTo(x > MAX_X / 2 ? MAX_X : 0, M_TOWALL);
        } else {                         // restless pacing back and forth
          paceLeft = 3;
          walkTo(x > MAX_X / 2 ? 0 : MAX_X);
          catSay(random(2) ? "pet me?" : "mew!", 1500);
        }
      }
      break;

    case M_SLEEP:
      if (sleepUntil && now >= sleepUntil) {   // nap over - wake up with a stretch
        mode = M_STRETCH;
        modeUntil = now + 1800;
      }
      break;

    case M_STRETCH:
    case M_GROOM:
      if (now >= modeUntil) sitFor(random(1500, 3000));
      break;

    case M_SPIN:   // chasing her tail: spin round with little hops
      if (now - lastSpinFlip > 130) {
        dir = -dir;
        lastSpinFlip = now;
        if (height <= 0) vy = 60;
      }
      if (now >= modeUntil) sitFor(2000);
      break;

    case M_CLIMB:
      stepTime += dt;
      if (climbPhase == 0) {
        climbH += 22 * dt;
        if (stepTime > 0.16f) { stepTime = 0; stepFrame = !stepFrame; }
        if (climbH >= climbTarget) {
          climbPhase = 1;
          modeUntil = now + random(800, 1800);
        }
      } else if (climbPhase == 1) {
        if (now >= modeUntil) climbPhase = 2;
      } else {
        climbH -= 90 * dt;
        if (climbH <= 0) {   // back on the floor, facing into the room
          climbH = 0;
          x = climbSide > 0 ? MAX_X : 0;
          dir = -climbSide;
          sitFor(2000);
        }
      }
      break;

    case M_WALK:
    case M_PLAY:
    case M_TOWALL:
    case M_ZOOM: {
      float speed = mode == M_ZOOM ? 95 : paceLeft > 0 ? 50 : 24;
      x += dir * speed * dt;
      stepTime += dt;
      if (stepTime > (mode == M_ZOOM ? 0.07f : 0.16f)) {
        stepTime = 0;
        stepFrame = !stepFrame;
      }
      if ((dir > 0 && x >= targetX) || (dir < 0 && x <= targetX)) {
        x = targetX;
        if (mode == M_ZOOM) {
          if (now < zoomUntil) walkTo(dir > 0 ? 0 : MAX_X, M_ZOOM);   // turn round and keep going
          else sitFor(2500);
        } else if (mode == M_PLAY) {
          dir = ballX > x + CAT_W / 2 ? 1 : -1;
          ballV = dir * random(60, 110);   // bat the yarn!
          sitFor(random(1500, 3000));
        } else if (mode == M_TOWALL) {     // at the wall - up she goes
          mode = M_CLIMB;
          climbPhase = 0;
          climbH = 0;
          climbTarget = random(18, 40);
          climbSide = x > MAX_X / 2 ? 1 : -1;
        } else if (paceLeft > 0) {         // restless: turn and pace back
          paceLeft--;
          walkTo(x > MAX_X / 2 ? 0 : MAX_X);
        } else {
          sitFor(random(2000, 5000));
        }
      }
      break;
    }

    default:
      break;
  }
  x = constrain(x, 0.0f, (float)MAX_X);

  // the yarn ball rolls and slows down, bouncing off the walls
  ballX += ballV * dt;
  ballSpin += ballV * dt / 6;
  ballV *= 1 - 1.2f * dt;
  if (ballX < 6) { ballX = 6; ballV = fabsf(ballV) * 0.6f; }
  if (ballX > 122) { ballX = 122; ballV = -fabsf(ballV) * 0.6f; }
  if (fabsf(ballV) < 2) ballV = 0;

  // old strands fade away and disappear
  if (messPhase == MP_NONE) {
    int keep = 0;
    for (int i = 0; i < mess; i++) {
      if (now - strands[i].born < STRAND_HOLD_MS + STRAND_FADE_MS) strands[keep++] = strands[i];
    }
    mess = keep;
  }

  // the big pink flood, then tidy up
  if (messPhase == MP_FLOOD && now - messAt > FLOOD_MS + PINK_MS) {
    messPhase = MP_CLEAR;
    messAt = now;
    mess = 0;   // fresh ball of yarn
    ballV = 0;
  } else if (messPhase == MP_CLEAR && now - messAt > CLEAR_MS) {
    messPhase = MP_NONE;
    catSay("all tidy!", 2000);
    catHappy();
  }

  if (now >= nextBlink) {
    blinkUntil = now + 150;
    nextBlink = now + random(2500, 6000);
  }

  for (Heart &h : hearts) {
    if (h.life > 0) {
      h.y -= 22 * dt;
      h.life -= dt;
    }
  }

  if (confettiOn) {
    for (Bit &b : confetti) {
      b.y += b.speed * dt;
      if (b.y > 130) { b.y = -4; b.x = random(128); }
    }
  }
}

static void drawSprite(GFXcanvas16 &c, const char *const *rows, int px, int py, bool flip, bool blink) {
  for (int r = 0; r < 12; r++) {
    for (int col = 0; col < 16; col++) {
      char ch = rows[r][flip ? 15 - col : col];
      if (ch == '.') continue;
      if (blink && ch == 'E') ch = 'O';
      c.fillRect(px + col * S, py + r * S, S, S, colourOf(ch));
    }
  }
}

// The window glass: x 11-42, y 13-38
static const int WX = 11, WY = 13, WW = 32, WH = 26;

static void drawCloud(GFXcanvas16 &c, int cx, int cy, uint16_t col) {
  c.fillCircle(cx - 5, cy + 1, 4, col);
  c.fillCircle(cx, cy - 2, 5, col);
  c.fillCircle(cx + 5, cy + 1, 4, col);
  c.fillRect(cx - 5, cy + 1, 11, 4, col);
}

static void drawWindow(GFXcanvas16 &c) {
  bool grey = weather >= W_CLOUDY;
  bool flash = weather == W_STORM && fmodf(t, 4.0f) < 0.12f;   // lightning!
  uint16_t sky = flash ? WHITE
               : night ? NIGHT
               : grey  ? rgb(125, 145, 170)
               :         DAY_SKY;
  c.fillRect(8, 10, 38, 32, FRAME);
  c.fillRect(WX, WY, WW, WH, sky);

  if (weather <= W_PARTLY) {   // sun or moon
    if (night) {
      c.fillCircle(34, 21, 5, MOON);
      c.fillCircle(31, 19, 5, NIGHT);   // crescent
      c.drawPixel(16, 18, WHITE);
      c.drawPixel(22, 30, WHITE);
      c.drawPixel(38, 33, WHITE);
      c.drawPixel(19, 26, WHITE);
    } else {
      c.fillCircle(33, 22, 5, SUN);
    }
  }

  uint16_t cloud = night ? rgb(70, 75, 100) : grey ? rgb(200, 205, 215) : WHITE;
  float drift = fmodf(t * 2, 44) - 6;   // clouds drift slowly across
  if (weather == W_PARTLY) drawCloud(c, WX + (int)drift % WW, 30, cloud);
  if (weather >= W_CLOUDY && weather != W_FOG) {
    drawCloud(c, WX + 8, 18, cloud);
    drawCloud(c, WX + 24, 20, cloud);
  }

  if (weather == W_RAIN || weather == W_STORM) {
    for (int i = 0; i < 9; i++) {
      int rx = WX + (i * 7) % WW;
      int ry = WY + 8 + (int)(t * 60 + i * 11) % (WH - 8);
      c.drawLine(rx, ry, rx - 1, ry + 3, rgb(120, 170, 255));
    }
  }
  if (weather == W_SNOW) {
    for (int i = 0; i < 12; i++) {
      int sx = WX + (int)(i * 5 + sinf(t * 2 + i) * 2 + WW) % WW;
      int sy = WY + (int)(t * 12 + i * 9) % WH;
      c.fillRect(sx, sy, 2, 2, WHITE);
    }
  }
  if (weather == W_FOG) {
    for (int i = 0; i < 4; i++) {
      int fy = WY + 4 + i * 6;
      int off = (int)(t * 4 + i * 8) % 10;
      c.drawFastHLine(WX + off, fy, WW - 10, rgb(215, 220, 225));
      c.drawFastHLine(WX + 10 - off, fy + 2, WW - 10, rgb(190, 195, 205));
    }
  }

  c.drawFastVLine(27, WY, WH, FRAME);
  c.drawFastHLine(WX, 26, WW, FRAME);

  if (haveTemp) {   // temperature on the window sill
    char buf[8];
    snprintf(buf, sizeof buf, "%d%cC", (int)roundf(tempC), 0xF8);
    c.setTextSize(1);
    c.setTextColor(rgb(230, 220, 240));
    c.setCursor(50, 30);
    c.print(buf);
  }
}

// Climbing: the walking kitten turned on her side, head up, paws against the wall
static void drawClimbing(GFXcanvas16 &c, const char *const *rows, int px, int py, bool leftWall) {
  for (int r = 0; r < 12; r++) {
    for (int col = 0; col < 16; col++) {
      char ch = rows[r][col];
      if (ch == '.') continue;
      int sx = leftWall ? 11 - r : r;
      c.fillRect(px + sx * S, py + (15 - col) * S, S, S, colourOf(ch));
    }
  }
}

static void drawRoom(GFXcanvas16 &c) {
  c.fillScreen(WALL);
  drawWindow(c);

  // floor boards
  c.fillRect(0, FLOOR_Y, 128, 128 - FLOOR_Y, FLOOR);
  c.drawFastHLine(0, FLOOR_Y, 128, FLOOR_L);
  c.drawFastHLine(0, FLOOR_Y + 9, 128, FLOOR_D);
  for (int bx = 20; bx < 128; bx += 40) c.drawFastVLine(bx, FLOOR_Y + 1, 8, FLOOR_D);
  for (int bx = 0; bx < 128; bx += 40) c.drawFastVLine(bx, FLOOR_Y + 10, 8, FLOOR_D);
}

void catDraw(GFXcanvas16 &c) {
  drawRoom(c);

  if (confettiOn) {
    for (const Bit &b : confetti) c.fillRect(b.x, b.y, 3, 3, b.col);
  }

  // unwound yarn all over the room, fading out as it gets old
  for (int i = 0; i < mess; i++) {
    const Strand &s = strands[i];
    uint32_t age = millis() - s.born;
    float fade = age < STRAND_HOLD_MS ? 1 : 1 - (float)(age - STRAND_HOLD_MS) / STRAND_FADE_MS;
    uint16_t col = blend(WALL, s.dark ? YARN_D : YARN, constrain(fade, 0.0f, 1.0f));
    float px = s.x0 * 2, py = s.y0 * 2;
    for (int k = 1; k <= 10; k++) {   // a gentle curve from the ball out into the room
      float u = k / 10.0f, v = 1 - u;
      float nx = v * v * s.x0 * 2 + 2 * v * u * s.cx * 2 + u * u * s.x1 * 2;
      float ny = v * v * s.y0 * 2 + 2 * v * u * s.cy * 2 + u * u * s.y1 * 2;
      c.drawLine(px, py, nx, ny, col);
      c.drawLine(px, py + 1, nx, ny + 1, col);
      px = nx;
      py = ny;
    }
  }

  // yarn ball - gets smaller as it unwinds
  int by = FLOOR_Y - 5;
  int br = 5 - mess * 3 / MESS_MAX;
  c.fillCircle(ballX, by, br, YARN);
  for (int k = 0; k < 2; k++) {
    float a = ballSpin + k * 1.6f;
    c.drawLine(ballX - cosf(a) * br, by - sinf(a) * br, ballX + cosf(a) * br, by + sinf(a) * br, YARN_D);
  }

  // kitten
  const char *const *frame = SIT;
  bool blink = millis() < blinkUntil;
  if (mode == M_SLEEP) frame = SLEEP;
  else if (height > 0) frame = JUMP;
  else if (mode == M_WALK || mode == M_PLAY || mode == M_ZOOM || mode == M_TOWALL) frame = stepFrame ? WALK2 : WALK1;
  else if (mode == M_STRETCH) frame = STRETCH;
  else if (mode == M_GROOM) {
    frame = (millis() / 250) % 2 ? GROOM1 : GROOM2;
    blink = true;   // eyes shut while she washes
  }
  int cy = FLOOR_Y - CAT_H - (int)height;

  if (mode == M_CLIMB) {
    const char *const *climbFrame = climbPhase == 0 ? (stepFrame ? WALK2 : WALK1) : WALK1;
    int px = climbSide > 0 ? 128 - CAT_H : 0;
    cy = FLOOR_Y - CAT_W - (int)climbH;
    drawClimbing(c, climbFrame, px, cy, climbSide < 0);
  } else {
    drawSprite(c, frame, x, cy, dir < 0, blink);
  }

  if (confettiOn && mode != M_SLEEP && mode != M_CLIMB && mode != M_STRETCH) {   // birthday party hat
    int hx = x + (dir > 0 ? 12 * S : 4 * S);
    c.fillTriangle(hx - 5, cy + 1, hx + 5, cy + 1, hx, cy - 14, rgb(80, 200, 255));
    c.drawLine(hx - 3, cy - 3, hx + 3, cy - 5, rgb(255, 220, 60));
    c.drawLine(hx - 2, cy - 8, hx + 2, cy - 9, rgb(255, 220, 60));
    c.fillCircle(hx, cy - 14, 2, rgb(255, 80, 140));
  }

  if (mode == M_SLEEP) {   // z z z
    c.setTextColor(WHITE);
    for (int i = 0; i < 3; i++) {
      float ph = fmodf(t * 0.4f + i / 3.0f, 1.0f);
      c.setTextSize(ph < 0.5f ? 1 : 2);
      c.setCursor(x + (dir > 0 ? CAT_W - 10 : 4) + ph * 10, cy + 8 - ph * 30);
      c.print('z');
    }
  }

  for (const Heart &h : hearts) {
    if (h.life <= 0) continue;
    for (int r = 0; r < 4; r++)
      for (int col = 0; col < 5; col++)
        if (HEART[r][col] == 'P') c.fillRect(h.x + col * 2, h.y + r * 2, 2, 2, colourOf('P'));
  }

  // the pink flood spreads out from the ball, then wipes away
  uint32_t e = millis() - messAt;
  if (messPhase == MP_FLOOD) {
    int r = e < FLOOD_MS ? 190 * e / FLOOD_MS : 190;
    c.fillCircle(ballX, by, r, YARN);
    if (e > FLOOD_MS) {
      c.setTextSize(3);
      c.setTextColor(YARN_D);
      c.setCursor(19, 52);
      c.print("oops!");
    }
  } else if (messPhase == MP_CLEAR) {
    c.fillRect(0, 0, 128, 128 - 128 * e / CLEAR_MS, YARN);
  }

  if ((int32_t)(sayUntil - millis()) > 0) {
    int w = strlen(sayText) * 6 + 10;
    c.fillRoundRect(64 - w / 2, 46, w, 14, 6, WHITE);
    c.setTextSize(1);
    c.setTextColor(INK);
    c.setCursor(64 - w / 2 + 5, 49);
    c.print(sayText);
  }
}
