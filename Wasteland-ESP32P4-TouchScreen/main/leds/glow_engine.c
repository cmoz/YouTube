#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_random.h"
#include "glow_engine.h"

static const char *TAG = "glow";

#define GLOW_FPS        50
#define GLOW_FRAME_MS   (1000 / GLOW_FPS)

static glow_ctx_t        s_ctx;
static SemaphoreHandle_t s_lock;
static int               s_effect      = 0;
static uint8_t           s_brightness  = 160;
static uint16_t          s_budget_ma   = 450;
static uint16_t          s_est_ma      = 0;
static bool              s_limited     = false;
static uint8_t           s_gamma[256];
static bool               s_external    = false;
static uint8_t            s_ext_fb[WS2812_LED_COUNT][3];
static char               s_message[64] = "SOS";

/* Ported from CMozGlow (github.com/cmoz/CMozGlow) — same algorithms,
 * rewritten against this engine's fb/scratch model instead of the
 * library's own setPixel()/show(). CMozGlow gates each effect's
 * animation state to its own per-effect interval (scaled by speed) and
 * only re-renders that often; this engine's task always ticks at
 * GLOW_FPS, so step_due() reproduces that gating — the render function
 * still runs every tick, but effect *state* (fades, steps, sparks) only
 * advances when step_due() says it's due, holding the last frame
 * in between. Effects that used a continuous time formula in the
 * original (Heartbeat, Silk, Aurora) don't need gating and just use
 * elapsed engine time directly. */

/* scratch layout: [0..3] next-due frame, [4..7] tick counter, both
 * owned by step_due(); effect-private state starts at scratch+8. */
static bool step_due(glow_ctx_t *c, uint16_t base_ms, uint32_t *ticks_out)
{
    uint32_t *next  = (uint32_t *)(c->scratch + 0);
    uint32_t *ticks = (uint32_t *)(c->scratch + 4);
    if (c->frame < *next) {
        if (ticks_out) *ticks_out = *ticks;
        return false;
    }
    uint32_t interval_ms = ((uint32_t)base_ms * (13 - c->speed)) / 8;
    if (interval_ms < GLOW_FRAME_MS) interval_ms = GLOW_FRAME_MS;
    uint32_t interval_frames = interval_ms / GLOW_FRAME_MS;
    if (interval_frames < 1) interval_frames = 1;
    *next = c->frame + interval_frames;
    (*ticks)++;
    if (ticks_out) *ticks_out = *ticks;
    return true;
}

/* ── effect 0: Solid ─────────────────────────────────────────── */
static void fx_solid_render(glow_ctx_t *c)
{
    uint8_t r = (c->color >> 16) & 0xFF;
    uint8_t g = (c->color >>  8) & 0xFF;
    uint8_t b =  c->color        & 0xFF;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        c->fb[i][0] = r;
        c->fb[i][1] = g;
        c->fb[i][2] = b;
    }
}

/* ── effect 1: Sequin — dims slowly, random glints flash to white ── */
static void fx_sequin_render(glow_ctx_t *c)
{
    if (!step_due(c, 45, NULL)) return;

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        for (int ch = 0; ch < 3; ch++) {
            c->fb[i][ch] = (uint8_t)((c->fb[i][ch] * 225) / 255);
        }
    }

    uint8_t sparks = 1 + WS2812_LED_COUNT / 24;
    uint8_t r = (c->color >> 16) & 0xFF, g = (c->color >> 8) & 0xFF, b = c->color & 0xFF;
    /* glint = colour pushed toward white. CMozGlow does this with a
     * bitwise OR (r | 0xB0), but that's lossy -- e.g. 0x20 and 0xA0 both
     * OR to 0xB0, so distinct swatches can produce an identical glint
     * (0xFF2020 and 0xFFA020 both landed on (255,176,176)). A
     * proportional blend keeps the glint effect but preserves hue. */
    for (int s = 0; s < sparks; s++) {
        if ((esp_random() % 100) < 45) {
            int i = esp_random() % WS2812_LED_COUNT;
            c->fb[i][0] = (uint8_t)(r + (((255 - r) * 0xB0) >> 8));
            c->fb[i][1] = (uint8_t)(g + (((255 - g) * 0xB0) >> 8));
            c->fb[i][2] = (uint8_t)(b + (((255 - b) * 0xB0) >> 8));
        }
    }
}

