#include "mode_tools.h"
#include <string.h>

#if LV_FONT_UNSCII_16
#define TOOLS_FONT (&lv_font_unscii_16)
#else
#define TOOLS_FONT LV_FONT_DEFAULT
#endif

#define TOOLS_BODY_FONT LV_FONT_DEFAULT
#define TOOLS_BIG_FONT  (&lv_font_montserrat_20)  /* resistor/power calc rows -- roomier than the default 14px */
#define TOOLS_INNER_W   660                        /* centered content width for those two tabs */

#define TOOLS_COLOR_BG    lv_color_hex(0x0a0a0a)
#define TOOLS_COLOR_TXT   lv_color_hex(0x9a9a9a)
#define TOOLS_COLOR_ACT   lv_color_hex(0xd4823a)
#define TOOLS_COLOR_LINE  lv_color_hex(0x2a2a2a)

#include <stdio.h>

typedef enum {
    TOOLS_TAB_RESISTOR = 0,
    TOOLS_TAB_POWER,
    TOOLS_TAB_4BAND,
    TOOLS_TAB_5BAND,
    TOOLS_TAB_COUNT
} tools_tab_t;

static const char *s_tab_labels[TOOLS_TAB_COUNT] = { "  resistor", "power", "4-band", "5-band" };

static struct {
    lv_obj_t *panel;
    lv_obj_t *subnav;
    lv_obj_t *tab_label[TOOLS_TAB_COUNT];
    lv_obj_t *content_area;
    lv_obj_t *content;
    tools_tab_t active_tab;
} s;


/* ---- LED resistor calc state ---- */
static float s_res_vcc   = 5.0f;
static float s_res_vf    = 2.0f;
static float s_res_if_ma = 20.0f;
static lv_obj_t *s_res_result_lbl = NULL;
static lv_obj_t *s_res_vcc_val    = NULL;
static lv_obj_t *s_res_vf_val     = NULL;
static lv_obj_t *s_res_if_val     = NULL;

static void update_resistor_result(void)
{
    if (!s_res_result_lbl) return;
    float r = (s_res_vcc - s_res_vf) / (s_res_if_ma / 1000.0f);
    float std = r < 100  ? 100  : r < 220  ? 220  : r < 330  ? 330
              : r < 470  ? 470  : r < 560  ? 560  : r < 680  ? 680
              : r < 820  ? 820  : r < 1000 ? 1000 : r;
    char buf[48];
    snprintf(buf, sizeof(buf), "R = %.0f ohm  (use %.0f ohm)", r, std);
    lv_label_set_text(s_res_result_lbl, buf);

    char vbuf[16];
    if (s_res_vcc_val) { snprintf(vbuf, sizeof(vbuf), "%.1fV",  s_res_vcc);   lv_label_set_text(s_res_vcc_val, vbuf); }
    if (s_res_vf_val)  { snprintf(vbuf, sizeof(vbuf), "%.1fV",  s_res_vf);    lv_label_set_text(s_res_vf_val,  vbuf); }
    if (s_res_if_val)  { snprintf(vbuf, sizeof(vbuf), "%.0fmA", s_res_if_ma); lv_label_set_text(s_res_if_val,  vbuf); }
}

static void res_vcc_inc_cb(lv_event_t *e) { LV_UNUSED(e); s_res_vcc += 0.5f; update_resistor_result(); }
static void res_vcc_dec_cb(lv_event_t *e) { LV_UNUSED(e); if (s_res_vcc > 1.0f) { s_res_vcc -= 0.5f; update_resistor_result(); } }
static void res_vf_inc_cb(lv_event_t *e)  { LV_UNUSED(e); s_res_vf += 0.1f; update_resistor_result(); }
static void res_vf_dec_cb(lv_event_t *e)  { LV_UNUSED(e); if (s_res_vf > 0.1f) { s_res_vf -= 0.1f; update_resistor_result(); } }
static void res_if_inc_cb(lv_event_t *e)  { LV_UNUSED(e); s_res_if_ma += 1.0f; update_resistor_result(); }
static void res_if_dec_cb(lv_event_t *e)  { LV_UNUSED(e); if (s_res_if_ma > 1.0f) { s_res_if_ma -= 1.0f; update_resistor_result(); } }

