#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "ws2812.h"   /* adjust if your header has a different filename */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t frame;                        /* increments every rendered frame */
    uint32_t color;                        /* packed 0xRRGGBB                 */
    uint8_t  speed;                        /* 1 = dreamy … 10 = party         */
    uint8_t  fb[WS2812_LED_COUNT][3];      /* effect output, pre-gamma        */
    uint8_t  scratch[WS2812_LED_COUNT * 4];/* per-effect persistent state     */
} glow_ctx_t;

typedef struct {
    const char *name;
    const char *description;
    bool        uses_color;                /* false for Ember and Aurora      */
    void      (*init)(glow_ctx_t *c);      /* called once when selected       */
    void      (*render)(glow_ctx_t *c);    /* called every frame              */
} glow_effect_t;

int                 glow_effect_count(void);
const glow_effect_t *glow_effect_at(int index);

esp_err_t glow_engine_start(void);

void     glow_set_effect(int index);
int      glow_get_effect(void);
void     glow_set_color(uint32_t rgb);
void     glow_set_speed(uint8_t speed);          /* clamped 1-10   */
bool     glow_set_message(const char *msg);      /* Morse effect: A-Z, 0-9, spaces, <=63 chars */
void     glow_set_brightness(uint8_t b);         /* 0-255          */
void     glow_set_power_budget_ma(uint16_t ma);  /* 0 = unlimited  */
uint16_t glow_estimated_current_ma(void);
bool     glow_power_limited(void);

/* Lets another owner (a mode screen) drive the strip directly, through
 * the engine's own lock, instead of calling ws2812_* itself. The engine
 * is the only thing that ever touches the hardware; while external
 * control is held, the engine's own ambient effect is paused so the two
 * can't race or fight over the last frame. Colors are pushed raw, with
 * no gamma/brightness/power-budget shaping.
 *
 * Usage: call glow_external_set_pixel for each LED, then
 * glow_external_commit to push the frame. When done, clear the frame
 * and commit once more, then call glow_external_release to hand the
 * strip back to the ambient effect. */
void glow_external_set_pixel(int index, uint8_t r, uint8_t g, uint8_t b);
void glow_external_clear(void);
void glow_external_commit(void);
void glow_external_release(void);

#ifdef __cplusplus
}
#endif