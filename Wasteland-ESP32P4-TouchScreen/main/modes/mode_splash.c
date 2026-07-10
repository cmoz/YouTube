#include "mode_splash.h"

#if LV_FONT_MONTSERRAT_48
#define SPLASH_FONT_TITLE (&lv_font_montserrat_48)
#else
#define SPLASH_FONT_TITLE LV_FONT_DEFAULT
#endif

#if LV_FONT_MONTSERRAT_20
#define SPLASH_FONT_SUB (&lv_font_montserrat_20)
#else
#define SPLASH_FONT_SUB LV_FONT_DEFAULT
#endif

#define SPLASH_COLOR_BG      lv_color_hex(0x1b4de4)  /* cobalt blue */
#define SPLASH_COLOR_YELLOW  lv_color_hex(0xffd23f)
#define SPLASH_COLOR_SUBTEXT lv_color_hex(0xf5efd8)

lv_obj_t *mode_splash_create(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, SPLASH_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "WASTELAND\nMAKER BAG");
    lv_obj_set_style_text_font(title, SPLASH_FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, SPLASH_COLOR_YELLOW, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *rule = lv_obj_create(panel);
    lv_obj_remove_style_all(rule);
    lv_obj_set_size(rule, 220, 3);
    lv_obj_set_style_bg_color(rule, SPLASH_COLOR_YELLOW, 0);
    lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
    lv_obj_align(rule, LV_ALIGN_CENTER, 0, 55);

    lv_obj_t *subtitle = lv_label_create(panel);
    lv_label_set_text(subtitle, "A CMOZMAKER BUILD");
    lv_obj_set_style_text_font(subtitle, SPLASH_FONT_SUB, 0);
    lv_obj_set_style_text_color(subtitle, SPLASH_COLOR_SUBTEXT, 0);
    lv_obj_set_style_text_letter_space(subtitle, 3, 0);
    lv_obj_set_style_opa(subtitle, LV_OPA_80, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 90);

    return panel;
}
