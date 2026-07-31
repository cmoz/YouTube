#include "mode_glow.h"
#include "leds/glow_engine.h"
#include <string.h>
#include <stdio.h>

#if LV_FONT_UNSCII_16
#define GLOW_FONT (&lv_font_unscii_16)
#else
#define GLOW_FONT LV_FONT_DEFAULT
#endif

#define GLOW_BODY_FONT LV_FONT_DEFAULT

#define GLOW_COLOR_BG    lv_color_hex(0x0a0a0a)
#define GLOW_COLOR_TXT   lv_color_hex(0x9a9a9a)
#define GLOW_COLOR_ACT   lv_color_hex(0xd4823a)
#define GLOW_COLOR_LINE  lv_color_hex(0x2a2a2a)
#define GLOW_COLOR_ROW   lv_color_hex(0x1c1c1c)

#define GLOW_MAX_FX 16   /* headroom above glow_effect_count() */

/* preset swatches offered for uses_color effects */
static const uint32_t s_swatches[] = {
    0xFF2A78, 0xFFFFFF, 0xFF2020, 0x20FF60, 0x2080FF, 0xFFA020,
};
#define GLOW_SWATCH_COUNT (sizeof(s_swatches) / sizeof(s_swatches[0]))

static const char *s_morse_presets[] = { "SOS", "HELLO", "TINKER TAILOR" };
#define GLOW_MORSE_PRESET_COUNT (sizeof(s_morse_presets) / sizeof(s_morse_presets[0]))

static struct {
    lv_obj_t *panel;
    lv_obj_t *row[GLOW_MAX_FX];
    lv_obj_t *row_label[GLOW_MAX_FX];
    int       active;

    lv_obj_t *name_lbl;
    lv_obj_t *desc_lbl;

    lv_obj_t *color_row;
    lv_obj_t *speed_slider;
    lv_obj_t *speed_val;
    lv_obj_t *bright_slider;
    lv_obj_t *bright_val;
    lv_obj_t *morse_row;
} s;

static void refresh_side_panel(void)
{
    const glow_effect_t *fx = glow_effect_at(s.active);
    if (!fx) return;

    lv_label_set_text(s.name_lbl, fx->name);
    lv_label_set_text(s.desc_lbl, fx->description);

    if (fx->uses_color) {
        lv_obj_clear_flag(s.color_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s.color_row, LV_OBJ_FLAG_HIDDEN);
    }

    if (strcmp(fx->name, "Morse") == 0) {
        lv_obj_clear_flag(s.morse_row, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s.morse_row, LV_OBJ_FLAG_HIDDEN);
    }
}

static void select_effect(int idx)
{
    if (idx == s.active) return;
    s.active = idx;
    glow_set_effect(idx);

    int count = glow_effect_count();
    for (int i = 0; i < count && i < GLOW_MAX_FX; i++) {
        lv_color_t c = (i == idx) ? GLOW_COLOR_ACT : GLOW_COLOR_TXT;
        lv_obj_set_style_text_color(s.row_label[i], c, 0);
        lv_obj_set_style_border_color(s.row[i], (i == idx) ? GLOW_COLOR_ACT : GLOW_COLOR_LINE, 0);
    }

    refresh_side_panel();
}

static void row_event_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    select_effect(idx);
}

static void swatch_event_cb(lv_event_t *e)
{
    uint32_t rgb = (uint32_t)(intptr_t)lv_event_get_user_data(e);
    glow_set_color(rgb);
}

static void speed_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    int32_t v = lv_slider_get_value(slider);
    glow_set_speed((uint8_t)v);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(s.speed_val, buf);
}

static void bright_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    int32_t v = lv_slider_get_value(slider);
    glow_set_brightness((uint8_t)v);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    lv_label_set_text(s.bright_val, buf);
}

static void morse_preset_event_cb(lv_event_t *e)
{
    const char *msg = (const char *)lv_event_get_user_data(e);
    glow_set_message(msg);
}

static lv_obj_t *make_slider_row(lv_obj_t *parent, const char *label, int y,
                                  int32_t min, int32_t max, int32_t val,
                                  lv_event_cb_t cb, lv_obj_t **val_lbl_out)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, GLOW_COLOR_TXT, 0);
    lv_obj_set_style_text_font(lbl, GLOW_BODY_FONT, 0);
    lv_obj_set_pos(lbl, 0, y);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_size(slider, 300, 16);
    lv_obj_set_pos(slider, 0, y + 24);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, GLOW_COLOR_ROW, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, GLOW_COLOR_ACT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, GLOW_COLOR_ACT, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *v = lv_label_create(parent);
    lv_obj_set_pos(v, 312, y + 22);
    lv_obj_set_style_text_color(v, GLOW_COLOR_ACT, 0);
    lv_obj_set_style_text_font(v, GLOW_BODY_FONT, 0);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)val);
    lv_label_set_text(v, buf);

    if (val_lbl_out) *val_lbl_out = v;
    return slider;
}

static void panel_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    memset(&s, 0, sizeof(s));
}

