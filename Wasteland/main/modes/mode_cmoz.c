#include "mode_cmoz.h"
#include "data/tutorial_list.h"
#include <stdio.h>
#include <string.h>
#include "modes/mode_cmoz_qr.h"


#if LV_FONT_UNSCII_16
#define CMOZ_FONT (&lv_font_unscii_16)
#else
#define CMOZ_FONT LV_FONT_DEFAULT
#endif

#define CMOZ_BODY_FONT LV_FONT_DEFAULT

#define CMOZ_COLOR_BG    lv_color_hex(0x0a0a0a)
#define CMOZ_COLOR_TXT   lv_color_hex(0x9a9a9a)
#define CMOZ_COLOR_ACT   lv_color_hex(0xd4823a)
#define CMOZ_COLOR_LINE  lv_color_hex(0x2a2a2a)

static void tutorial_item_event_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *content_area = ui_shell_get_content_area();
    lv_obj_t *qr_screen = mode_cmoz_qr_create(content_area, idx);
    ui_shell_set_content(qr_screen);
}

lv_obj_t *mode_cmoz_create(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, CMOZ_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_set_style_pad_all(panel, 12, 0);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "CMOZMAKER");
    lv_obj_set_style_text_font(title, CMOZ_FONT, 0);
    lv_obj_set_style_text_color(title, CMOZ_COLOR_ACT, 0);
    lv_obj_set_pos(title, 0, 0);

lv_obj_t *bio = lv_label_create(panel);
    lv_label_set_long_mode(bio, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(bio, lv_pct(96));
    lv_label_set_text(bio,
        "CmozMaker explores wearable and embedded electronics through "
        "practical ESP32-based builds that bridge textiles, sensors, and "
        "real-world interaction. Tutorials, tips, and project ideas for "
        "wearable tech, fashion tech, and prototyping. Components and "
        "conductive fabrics from my little wearables shop, TinkerTailor.ca "
        "-- support small business!");
    lv_obj_set_style_text_font(bio, CMOZ_BODY_FONT, 0);
    lv_obj_set_style_text_color(bio, CMOZ_COLOR_TXT, 0);
    lv_obj_set_pos(bio, 0, 32);

    lv_obj_t *hairline = lv_obj_create(panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(96), 1);
    lv_obj_set_style_bg_color(hairline, CMOZ_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_set_pos(hairline, 0, 88);

    lv_obj_t *sub_row = lv_obj_create(panel);
    lv_obj_remove_style_all(sub_row);
    lv_obj_set_size(sub_row, lv_pct(96), 34);
    lv_obj_set_pos(sub_row, 0, 96);
    lv_obj_set_style_bg_color(sub_row, lv_color_hex(0x1c140a), 0);
    lv_obj_set_style_bg_opa(sub_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sub_row, CMOZ_COLOR_ACT, 0);
    lv_obj_set_style_border_width(sub_row, 1, 0);
    lv_obj_add_flag(sub_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(sub_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(sub_row, tutorial_item_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);

    lv_obj_t *sub_lbl = lv_label_create(sub_row);
    lv_label_set_text(sub_lbl, "SUBSCRIBE ON YOUTUBE");
    lv_obj_set_style_text_font(sub_lbl, CMOZ_BODY_FONT, 0);
    lv_obj_set_style_text_color(sub_lbl, CMOZ_COLOR_ACT, 0);
    lv_obj_align(sub_lbl, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *ig_row = lv_obj_create(panel);
    lv_obj_remove_style_all(ig_row);
    lv_obj_set_size(ig_row, lv_pct(96), 34);
    lv_obj_set_pos(ig_row, 0, 136);
    lv_obj_set_style_bg_color(ig_row, lv_color_hex(0x1c140a), 0);
    lv_obj_set_style_bg_opa(ig_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ig_row, CMOZ_COLOR_ACT, 0);
    lv_obj_set_style_border_width(ig_row, 1, 0);
    lv_obj_add_flag(ig_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ig_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ig_row, tutorial_item_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-2);

    lv_obj_t *ig_lbl = lv_label_create(ig_row);
    lv_label_set_text(ig_lbl, "INSTAGRAM @CMoz");
    lv_obj_set_style_text_font(ig_lbl, CMOZ_BODY_FONT, 0);
    lv_obj_set_style_text_color(ig_lbl, CMOZ_COLOR_ACT, 0);
    lv_obj_align(ig_lbl, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *list_title = lv_label_create(panel);
    lv_label_set_text(list_title, "TUTORIALS");
    lv_obj_set_style_text_font(list_title, CMOZ_FONT, 0);
    lv_obj_set_style_text_color(list_title, CMOZ_COLOR_ACT, 0);
    lv_obj_set_pos(list_title, 0, 184);

    int count = tutorial_list_count();
    int y = 216;


    for (int i = 0; i < count; i++) {
        const tutorial_entry_t *t = tutorial_list_get(i);
        if (!t) continue;

        lv_obj_t *item = lv_obj_create(panel);
        lv_obj_remove_style_all(item);
        lv_obj_set_size(item, lv_pct(96), 34);
        lv_obj_set_pos(item, 0, y);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x1c1c1c), 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(item, tutorial_item_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(item);
        lv_label_set_text(lbl, t->title);
        lv_obj_set_style_text_font(lbl, CMOZ_BODY_FONT, 0);
        lv_obj_set_style_text_color(lbl, CMOZ_COLOR_TXT, 0);
        lv_obj_set_width(lbl, lv_pct(90));
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);

        y += 40;
    }

    return panel;
}