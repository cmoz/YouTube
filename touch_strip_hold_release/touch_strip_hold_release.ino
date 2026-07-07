// Touch-driven external NeoPixel strip (FastLED) for Circuit Playground Classic
//
// Behavior:
//   - Touch and HOLD a pad -> that pad's animation plays continuously
//   - RELEASE the pad -> animation stops (or resumes a different pad still held)
//   - Touch a NEW pad while one is already held -> the new pad's animation
//     takes over immediately (interrupt), and releasing it falls back to
//     whichever pad is still held, if any
//
// Touch sensing uses CircuitPlayground.readCap() (same pins as before).
// Undefines ENCODER to avoid a FastLED naming conflict.

#include <Adafruit_CircuitPlayground.h>
#ifdef ENCODER
#undef ENCODER
#endif
#include <FastLED.h>

#define STRIP_PIN    2
#define STRIP_COUNT  38
CRGB leds[STRIP_COUNT];
const uint8_t BRIGHTNESS = 140;

// Touch mapping - pin 2 is reserved for the NeoPixel strip (STRIP_PIN) and
// is intentionally excluded here to avoid the pinMode conflict.
const int touchPins[] = {10, 9, 6, 12, 0, 3};
const int NUM_TOUCH = sizeof(touchPins) / sizeof(touchPins[0]);

// Calibration and smoothing
uint32_t baseline[NUM_TOUCH];
float smoothVal[NUM_TOUCH];
const int CAL_SAMPLES = 20;
const float SMOOTH_ALPHA = 0.25;          // smoothing factor (0..1)
const uint16_t SATURATION_VALUE = 60000;  // treat >= this as invalid/stuck reading

// Hysteresis: press threshold is higher than release threshold so a finger
// resting right at the edge does not cause rapid on/off flicker.
const int ON_DELTA  = 60;   // delta above baseline that counts as "pressed"
const int OFF_DELTA = 30;   // delta must fall below this to count as "released"

bool padHeld[NUM_TOUCH] = {false};

// Stack of currently held pads, most recently pressed at the end.
// The pad at the top of the stack is always the active/animating one.
int holdStack[NUM_TOUCH];
int holdStackSize = 0;

enum Anim { NONE, RAINBOW, SPARKLE, PULSE_BLUE, THEATER, WIPE_RED, WIPE_GREEN };
Anim currentAnim = NONE;
int activePad = -1;
unsigned long lastFrame = 0;
uint16_t animFrame = 0;

// touchPins = {10, 9, 6, 12, 0, 3}
Anim animForPad(int idx) {
  switch (idx) {
    case 0: return RAINBOW;      // pin 10
    case 1: return SPARKLE;      // pin 9
    case 2: return PULSE_BLUE;   // pin 6
    case 3: return THEATER;      // pin 12
    case 4: return WIPE_RED;     // pin 0
    case 5: return WIPE_GREEN;   // pin 3
    default: return RAINBOW;
  }
}

int freqForPad(int idx) {
  switch (idx) {
    case 0: return 523;  // pin 10
    case 1: return 587;  // pin 9
    case 2: return 659;  // pin 6
    case 3: return 698;  // pin 12
    case 4: return 784;  // pin 0
    case 5: return 880;  // pin 3
    default: return 0;
  }
}

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();

  FastLED.addLeds<NEOPIXEL, STRIP_PIN>(leds, STRIP_COUNT);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  Serial.println("Calibrating touch baselines - do not touch pads...");
  for (int i = 0; i < NUM_TOUCH; i++) {
    uint32_t sum = 0;
    for (int s = 0; s < CAL_SAMPLES; s++) {
      uint16_t v = CircuitPlayground.readCap(touchPins[i]);
      if (v >= SATURATION_VALUE) v = 0;
      sum += v;
      delay(10);
    }
    baseline[i] = sum / CAL_SAMPLES;
    smoothVal[i] = baseline[i];
    Serial.print("pad "); Serial.print(touchPins[i]);
    Serial.print(" baseline="); Serial.println(baseline[i]);
  }
  Serial.println("Ready.");

  // quick startup flash to confirm strip works
  fill_solid(leds, STRIP_COUNT, CRGB::Blue);
  FastLED.show();
  delay(250);
  clearStrip();
}

void loop() {
  updateTouch();
  runAnimation();
  delay(6);
}

