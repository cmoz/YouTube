#include "mode_dormant.h"

#if LV_FONT_UNSCII_16
#define DORMANT_FONT (&lv_font_unscii_16)
#else
#define DORMANT_FONT LV_FONT_DEFAULT
#endif

#define DORMANT_COLOR_BG    lv_color_hex(0x0a0a0a)
#define DORMANT_COLOR_LINE  lv_color_hex(0x3a2a1a)
#define DORMANT_COLOR_TEXT  lv_color_hex(0xb87445)

lv_obj_t *mode_dormant_create(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, DORMANT_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *standby = lv_label_create(panel);
    lv_label_set_text(standby, "STANDBY");
    lv_obj_set_style_text_font(standby, DORMANT_FONT, 0);
    lv_obj_set_style_text_color(standby, DORMANT_COLOR_TEXT, 0);
    lv_obj_set_style_opa(standby, LV_OPA_70, 0);
    lv_obj_align(standby, LV_ALIGN_TOP_LEFT, 16, 12);

    lv_obj_t *hairline = lv_obj_create(panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(94), 1);
    lv_obj_set_style_bg_color(hairline, DORMANT_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_align(hairline, LV_ALIGN_TOP_MID, 0, 40);

    static lv_point_precise_t hscratch_pts[] = { {0, 225}, {1008, 225} };
    lv_obj_t *hscratch = lv_line_create(panel);
    lv_line_set_points(hscratch, hscratch_pts, 2);
    lv_obj_set_style_line_width(hscratch, 1, 0);
    lv_obj_set_style_line_color(hscratch, DORMANT_COLOR_LINE, 0);
    lv_obj_set_style_line_opa(hscratch, LV_OPA_60, 0);
    lv_obj_set_style_line_rounded(hscratch, false, 0);

    static lv_point_precise_t signal_scratch_pts[] = { {665, 135}, {455, 439} };
    lv_obj_t *signal_scratch = lv_line_create(panel);
    lv_line_set_points(signal_scratch, signal_scratch_pts, 2);
    lv_obj_set_style_line_width(signal_scratch, 4, 0);
    lv_obj_set_style_line_color(signal_scratch, DORMANT_COLOR_LINE, 0);
    lv_obj_set_style_line_opa(signal_scratch, LV_OPA_70, 0);
    lv_obj_set_style_line_rounded(signal_scratch, false, 0);

static lv_point_precise_t left_scratch_pts[] = { {50, 150}, {130, 290} };
    lv_obj_t *left_scratch = lv_line_create(panel);
    lv_line_set_points(left_scratch, left_scratch_pts, 2);
    lv_obj_set_style_line_width(left_scratch, 2, 0);
    lv_obj_set_style_line_color(left_scratch, DORMANT_COLOR_LINE, 0);
    lv_obj_set_style_line_opa(left_scratch, LV_OPA_60, 0);
    lv_obj_set_style_line_rounded(left_scratch, false, 0);

    lv_obj_t *circle = lv_obj_create(panel);
    lv_obj_remove_style_all(circle);
    lv_obj_set_size(circle, 24, 24);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(circle, DORMANT_COLOR_TEXT, 0);
    lv_obj_set_style_border_width(circle, 1, 0);
    lv_obj_set_style_border_opa(circle, LV_OPA_70, 0);
    lv_obj_align(circle, LV_ALIGN_CENTER, -20, -6);

    lv_obj_t *no_signal = lv_label_create(panel);
    lv_label_set_text(no_signal, "NO SIGNAL");
    lv_obj_set_style_text_font(no_signal, DORMANT_FONT, 0);
    lv_obj_set_style_text_color(no_signal, DORMANT_COLOR_TEXT, 0);
    lv_obj_set_style_opa(no_signal, LV_OPA_80, 0);
    lv_obj_align(no_signal, LV_ALIGN_CENTER, 0, 30);

    return panel;
}