/* ── effect 2: Catwalk — bold head sweeps back and forth, fading train ── */
static void fx_catwalk_render(glow_ctx_t *c)
{
    uint32_t ticks;
    if (!step_due(c, 35, &ticks)) return;

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        for (int ch = 0; ch < 3; ch++) {
            c->fb[i][ch] = (uint8_t)((c->fb[i][ch] * 200) / 255);
        }
    }

    uint32_t span = (WS2812_LED_COUNT - 1) * 2;
    uint32_t s    = ticks % span;
    int head = (s < WS2812_LED_COUNT) ? (int)s : (int)(span - s);

    c->fb[head][0] = (c->color >> 16) & 0xFF;
    c->fb[head][1] = (c->color >>  8) & 0xFF;
    c->fb[head][2] =  c->color        & 0xFF;
    if (head + 1 < WS2812_LED_COUNT) {
        c->fb[head + 1][0] = ((c->color >> 16) & 0xFF) >> 1;
        c->fb[head + 1][1] = ((c->color >>  8) & 0xFF) >> 1;
        c->fb[head + 1][2] = ( c->color        & 0xFF) >> 1;
    }
}

/* ── effect 3: Loom — two threads weave past each other ─────────── */
static void fx_loom_render(glow_ctx_t *c)
{
    uint32_t ticks;
    if (!step_due(c, 120, &ticks)) return;

    uint32_t a = c->color;
    uint32_t b = ((a & 0xFF) << 16) | (a & 0x00FF00) | ((a >> 16) & 0xFF); /* rotated hue */

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        uint32_t col = (((i + ticks) / 3) & 1) ? a : b;
        c->fb[i][0] = (col >> 16) & 0xFF;
        c->fb[i][1] = (col >>  8) & 0xFF;
        c->fb[i][2] =  col        & 0xFF;
    }
}

/* ── effect 4: Heartbeat — lub-DUB double pulse ──────────────────── */
static void fx_heartbeat_render(glow_ctx_t *c)
{
    uint32_t t = (c->frame * GLOW_FRAME_MS) % 1500;
    uint16_t e = 0;
    if (t < 150)                   e = 255 - (t * 255) / 150;
    else if (t >= 250 && t < 420)  e = 170 - ((t - 250) * 170) / 170;

    uint8_t r = (((c->color >> 16) & 0xFF) * e) >> 8;
    uint8_t g = (((c->color >>  8) & 0xFF) * e) >> 8;
    uint8_t b = (( c->color        & 0xFF) * e) >> 8;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        c->fb[i][0] = r; c->fb[i][1] = g; c->fb[i][2] = b;
    }
}

/* ── effect 5: Firefly — soft lights breathe in and out at random ── */
#define FLY_COUNT 6
typedef struct { uint16_t pos; uint16_t phase; uint8_t rate; uint8_t alive; } fly_t;
_Static_assert(sizeof(((glow_ctx_t *)0)->scratch) >= 8 + sizeof(fly_t) * FLY_COUNT,
               "glow_ctx_t.scratch too small for Firefly state at this WS2812_LED_COUNT");