lv_obj_t *mode_glow_create(lv_obj_t *parent)
{
    memset(&s, 0, sizeof(s));
    s.active = -1;   /* sentinel so the first select_effect() always runs */

    s.panel = lv_obj_create(parent);
    lv_obj_remove_style_all(s.panel);
    lv_obj_set_size(s.panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s.panel, GLOW_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s.panel, panel_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *title = lv_label_create(s.panel);
    lv_label_set_text(title, "CMOZGLOW");
    lv_obj_set_style_text_font(title, GLOW_FONT, 0);
    lv_obj_set_style_text_color(title, GLOW_COLOR_ACT, 0);
    lv_obj_set_pos(title, 12, 8);

    lv_obj_t *subtitle = lv_label_create(s.panel);
    lv_label_set_text(subtitle, "cmoz/CMozGlow, ported to this strip -- tap an effect to preview it");
    lv_obj_set_style_text_font(subtitle, GLOW_BODY_FONT, 0);
    lv_obj_set_style_text_color(subtitle, GLOW_COLOR_TXT, 0);
    lv_obj_set_pos(subtitle, 12, 36);

    lv_obj_t *hairline = lv_obj_create(s.panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(96), 1);
    lv_obj_set_style_bg_color(hairline, GLOW_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_set_pos(hairline, 12, 62);

    /* ── left: scrollable effect list ─────────────────────────── */
    lv_obj_t *list = lv_obj_create(s.panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 300, 460);
    lv_obj_set_pos(list, 12, 66);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_style_pad_row(list, 6, 0);

    int count = glow_effect_count();
    int y = 0;
    for (int i = 0; i < count && i < GLOW_MAX_FX; i++) {
        const glow_effect_t *fx = glow_effect_at(i);
        if (!fx) continue;

        lv_obj_t *row = lv_obj_create(list);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), 40);
        lv_obj_set_pos(row, 0, y);
        lv_obj_set_style_bg_color(row, GLOW_COLOR_ROW, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(row, GLOW_COLOR_LINE, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row, row_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s.row[i] = row;

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, fx->name);
        lv_obj_set_style_text_font(lbl, GLOW_BODY_FONT, 0);
        lv_obj_set_style_text_color(lbl, GLOW_COLOR_TXT, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 10, 0);
        s.row_label[i] = lbl;

        y += 46;
    }

    /* ── right: description + controls ────────────────────────── */
    lv_obj_t *side = lv_obj_create(s.panel);
    lv_obj_remove_style_all(side);
    lv_obj_set_size(side, 620, 460);
    lv_obj_set_pos(side, 336, 76);
    lv_obj_clear_flag(side, LV_OBJ_FLAG_SCROLLABLE);

    s.name_lbl = lv_label_create(side);
    lv_obj_set_style_text_font(s.name_lbl, GLOW_FONT, 0);
    lv_obj_set_style_text_color(s.name_lbl, GLOW_COLOR_ACT, 0);
    lv_obj_set_pos(s.name_lbl, 0, 0);

    s.desc_lbl = lv_label_create(side);
    lv_label_set_long_mode(s.desc_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s.desc_lbl, lv_pct(100));
    lv_obj_set_style_text_font(s.desc_lbl, GLOW_BODY_FONT, 0);
    lv_obj_set_style_text_color(s.desc_lbl, GLOW_COLOR_TXT, 0);
    lv_obj_set_pos(s.desc_lbl, 0, 30);

    /* colour swatches (hidden for uses_color == false effects) */
    s.color_row = lv_obj_create(side);
    lv_obj_remove_style_all(s.color_row);
    lv_obj_set_size(s.color_row, lv_pct(100), 44);
    lv_obj_set_pos(s.color_row, 0, 70);
    lv_obj_clear_flag(s.color_row, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0; i < GLOW_SWATCH_COUNT; i++) {
        lv_obj_t *sw = lv_obj_create(s.color_row);
        lv_obj_remove_style_all(sw);
        lv_obj_set_size(sw, 40, 40);
        lv_obj_set_pos(sw, (int)i * 48, 0);
        lv_obj_set_style_bg_color(sw, lv_color_hex(s_swatches[i]), 0);
        lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(sw, GLOW_COLOR_LINE, 0);
        lv_obj_set_style_border_width(sw, 1, 0);
        lv_obj_set_style_radius(sw, 4, 0);
        lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sw, swatch_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)s_swatches[i]);
    }

    s.speed_slider  = make_slider_row(side, "Speed (1 dreamy -- 10 party)", 132, 1, 10, 5,
                                       speed_event_cb, &s.speed_val);
    s.bright_slider = make_slider_row(side, "Brightness", 196, 0, 255, 160,
                                       bright_event_cb, &s.bright_val);

    /* Morse message presets (hidden unless the Morse effect is active) */
    s.morse_row = lv_obj_create(side);
    lv_obj_remove_style_all(s.morse_row);
    lv_obj_set_size(s.morse_row, lv_pct(100), 44);
    lv_obj_set_pos(s.morse_row, 0, 260);
    lv_obj_set_flex_flow(s.morse_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s.morse_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s.morse_row, 12, 0);
    lv_obj_clear_flag(s.morse_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *morse_lbl = lv_label_create(s.morse_row);
    lv_label_set_text(morse_lbl, "Message:");
    lv_obj_set_style_text_color(morse_lbl, GLOW_COLOR_TXT, 0);
    lv_obj_set_style_text_font(morse_lbl, GLOW_BODY_FONT, 0);

    for (uint32_t i = 0; i < GLOW_MORSE_PRESET_COUNT; i++) {
        lv_obj_t *btn = lv_obj_create(s.morse_row);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, 34);
        lv_obj_set_style_bg_color(btn, GLOW_COLOR_ROW, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, GLOW_COLOR_ACT, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_pad_hor(btn, 10, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, morse_preset_event_cb, LV_EVENT_CLICKED, (void *)s_morse_presets[i]);

        lv_obj_t *bl = lv_label_create(btn);
        lv_label_set_text(bl, s_morse_presets[i]);
        lv_obj_set_style_text_color(bl, GLOW_COLOR_ACT, 0);
        lv_obj_set_style_text_font(bl, GLOW_BODY_FONT, 0);
        lv_obj_center(bl);
    }

    /* land on whatever effect the engine is already running (e.g. left
     * over from a previous visit to this tab) rather than forcing Solid */
    select_effect(glow_get_effect());

    return s.panel;
}
