#include "mode_demo.h"

#include "leds/ws2812.h"
#include "ui/ui_shell.h"

#if LV_FONT_MONTSERRAT_32
#define DEMO_FONT_TITLE (&lv_font_montserrat_32)
#else
#define DEMO_FONT_TITLE LV_FONT_DEFAULT
#endif

#define DEMO_COLOR_BG      lv_color_hex(0x0a0a0a)
#define DEMO_COLOR_MUTED   lv_color_hex(0x8a8a8a)

#define DEMO_TICK_MS         40    /* ~25fps */
#define DEMO_HUE_STEP_DEG    2     /* full rotation every ~7.2s */
#define DEMO_SWATCH_COUNT    8
#define DEMO_SWATCH_HUE_STEP 45    /* 360 / DEMO_SWATCH_COUNT */
#define DEMO_GLITCH_PERIOD_MS 3200
#define DEMO_BALL_ANIM_MS    1600
#define DEMO_BALL_RANGE      380   /* +/- px from center; tuned for the ~1008px content area */
#define DEMO_BALL_Y_OFFSET   90

static struct {
    lv_obj_t *root;
    lv_obj_t *title;
    lv_obj_t *swatch[DEMO_SWATCH_COUNT];
    lv_obj_t *ball;
    lv_timer_t *tick_timer;
    lv_timer_t *glitch_timer;
    uint16_t base_hue;
} s;

static void tick_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    s.base_hue = (s.base_hue + DEMO_HUE_STEP_DEG) % 360;

    lv_obj_set_style_text_color(s.title, lv_color_hsv_to_rgb(s.base_hue, 70, 95), 0);

    for (int i = 0; i < DEMO_SWATCH_COUNT; i++) {
        uint16_t hue = (s.base_hue + i * DEMO_SWATCH_HUE_STEP) % 360;
        lv_obj_set_style_bg_color(s.swatch[i], lv_color_hsv_to_rgb(hue, 85, 70), 0);
    }

    lv_color_t ball_color = lv_color_hsv_to_rgb((s.base_hue * 2) % 360, 90, 95);
    lv_obj_set_style_bg_color(s.ball, ball_color, 0);

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        uint16_t hue = (s.base_hue + (i * 360) / WS2812_LED_COUNT) % 360;
        lv_color_t c = lv_color_hsv_to_rgb(hue, 100, 45);
        ws2812_set_pixel(i, c.red, c.green, c.blue);
    }
    ws2812_refresh();
}

static void glitch_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    ui_shell_glitch_trigger();
}

static void ball_anim_cb(void *var, int32_t v)
{
    lv_obj_align((lv_obj_t *)var, LV_ALIGN_CENTER, v, DEMO_BALL_Y_OFFSET);
}

static void root_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);

    if (s.tick_timer) {
        lv_timer_del(s.tick_timer);
        s.tick_timer = NULL;
    }
    if (s.glitch_timer) {
        lv_timer_del(s.glitch_timer);
        s.glitch_timer = NULL;
    }
    ws2812_clear();
    ws2812_refresh();
    s.root = NULL;
}

lv_obj_t *mode_demo_create(lv_obj_t *parent)
{
    s.base_hue = 0;

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, DEMO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    s.root = root;

    s.title = lv_label_create(root);
    lv_label_set_text(s.title, "DEMO MODE");
    lv_obj_set_style_text_font(s.title, DEMO_FONT_TITLE, 0);
    lv_obj_set_style_text_letter_space(s.title, 2, 0);
    lv_obj_align(s.title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, "SCREEN + LED COLOR SHOWCASE");
    lv_obj_set_style_text_color(subtitle, DEMO_COLOR_MUTED, 0);
    lv_obj_set_style_text_letter_space(subtitle, 2, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 66);

    lv_obj_t *swatch_row = lv_obj_create(root);
    lv_obj_remove_style_all(swatch_row);
    lv_obj_set_size(swatch_row, LV_SIZE_CONTENT, 84);
    lv_obj_set_flex_flow(swatch_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(swatch_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(swatch_row, 12, 0);
    lv_obj_clear_flag(swatch_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(swatch_row, LV_ALIGN_CENTER, 0, -20);

    for (int i = 0; i < DEMO_SWATCH_COUNT; i++) {
        lv_obj_t *swatch = lv_obj_create(swatch_row);
        lv_obj_remove_style_all(swatch);
        lv_obj_set_size(swatch, 84, 84);
        lv_obj_set_style_radius(swatch, 8, 0);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
        s.swatch[i] = swatch;
    }

    s.ball = lv_obj_create(root);
    lv_obj_remove_style_all(s.ball);
    lv_obj_set_size(s.ball, 28, 28);
    lv_obj_set_style_radius(s.ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s.ball, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s.ball, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(s.ball, LV_ALIGN_CENTER, -DEMO_BALL_RANGE, DEMO_BALL_Y_OFFSET);

    lv_anim_t ball_anim;
    lv_anim_init(&ball_anim);
    lv_anim_set_var(&ball_anim, s.ball);
    lv_anim_set_values(&ball_anim, -DEMO_BALL_RANGE, DEMO_BALL_RANGE);
    lv_anim_set_time(&ball_anim, DEMO_BALL_ANIM_MS);
    lv_anim_set_playback_time(&ball_anim, DEMO_BALL_ANIM_MS);
    lv_anim_set_repeat_count(&ball_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&ball_anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&ball_anim, ball_anim_cb);
    lv_anim_start(&ball_anim);

    s.tick_timer = lv_timer_create(tick_timer_cb, DEMO_TICK_MS, NULL);
    s.glitch_timer = lv_timer_create(glitch_timer_cb, DEMO_GLITCH_PERIOD_MS, NULL);

    return root;
}