static void fx_firefly_render(glow_ctx_t *c)
{
    fly_t flies[FLY_COUNT];
    memcpy(flies, c->scratch + 8, sizeof(flies));

    uint32_t ticks;
    bool due = step_due(c, 30, &ticks);

    if (due) {
        for (int i = 0; i < FLY_COUNT; i++) {
            if (!flies[i].alive && (esp_random() % 100) < 6) {
                flies[i].alive = 1;
                flies[i].pos   = esp_random() % WS2812_LED_COUNT;
                flies[i].phase = 0;
                flies[i].rate  = 2 + (esp_random() % 5);
            }
        }
    }

    memset(c->fb, 0, sizeof(c->fb));
    uint8_t r = (c->color >> 16) & 0xFF, g = (c->color >> 8) & 0xFF, b = c->color & 0xFF;
    for (int i = 0; i < FLY_COUNT; i++) {
        if (!flies[i].alive) continue;
        if (due) {
            flies[i].phase += flies[i].rate * 40;
            if (flies[i].phase >= 32768) { flies[i].alive = 0; continue; }
        }
        uint16_t ph = flies[i].phase >> 6;             /* 0..511 */
        uint8_t  e  = (ph < 256) ? ph : (511 - ph);     /* triangle envelope */
        c->fb[flies[i].pos][0] = (r * e) >> 8;
        c->fb[flies[i].pos][1] = (g * e) >> 8;
        c->fb[flies[i].pos][2] = (b * e) >> 8;
    }

    memcpy(c->scratch + 8, flies, sizeof(flies));
}

/* ── effect 6: Silk — slow shimmering sheen rolling down the strip ── */
static void fx_silk_render(glow_ctx_t *c)
{
    float t = (c->frame * GLOW_FRAME_MS) * 0.0025f;
    uint8_t cr = (c->color >> 16) & 0xFF, cg = (c->color >> 8) & 0xFF, cb = c->color & 0xFF;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        float s = 0.62f + 0.38f * sinf(i * 0.45f + t);   /* never fully dark */
        c->fb[i][0] = (uint8_t)(cr * s);
        c->fb[i][1] = (uint8_t)(cg * s);
        c->fb[i][2] = (uint8_t)(cb * s);
    }
}

/* ── effect 7: Ember — random walk through campfire colours ─────── */
static void fx_ember_render(glow_ctx_t *c)
{
    uint8_t *heat = c->scratch + 8;   /* WS2812_LED_COUNT bytes */

    if (step_due(c, 45, NULL)) {
        for (int i = 0; i < WS2812_LED_COUNT; i++) {
            int16_t h = heat[i] + ((int)(esp_random() % 29) - 14);
            if (h < 40)  h = 40;
            if (h > 255) h = 255;
            heat[i] = (uint8_t)h;
        }
    }

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        uint8_t h = heat[i];
        c->fb[i][0] = h;
        c->fb[i][1] = (h * 78) >> 8;
        c->fb[i][2] = (h > 200) ? (h - 200) / 4 : 0;
    }
}

/* ── effect 8: Aurora — drifting curtains of green, teal, violet ── */
static void fx_aurora_render(glow_ctx_t *c)
{
    float t = (c->frame * GLOW_FRAME_MS) * 0.001f;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        float w1 = sinf(i * 0.30f + t * 1.1f);
        float w2 = sinf(i * 0.13f - t * 0.7f + 1.7f);
        float v  = (w1 + w2) * 0.25f + 0.5f;             /* 0..1 */
        uint8_t g = (uint8_t)(30 + 170 * v);
        uint8_t b = (uint8_t)(20 + 120 * (1.0f - v) * v * 4.0f * 0.6f);
        uint8_t r = (v < 0.25f) ? (uint8_t)(90 * (0.25f - v) * 4.0f) : 0;
        c->fb[i][0] = r; c->fb[i][1] = g; c->fb[i][2] = b;
    }
}