static lv_obj_t *make_spin_row(lv_obj_t *parent, const char *label,
                                lv_event_cb_t dec_cb, lv_event_cb_t inc_cb, int y)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(lbl, TOOLS_BIG_FONT, 0);
    lv_obj_set_pos(lbl, 0, y + 14);

    lv_obj_t *dec = lv_obj_create(parent);
    lv_obj_remove_style_all(dec);
    lv_obj_set_size(dec, 56, 48);
    lv_obj_set_pos(dec, 430, y);
    lv_obj_set_style_bg_color(dec, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(dec, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dec, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(dec, 1, 0);
    lv_obj_add_flag(dec, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dec, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(dec, dec_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *dl = lv_label_create(dec);
    lv_label_set_text(dl, "-");
    lv_obj_set_style_text_color(dl, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(dl, TOOLS_BIG_FONT, 0);
    lv_obj_center(dl);

    lv_obj_t *val = lv_label_create(parent);
    lv_obj_set_size(val, 96, 48);
    lv_obj_set_pos(val, 498, y + 12);
    lv_obj_set_style_text_color(val, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_text_font(val, TOOLS_BIG_FONT, 0);
    lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *inc = lv_obj_create(parent);
    lv_obj_remove_style_all(inc);
    lv_obj_set_size(inc, 56, 48);
    lv_obj_set_pos(inc, 604, y);
    lv_obj_set_style_bg_color(inc, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(inc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(inc, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(inc, 1, 0);
    lv_obj_add_flag(inc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(inc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(inc, inc_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *il = lv_label_create(inc);
    lv_label_set_text(il, "+");
    lv_obj_set_style_text_color(il, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(il, TOOLS_BIG_FONT, 0);
    lv_obj_center(il);

    return val;
}



static lv_obj_t *build_resistor_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *inner = lv_obj_create(panel);
    lv_obj_remove_style_all(inner);
    lv_obj_set_size(inner, TOOLS_INNER_W, LV_SIZE_CONTENT);
    lv_obj_align(inner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *info = lv_label_create(inner);
    lv_label_set_text(info, "R = (Vcc - Vf) / If");
    lv_obj_set_style_text_color(info, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(info, TOOLS_BIG_FONT, 0);
    lv_obj_set_pos(info, 0, 0);

    s_res_vcc_val = make_spin_row(inner, "Vcc (Supply V)", res_vcc_dec_cb, res_vcc_inc_cb, 50);
    s_res_vf_val  = make_spin_row(inner, "Vf  (LED Fwd V)", res_vf_dec_cb,  res_vf_inc_cb,  118);
    s_res_if_val  = make_spin_row(inner, "If  (LED mA)",    res_if_dec_cb,  res_if_inc_cb,  186);

    lv_obj_t *res_box = lv_obj_create(inner);
    lv_obj_remove_style_all(res_box);
    lv_obj_set_size(res_box, TOOLS_INNER_W, 56);
    lv_obj_set_pos(res_box, 0, 262);
    lv_obj_set_style_bg_color(res_box, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(res_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(res_box, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(res_box, 1, 0);
    lv_obj_clear_flag(res_box, LV_OBJ_FLAG_SCROLLABLE);

    s_res_result_lbl = lv_label_create(res_box);
    lv_obj_set_style_text_color(s_res_result_lbl, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_text_font(s_res_result_lbl, TOOLS_BIG_FONT, 0);
    lv_obj_center(s_res_result_lbl);

    lv_obj_t *ref = lv_label_create(inner);
    lv_label_set_text(ref, "Common Vf: Red 1.8V  Green 2.1V  Blue 3.3V  White 3.3V  IR 1.2V");
    lv_obj_set_style_text_color(ref, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(ref, TOOLS_BODY_FONT, 0);
    lv_obj_set_pos(ref, 0, 332);

    update_resistor_result();
    return panel;
}

/* ---- Power calc state ---- */
static float s_pw_volts = 5.0f;
static float s_pw_amps  = 0.5f;
static lv_obj_t *s_pw_result_lbl = NULL;
static lv_obj_t *s_pw_v_val      = NULL;
static lv_obj_t *s_pw_a_val      = NULL;

static void update_power_result(void)
{
    if (!s_pw_result_lbl) return;
    char buf[48];
    snprintf(buf, sizeof(buf), "P = %.2f W   R = %.1f ohm",
             s_pw_volts * s_pw_amps, s_pw_volts / s_pw_amps);
    lv_label_set_text(s_pw_result_lbl, buf);

    char vbuf[16];
    if (s_pw_v_val) { snprintf(vbuf, sizeof(vbuf), "%.1fV", s_pw_volts); lv_label_set_text(s_pw_v_val, vbuf); }
    if (s_pw_a_val) { snprintf(vbuf, sizeof(vbuf), "%.1fA", s_pw_amps);  lv_label_set_text(s_pw_a_val, vbuf); }
}

static void pw_v_inc_cb(lv_event_t *e) { LV_UNUSED(e); s_pw_volts += 0.5f; update_power_result(); }
static void pw_v_dec_cb(lv_event_t *e) { LV_UNUSED(e); if (s_pw_volts > 0.5f) { s_pw_volts -= 0.5f; update_power_result(); } }
static void pw_a_inc_cb(lv_event_t *e) { LV_UNUSED(e); s_pw_amps += 0.1f; update_power_result(); }
static void pw_a_dec_cb(lv_event_t *e) { LV_UNUSED(e); if (s_pw_amps > 0.1f) { s_pw_amps -= 0.1f; update_power_result(); } }

static lv_obj_t *build_power_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *inner = lv_obj_create(panel);
    lv_obj_remove_style_all(inner);
    lv_obj_set_size(inner, TOOLS_INNER_W, LV_SIZE_CONTENT);
    lv_obj_align(inner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_clear_flag(inner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *info = lv_label_create(inner);
    lv_label_set_text(info, "P = V x I        V = I x R");
    lv_obj_set_style_text_color(info, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(info, TOOLS_BIG_FONT, 0);
    lv_obj_set_pos(info, 0, 0);

    s_pw_v_val = make_spin_row(inner, "Voltage (V)", pw_v_dec_cb, pw_v_inc_cb, 50);
    s_pw_a_val = make_spin_row(inner, "Current (A)", pw_a_dec_cb, pw_a_inc_cb, 118);

    lv_obj_t *res_box = lv_obj_create(inner);
    lv_obj_remove_style_all(res_box);
    lv_obj_set_size(res_box, TOOLS_INNER_W, 56);
    lv_obj_set_pos(res_box, 0, 194);
    lv_obj_set_style_bg_color(res_box, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(res_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(res_box, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(res_box, 1, 0);
    lv_obj_clear_flag(res_box, LV_OBJ_FLAG_SCROLLABLE);

    s_pw_result_lbl = lv_label_create(res_box);
    lv_obj_set_style_text_color(s_pw_result_lbl, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_text_font(s_pw_result_lbl, TOOLS_BIG_FONT, 0);
    lv_obj_center(s_pw_result_lbl);

    lv_obj_t *ws = lv_label_create(inner);
    lv_label_set_text(ws,
        "WS2812B: 1 LED full = 60mA @ 5V = 0.3W\n"
        "30 LEDs = 1.8A minimum PSU");
    lv_obj_set_style_text_color(ws, TOOLS_COLOR_TXT, 0);
    lv_obj_set_style_text_font(ws, TOOLS_BODY_FONT, 0);
    lv_obj_set_pos(ws, 0, 268);

    update_power_result();
    return panel;
}

/* ---- Colour code calc state (shared between 4-band and 5-band) ---- */
static lv_obj_t *s_cc_roller[5] = { NULL, NULL, NULL, NULL, NULL };
static lv_obj_t *s_cc_swatch[5] = { NULL, NULL, NULL, NULL, NULL };
static lv_obj_t *s_cc_visual[5] = { NULL, NULL, NULL, NULL, NULL };
static lv_obj_t *s_cc_result_lbl = NULL;
static int s_cc_band_count = 4;

static const char *const CC_DIGIT_OPTIONS = "Black\nBrown\nRed\nOrange\nYellow\nGreen\nBlue\nViolet\nGrey\nWhite";
static const char *const CC_MULT_OPTIONS  = "Black\nBrown\nRed\nOrange\nYellow\nGreen\nBlue\nViolet\nGrey\nWhite\nGold\nSilver";
static const char *const CC_TOL_OPTIONS   = "Brown\nRed\nOrange\nYellow\nGreen\nBlue\nViolet\nGrey\nGold\nSilver\nNone";

static const double cc_mult_values[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    100000000, 1000000000, 0.1, 0.01
};
static const char *const cc_tol_strings[] = {
    "1%", "2%", "3%", "4%", "0.5%", "0.25%", "0.1%", "0.05%", "5%", "10%", "20%"
};

static const uint8_t cc_band_r[] = { 20, 139, 210, 255, 255, 50, 50, 148, 160, 255, 212, 192 };
static const uint8_t cc_band_g[] = { 20, 69, 50, 140, 210, 180, 100, 0, 160, 255, 175, 192 };
static const uint8_t cc_band_b[] = { 20, 19, 50, 0, 0, 80, 220, 211, 160, 255, 55, 192 };

static const uint8_t cc_tol_r[] = { 139, 210, 255, 255, 50, 50, 148, 160, 212, 192, 60 };
static const uint8_t cc_tol_g[] = { 69, 50, 140, 210, 180, 100, 0, 160, 175, 192, 60 };
static const uint8_t cc_tol_b[] = { 19, 50, 0, 0, 80, 220, 211, 160, 55, 192, 60 };

static void update_colour_result(void)
{
    if (!s_cc_result_lbl) return;

    uint16_t b1 = lv_roller_get_selected(s_cc_roller[0]);
    uint16_t b2 = lv_roller_get_selected(s_cc_roller[1]);
    uint16_t b3 = 0, mult = 0, tol = 0;

    if (s_cc_band_count == 4) {
        mult = lv_roller_get_selected(s_cc_roller[2]);
        tol  = lv_roller_get_selected(s_cc_roller[3]);
    } else {
        b3   = lv_roller_get_selected(s_cc_roller[2]);
        mult = lv_roller_get_selected(s_cc_roller[3]);
        tol  = lv_roller_get_selected(s_cc_roller[4]);
    }

    lv_obj_set_style_bg_color(s_cc_swatch[0], lv_color_make(cc_band_r[b1], cc_band_g[b1], cc_band_b[b1]), 0);
    lv_obj_set_style_bg_color(s_cc_visual[0], lv_color_make(cc_band_r[b1], cc_band_g[b1], cc_band_b[b1]), 0);
    lv_obj_set_style_bg_color(s_cc_swatch[1], lv_color_make(cc_band_r[b2], cc_band_g[b2], cc_band_b[b2]), 0);
    lv_obj_set_style_bg_color(s_cc_visual[1], lv_color_make(cc_band_r[b2], cc_band_g[b2], cc_band_b[b2]), 0);

    if (s_cc_band_count == 5) {
        lv_obj_set_style_bg_color(s_cc_swatch[2], lv_color_make(cc_band_r[b3], cc_band_g[b3], cc_band_b[b3]), 0);
        lv_obj_set_style_bg_color(s_cc_visual[2], lv_color_make(cc_band_r[b3], cc_band_g[b3], cc_band_b[b3]), 0);
        lv_obj_set_style_bg_color(s_cc_swatch[3], lv_color_make(cc_band_r[mult], cc_band_g[mult], cc_band_b[mult]), 0);
        lv_obj_set_style_bg_color(s_cc_visual[3], lv_color_make(cc_band_r[mult], cc_band_g[mult], cc_band_b[mult]), 0);
        lv_obj_set_style_bg_color(s_cc_swatch[4], lv_color_make(cc_tol_r[tol], cc_tol_g[tol], cc_tol_b[tol]), 0);
        lv_obj_set_style_bg_color(s_cc_visual[4], lv_color_make(cc_tol_r[tol], cc_tol_g[tol], cc_tol_b[tol]), 0);
    } else {
        lv_obj_set_style_bg_color(s_cc_swatch[2], lv_color_make(cc_band_r[mult], cc_band_g[mult], cc_band_b[mult]), 0);
        lv_obj_set_style_bg_color(s_cc_visual[2], lv_color_make(cc_band_r[mult], cc_band_g[mult], cc_band_b[mult]), 0);
        lv_obj_set_style_bg_color(s_cc_swatch[3], lv_color_make(cc_tol_r[tol], cc_tol_g[tol], cc_tol_b[tol]), 0);
        lv_obj_set_style_bg_color(s_cc_visual[3], lv_color_make(cc_tol_r[tol], cc_tol_g[tol], cc_tol_b[tol]), 0);
    }

    double value = (s_cc_band_count == 4)
        ? (b1 * 10 + b2) * cc_mult_values[mult]
        : (b1 * 100 + b2 * 10 + b3) * cc_mult_values[mult];

    char val_buf[32];
    if (value >= 1000000) snprintf(val_buf, sizeof(val_buf), "%.2f Mohm", value / 1000000.0);
    else if (value >= 1000) snprintf(val_buf, sizeof(val_buf), "%.2f kohm", value / 1000.0);
    else if (value < 1) snprintf(val_buf, sizeof(val_buf), "%.2f ohm", value);
    else snprintf(val_buf, sizeof(val_buf), "%.0f ohm", value);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s  %s", val_buf, cc_tol_strings[tol]);
    lv_label_set_text(s_cc_result_lbl, buf);
}

static void cc_roller_event_cb(lv_event_t *e)
{
    lv_obj_t *roller = lv_event_get_target_obj(e);

    for (int i = 0; i < s_cc_band_count; i++) {
        if (s_cc_visual[i]) {
            lv_obj_set_style_shadow_opa(s_cc_visual[i], LV_OPA_0, 0);
        }
    }
    for (int i = 0; i < s_cc_band_count; i++) {
        if (s_cc_roller[i] == roller) {
            lv_obj_set_style_shadow_width(s_cc_visual[i], 10, 0);
            lv_obj_set_style_shadow_color(s_cc_visual[i], lv_color_white(), 0);
            lv_obj_set_style_shadow_opa(s_cc_visual[i], LV_OPA_40, 0);
        }
    }
    update_colour_result();
}

static void draw_resistor(lv_obj_t *parent, int band_count, lv_obj_t *bands_out[])
{
    lv_obj_t *wire_l = lv_obj_create(parent);
    lv_obj_remove_style_all(wire_l);
    lv_obj_set_size(wire_l, 18, 4);
    lv_obj_align(wire_l, LV_ALIGN_TOP_MID, -116, 82);
    lv_obj_set_style_bg_color(wire_l, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_bg_opa(wire_l, LV_OPA_COVER, 0);

    lv_obj_t *wire_r = lv_obj_create(parent);
    lv_obj_remove_style_all(wire_r);
    lv_obj_set_size(wire_r, 18, 4);
    lv_obj_align(wire_r, LV_ALIGN_TOP_MID, 116, 82);
    lv_obj_set_style_bg_color(wire_r, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_bg_opa(wire_r, LV_OPA_COVER, 0);

    lv_obj_t *body = lv_obj_create(parent);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, 196, 28);
    lv_obj_align(body, LV_ALIGN_TOP_MID, 0, 75);
    lv_obj_set_style_bg_color(body, lv_color_make(210, 185, 130), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(body, 8, 0);

    static const int offsets_4[] = { -68, -28, 12, 52 };
    static const int offsets_5[] = { -78, -38, 2, 42, 82 };
    const int *offsets = (band_count == 4 ? offsets_4 : offsets_5);

    for (int i = 0; i < band_count; i++) {
        bands_out[i] = lv_obj_create(parent);
        lv_obj_remove_style_all(bands_out[i]);
        lv_obj_set_size(bands_out[i], 16, 28);
        lv_obj_align(bands_out[i], LV_ALIGN_TOP_MID, offsets[i], 75);
        lv_obj_set_style_bg_color(bands_out[i], lv_color_make(20, 20, 20), 0);
        lv_obj_set_style_bg_opa(bands_out[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bands_out[i], 3, 0);
    }
}

static void build_colour_calc_ui(lv_obj_t *parent, int band_count)
{
    draw_resistor(parent, band_count, s_cc_visual);

    static const char *labels_4[] = { "Band 1", "Band 2", "Mult", "Tol" };
    static const char *labels_5[] = { "Band 1", "Band 2", "Band 3", "Mult", "Tol" };
    const char **labels = (band_count == 4 ? labels_4 : labels_5);

    int roller_w = (band_count == 4 ? 66 : 56);
    static int roller_cx_4[] = { 60, 132, 204, 276 };
    static int roller_cx_5[] = { 52, 116, 180, 244, 308 };
    int *roller_cx = (band_count == 4 ? roller_cx_4 : roller_cx_5);

    for (int i = 0; i < band_count; i++) {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, TOOLS_COLOR_TXT, 0);
        lv_obj_set_style_text_font(lbl, TOOLS_BODY_FONT, 0);
        lv_obj_set_pos(lbl, roller_cx[i] - roller_w / 2, 116);

        s_cc_swatch[i] = lv_obj_create(parent);
        lv_obj_remove_style_all(s_cc_swatch[i]);
        lv_obj_set_size(s_cc_swatch[i], roller_w, 18);
        lv_obj_set_pos(s_cc_swatch[i], roller_cx[i] - roller_w / 2, 135);
        lv_obj_set_style_bg_color(s_cc_swatch[i], lv_color_make(20, 20, 20), 0);
        lv_obj_set_style_bg_opa(s_cc_swatch[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s_cc_swatch[i], TOOLS_COLOR_ACT, 0);
        lv_obj_set_style_border_width(s_cc_swatch[i], 1, 0);

        s_cc_roller[i] = lv_roller_create(parent);
        lv_obj_set_size(s_cc_roller[i], roller_w, 156);
        lv_obj_set_pos(s_cc_roller[i], roller_cx[i] - roller_w / 2, 157);
        lv_obj_set_style_text_font(s_cc_roller[i], TOOLS_BODY_FONT, 0);
        lv_obj_set_style_bg_color(s_cc_roller[i], lv_color_hex(0x1c1c1c), 0);
        lv_obj_set_style_text_color(s_cc_roller[i], TOOLS_COLOR_TXT, 0);
        lv_obj_set_style_border_color(s_cc_roller[i], TOOLS_COLOR_ACT, 0);
        lv_obj_set_style_border_width(s_cc_roller[i], 1, 0);
        lv_obj_set_style_bg_color(s_cc_roller[i], TOOLS_COLOR_ACT, LV_PART_SELECTED);
        lv_obj_set_style_text_color(s_cc_roller[i], lv_color_black(), LV_PART_SELECTED);

        if (band_count == 4) {
            if (i < 2) lv_roller_set_options(s_cc_roller[i], CC_DIGIT_OPTIONS, LV_ROLLER_MODE_NORMAL);
            else if (i == 2) lv_roller_set_options(s_cc_roller[i], CC_MULT_OPTIONS, LV_ROLLER_MODE_NORMAL);
            else lv_roller_set_options(s_cc_roller[i], CC_TOL_OPTIONS, LV_ROLLER_MODE_NORMAL);
        } else {
            if (i < 3) lv_roller_set_options(s_cc_roller[i], CC_DIGIT_OPTIONS, LV_ROLLER_MODE_NORMAL);
            else if (i == 3) lv_roller_set_options(s_cc_roller[i], CC_MULT_OPTIONS, LV_ROLLER_MODE_NORMAL);
            else lv_roller_set_options(s_cc_roller[i], CC_TOL_OPTIONS, LV_ROLLER_MODE_NORMAL);
        }

        lv_obj_add_event_cb(s_cc_roller[i], cc_roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

static lv_obj_t *build_4band_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    s_cc_band_count = 4;
    build_colour_calc_ui(panel, 4);

    lv_obj_t *res_box = lv_obj_create(panel);
    lv_obj_remove_style_all(res_box);
    lv_obj_set_size(res_box, 300, 50);
    lv_obj_align(res_box, LV_ALIGN_TOP_MID, 0, 322);
    lv_obj_set_style_bg_color(res_box, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(res_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(res_box, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(res_box, 1, 0);
    lv_obj_clear_flag(res_box, LV_OBJ_FLAG_SCROLLABLE);

    s_cc_result_lbl = lv_label_create(res_box);
    lv_obj_set_style_text_color(s_cc_result_lbl, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_text_font(s_cc_result_lbl, TOOLS_BODY_FONT, 0);
    lv_obj_center(s_cc_result_lbl);

    update_colour_result();
    return panel;
}

static lv_obj_t *build_5band_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    s_cc_band_count = 5;
    build_colour_calc_ui(panel, 5);

    lv_obj_t *res_box = lv_obj_create(panel);
    lv_obj_remove_style_all(res_box);
    lv_obj_set_size(res_box, 300, 50);
    lv_obj_align(res_box, LV_ALIGN_TOP_MID, 0, 322);
    lv_obj_set_style_bg_color(res_box, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_bg_opa(res_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(res_box, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(res_box, 1, 0);
    lv_obj_clear_flag(res_box, LV_OBJ_FLAG_SCROLLABLE);

    s_cc_result_lbl = lv_label_create(res_box);
    lv_obj_set_style_text_color(s_cc_result_lbl, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_text_font(s_cc_result_lbl, TOOLS_BODY_FONT, 0);
    lv_obj_center(s_cc_result_lbl);

    update_colour_result();
    return panel;
}

static void set_active_tab(tools_tab_t tab);

static lv_obj_t *build_stub_panel(lv_obj_t *parent, const char *title)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(panel);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, TOOLS_FONT, 0);
    lv_obj_set_style_text_color(t, TOOLS_COLOR_ACT, 0);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *b = lv_label_create(panel);
    lv_label_set_text(b, "coming soon");
    lv_obj_set_style_text_font(b, TOOLS_BODY_FONT, 0);
    lv_obj_set_style_text_color(b, TOOLS_COLOR_TXT, 0);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 0, 32);

    return panel;
}

static void tab_btn_event_cb(lv_event_t *e)
{
    tools_tab_t tab = (tools_tab_t)(intptr_t)lv_event_get_user_data(e);
    if (tab == s.active_tab) {
        return;
    }
    set_active_tab(tab);
}

static void set_active_tab(tools_tab_t tab)
{
    s.active_tab = tab;

    for (int i = 0; i < TOOLS_TAB_COUNT; i++) {
        lv_color_t c = (i == tab) ? TOOLS_COLOR_ACT : TOOLS_COLOR_TXT;
        lv_obj_set_style_text_color(s.tab_label[i], c, 0);
    }

    if (s.content) {
        lv_obj_del(s.content);
        s.content = NULL;
    }

    switch (tab) {
        case TOOLS_TAB_RESISTOR:
            s.content = build_resistor_panel(s.content_area);
            break;
        case TOOLS_TAB_POWER:
            s.content = build_power_panel(s.content_area);
            break;
        case TOOLS_TAB_4BAND:
            s.content = build_4band_panel(s.content_area);
            break;
        case TOOLS_TAB_5BAND:
            s.content = build_5band_panel(s.content_area);
            break;
        default:
            break;
    }
}

static void panel_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s.panel = NULL;
}

lv_obj_t *mode_tools_create(lv_obj_t *parent)
{
    memset(&s, 0, sizeof(s));

    s.panel = lv_obj_create(parent);
    lv_obj_remove_style_all(s.panel);
    lv_obj_set_size(s.panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s.panel, TOOLS_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s.panel, TOOLS_COLOR_ACT, 0);
    lv_obj_set_style_border_width(s.panel, 1, 0);
    lv_obj_set_style_border_opa(s.panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s.panel, 6, 0);
    lv_obj_set_flex_flow(s.panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(s.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s.panel, panel_delete_cb, LV_EVENT_DELETE, NULL);

    s.subnav = lv_obj_create(s.panel);
    lv_obj_remove_style_all(s.subnav);
    lv_obj_set_size(s.subnav, lv_pct(100), 32);
    lv_obj_set_style_bg_opa(s.subnav, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(s.subnav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s.subnav, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s.subnav, 10, 0);
    lv_obj_clear_flag(s.subnav, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < TOOLS_TAB_COUNT; i++) {
        if (i > 0) {
            lv_obj_t *dot = lv_label_create(s.subnav);
            lv_label_set_text(dot, ".");
            lv_obj_set_style_text_font(dot, TOOLS_FONT, 0);
            lv_obj_set_style_text_color(dot, TOOLS_COLOR_TXT, 0);
            lv_obj_set_style_opa(dot, LV_OPA_50, 0);
        }

        lv_obj_t *btn = lv_obj_create(s.subnav);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, 32);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, tab_btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, s_tab_labels[i]);
        lv_obj_set_style_text_font(label, TOOLS_FONT, 0);
        lv_obj_set_style_text_color(label, TOOLS_COLOR_TXT, 0);
        lv_obj_center(label);

        s.tab_label[i] = label;
    }

    lv_obj_t *hairline = lv_obj_create(s.panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(100), 1);
    lv_obj_set_style_bg_color(hairline, TOOLS_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_set_style_margin_bottom(hairline, 6, 0);

    s.content_area = lv_obj_create(s.panel);
    lv_obj_remove_style_all(s.content_area);
    lv_obj_set_width(s.content_area, lv_pct(100));
    lv_obj_set_flex_grow(s.content_area, 1);
    lv_obj_set_style_bg_opa(s.content_area, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s.content_area, LV_OBJ_FLAG_SCROLLABLE);

    s.active_tab = TOOLS_TAB_COUNT; /* sentinel so the first switch always runs */
    set_active_tab(TOOLS_TAB_RESISTOR);

    return s.panel;
}