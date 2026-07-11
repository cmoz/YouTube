#include "mode_placeholder.h"

static const char *mode_name(ui_shell_mode_t mode)
{
    switch (mode) {
        case UI_SHELL_MODE_SCAN:   return "TOOLS";
        case UI_SHELL_MODE_PLAY:   return "SYNTH KEYS";
        case UI_SHELL_MODE_GEIGER: return "GEIGER COUNTER";
        case UI_SHELL_MODE_LOG:    return "JOURNAL / LOG";
        default:                   return "UNKNOWN MODE";
    }
}

lv_obj_t *mode_placeholder_create(lv_obj_t *parent, ui_shell_mode_t mode)
{
    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(content);
    lv_label_set_text(label, mode_name(mode));
    lv_obj_set_style_text_color(label, lv_color_hex(0xc8c8c8), 0);
    lv_obj_center(label);

    return content;
}