/* ── effect 9: Morse — blinks s_message in real Morse code ──────── */
static void fx_morse_render(glow_ctx_t *c)
{
    static const char *CODE[36] = {
        ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---",      /* A-J */
        "-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",        /* K-T */
        "..-","...-",".--","-..-","-.--","--..",                          /* U-Z */
        "-----",".----","..---","...--","....-",".....",                  /* 0-5 */
        "-....","--...","---..","----."                                   /* 6-9 */
    };

    uint32_t *next     = (uint32_t *)(c->scratch + 8);
    uint8_t  *char_idx = c->scratch + 12;
    uint8_t  *elem_idx = c->scratch + 13;
    uint8_t  *on       = c->scratch + 14;

    if (c->frame < *next) return;

    uint32_t unit_ms     = 60 + (10 - c->speed) * 30;
    uint32_t unit_frames = unit_ms / GLOW_FRAME_MS;
    if (unit_frames < 1) unit_frames = 1;

    char ch = s_message[*char_idx];
    if (ch == 0) {
        memset(c->fb, 0, sizeof(c->fb));
        *on = 0; *char_idx = 0; *elem_idx = 0;
        *next = c->frame + unit_frames * 10;
        return;
    }
    ch = (char)toupper((unsigned char)ch);
    if (ch == ' ') {
        memset(c->fb, 0, sizeof(c->fb));
        (*char_idx)++; *elem_idx = 0;
        *next = c->frame + unit_frames * 7;
        return;
    }
    const char *pat = (ch >= 'A' && ch <= 'Z') ? CODE[ch - 'A']
                     : (ch >= '0' && ch <= '9') ? CODE[26 + (ch - '0')]
                     : NULL;
    if (!pat) { (*char_idx)++; *next = c->frame + 1; return; }

    if (!*on) {
        char e = pat[*elem_idx];
        uint8_t r = (c->color >> 16) & 0xFF, g = (c->color >> 8) & 0xFF, b = c->color & 0xFF;
        for (int i = 0; i < WS2812_LED_COUNT; i++) {
            c->fb[i][0] = r; c->fb[i][1] = g; c->fb[i][2] = b;
        }
        *on = 1;
        *next = c->frame + unit_frames * ((e == '-') ? 3 : 1);
    } else {
        memset(c->fb, 0, sizeof(c->fb));
        *on = 0;
        (*elem_idx)++;
        if (pat[*elem_idx] == 0) {
            *elem_idx = 0; (*char_idx)++;
            *next = c->frame + unit_frames * 3;
        } else {
            *next = c->frame + unit_frames;
        }
    }
}

/* ── effect table (CMozGlow effect order, for parity) ────────────── */
static const glow_effect_t s_effects[] = {
    { "Solid",     "steady colour",                              true,  NULL, fx_solid_render     },
    { "Sequin",    "random glints, like sequins catching light", true,  NULL, fx_sequin_render    },
    { "Catwalk",   "a bold sweep back and forth, fading train",  true,  NULL, fx_catwalk_render   },
    { "Loom",      "two threads weaving past each other",        true,  NULL, fx_loom_render      },
    { "Heartbeat", "realistic lub-dub double pulse",              true,  NULL, fx_heartbeat_render },
    { "Firefly",   "soft fireflies blinking in and out",          true,  NULL, fx_firefly_render   },
    { "Silk",      "slow shimmering sheen rolling down the strip",true,  NULL, fx_silk_render      },
    { "Ember",     "warm campfire glow",                          false, NULL, fx_ember_render     },
    { "Aurora",    "northern lights",                             false, NULL, fx_aurora_render    },
    { "Morse",     "blinks a real Morse-code message",            true,  NULL, fx_morse_render     },
};

#define GLOW_EFFECT_TOTAL (sizeof(s_effects) / sizeof(s_effects[0]))

int glow_effect_count(void) { return (int)GLOW_EFFECT_TOTAL; }

const glow_effect_t *glow_effect_at(int index)
{
    if (index < 0 || index >= (int)GLOW_EFFECT_TOTAL) return NULL;
    return &s_effects[index];
}

/* ── output pipeline: gamma -> brightness -> power budget ────── */
static void glow_commit(void)
{
    uint8_t  out[WS2812_LED_COUNT][3];
    uint32_t sum = 0;

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        for (int ch = 0; ch < 3; ch++) {
            uint32_t v = s_gamma[s_ctx.fb[i][ch]];
            v = (v * s_brightness) / 255;
            out[i][ch] = (uint8_t)v;
            sum += v;
        }
    }

    /* ~20 mA per fully-lit channel, ~1 mA idle per chip */
    uint32_t est = WS2812_LED_COUNT + (sum * 20) / 255;
    s_limited = false;

    if (s_budget_ma > 0 && est > s_budget_ma) {
        s_limited = true;
        uint32_t scale = ((uint32_t)s_budget_ma * 255) / est;
        if (scale > 255) scale = 255;
        sum = 0;
        for (int i = 0; i < WS2812_LED_COUNT; i++) {
            for (int ch = 0; ch < 3; ch++) {
                out[i][ch] = (uint8_t)((out[i][ch] * scale) / 255);
                sum += out[i][ch];
            }
        }
        est = WS2812_LED_COUNT + (sum * 20) / 255;
    }

    s_est_ma = (uint16_t)est;

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_pixel(i, out[i][0], out[i][1], out[i][2]);
    }
    ws2812_refresh();
}