void updateTouch() {
  for (int i = 0; i < NUM_TOUCH; i++) {
    uint16_t raw = CircuitPlayground.readCap(touchPins[i]);
    bool saturated = (raw >= SATURATION_VALUE);

    if (!saturated) {
      smoothVal[i] = SMOOTH_ALPHA * raw + (1.0 - SMOOTH_ALPHA) * smoothVal[i];
    } else {
      static unsigned long lastSatReport[NUM_TOUCH] = {0};
      unsigned long now = millis();
      if (now - lastSatReport[i] > 1000) {
        Serial.print("WARNING: pad "); Serial.print(touchPins[i]);
        Serial.println(" reading saturated (>=60000). Check wiring/sensor.");
        lastSatReport[i] = now;
      }
    }

    int delta = saturated ? 0 : (int)smoothVal[i] - (int)baseline[i];

    if (!padHeld[i] && delta >= ON_DELTA) {
      // new press
      padHeld[i] = true;
      pushHold(i);
      Serial.print("PRESS pad "); Serial.println(touchPins[i]);
      int freq = freqForPad(i);
      if (freq > 0) CircuitPlayground.playTone(freq, 100);
      activatePad(i);
    } else if (padHeld[i] && delta < OFF_DELTA) {
      // release
      padHeld[i] = false;
      popHold(i);
      Serial.print("RELEASE pad "); Serial.println(touchPins[i]);
      if (holdStackSize > 0) {
        activatePad(holdStack[holdStackSize - 1]); // fall back to still-held pad
      } else {
        stopAnimation();
      }
    }
  }
}

void pushHold(int idx) {
  for (int i = 0; i < holdStackSize; i++) {
    if (holdStack[i] == idx) return; // already tracked
  }
  if (holdStackSize < NUM_TOUCH) {
    holdStack[holdStackSize++] = idx;
  }
}

void popHold(int idx) {
  int writeIdx = 0;
  for (int i = 0; i < holdStackSize; i++) {
    if (holdStack[i] != idx) {
      holdStack[writeIdx++] = holdStack[i];
    }
  }
  holdStackSize = writeIdx;
}

void activatePad(int idx) {
  activePad = idx;
  currentAnim = animForPad(idx);
  animFrame = 0;
  lastFrame = millis();
  clearStrip();
}

void stopAnimation() {
  activePad = -1;
  currentAnim = NONE;
  clearStrip();
}

void runAnimation() {
  if (currentAnim == NONE) return;
  unsigned long now = millis();

  switch (currentAnim) {
    case RAINBOW:
      if (now - lastFrame >= 18) {
        lastFrame = now;
        for (int i = 0; i < STRIP_COUNT; i++) {
          uint8_t hue = (uint8_t)((i * 256 / STRIP_COUNT) + animFrame);
          leds[i] = CHSV(hue, 255, 255);
        }
        FastLED.show();
        animFrame++;
      }
      break;

    case SPARKLE:
      if (now - lastFrame >= 40) {
        lastFrame = now;
        fadeToBlackBy(leds, STRIP_COUNT, 40);
        int p = random(STRIP_COUNT);
        leds[p] = CRGB::White;
        FastLED.show();
      }
      break;

    case PULSE_BLUE:
      if (now - lastFrame >= 10) {
        lastFrame = now;
        uint8_t v = beatsin8(40, 20, 255, 0, 0);
        for (int i = 0; i < STRIP_COUNT; i++) leds[i] = CRGB(0, 0, v);
        FastLED.show();
      }
      break;

    case THEATER:
      if (now - lastFrame >= 90) {
        lastFrame = now;
        for (int i = 0; i < STRIP_COUNT; i++) {
          if ((i + animFrame) % 3 == 0) leds[i] = CRGB(255, 0, 0);
          else leds[i] = CRGB::Black;
        }
        FastLED.show();
        animFrame++;
      }
      break;

    case WIPE_RED:
    case WIPE_GREEN:
      if (now - lastFrame >= 10) {
        lastFrame = now;
        CRGB c = (currentAnim == WIPE_RED) ? CRGB(255, 0, 0) : CRGB(0, 255, 0);
        int pos = animFrame % (STRIP_COUNT * 2);
        if (pos < STRIP_COUNT) {
          leds[pos] = c;
        } else {
          leds[(STRIP_COUNT * 2 - 1) - pos] = CRGB::Black;
        }
        FastLED.show();
        animFrame++;
      }
      break;

    default:
      break;
  }
}

void clearStrip() {
  fill_solid(leds, STRIP_COUNT, CRGB::Black);
  FastLED.show();
}