/* ── render task ─────────────────────────────────────────────── */
static void glow_task(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        if (!s_external) {
            if (s_effects[s_effect].render) {
                s_effects[s_effect].render(&s_ctx);
            }
            glow_commit();
            s_ctx.frame++;
        }
        xSemaphoreGive(s_lock);

        vTaskDelayUntil(&last, pdMS_TO_TICKS(GLOW_FRAME_MS));
    }
}

esp_err_t glow_engine_start(void)
{
    for (int i = 0; i < 256; i++) {
        s_gamma[i] = (uint8_t)(powf((float)i / 255.0f, 2.8f) * 255.0f + 0.5f);
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.color = 0xFF2A78;   /* the pink from your README example */
    s_ctx.speed = 5;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    if (s_effects[s_effect].init) s_effects[s_effect].init(&s_ctx);

    if (xTaskCreate(glow_task, "glow", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "render task failed to start");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "engine running, %d effects", (int)GLOW_EFFECT_TOTAL);
    return ESP_OK;
}

/* ── setters, safe from the LVGL task ────────────────────────── */
void glow_set_effect(int index)
{
    if (index < 0 || index >= (int)GLOW_EFFECT_TOTAL) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_effect    = index;
    s_ctx.frame = 0;
    memset(s_ctx.fb, 0, sizeof(s_ctx.fb));
    memset(s_ctx.scratch, 0, sizeof(s_ctx.scratch));
    if (s_effects[index].init) s_effects[index].init(&s_ctx);
    xSemaphoreGive(s_lock);
}

int  glow_get_effect(void) { return s_effect; }

void glow_set_color(uint32_t rgb)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ctx.color = rgb;
    xSemaphoreGive(s_lock);
}

void glow_set_speed(uint8_t speed)
{
    if (speed < 1)  speed = 1;
    if (speed > 10) speed = 10;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ctx.speed = speed;
    xSemaphoreGive(s_lock);
}

/* A-Z, 0-9, spaces only, up to 63 chars — anything else is rejected so
 * the Morse effect never has to cope with an unsupported character. */
bool glow_set_message(const char *msg)
{
    if (!msg || !*msg || strlen(msg) > 63) return false;
    for (const char *p = msg; *p; p++) {
        char u = (char)toupper((unsigned char)*p);
        if (!(u == ' ' || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9'))) {
            return false;
        }
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    strncpy(s_message, msg, sizeof(s_message) - 1);
    s_message[sizeof(s_message) - 1] = 0;
    xSemaphoreGive(s_lock);
    return true;
}

void glow_set_brightness(uint8_t b)         { s_brightness = b; }
void glow_set_power_budget_ma(uint16_t ma)  { s_budget_ma  = ma; }
uint16_t glow_estimated_current_ma(void)    { return s_est_ma; }
bool glow_power_limited(void)               { return s_limited; }

/* ── external control, for mode screens that draw the strip directly ── */
void glow_external_set_pixel(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index < 0 || index >= WS2812_LED_COUNT) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ext_fb[index][0] = r;
    s_ext_fb[index][1] = g;
    s_ext_fb[index][2] = b;
    xSemaphoreGive(s_lock);
}

void glow_external_clear(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memset(s_ext_fb, 0, sizeof(s_ext_fb));
    xSemaphoreGive(s_lock);
}

void glow_external_commit(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_external = true;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_pixel(i, s_ext_fb[i][0], s_ext_fb[i][1], s_ext_fb[i][2]);
    }
    ws2812_refresh();
    xSemaphoreGive(s_lock);
}

void glow_external_release(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_external = false;
    xSemaphoreGive(s_lock);
}