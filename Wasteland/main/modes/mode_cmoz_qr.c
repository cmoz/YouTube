#include "mode_cmoz_qr.h"
#include "data/tutorial_list.h"
#include "lib/qrcode.h"
#include <stdio.h>
#include <string.h>
#include "modes/mode_cmoz.h"

#if LV_FONT_UNSCII_16
#define QR_FONT (&lv_font_unscii_16)
#else
#define QR_FONT LV_FONT_DEFAULT
#endif

#define QR_BODY_FONT LV_FONT_DEFAULT
#define QR_COLOR_BG  lv_color_hex(0x0a0a0a)
#define QR_COLOR_TXT lv_color_hex(0x9a9a9a)
#define QR_COLOR_ACT lv_color_hex(0xd4823a)

#define QR_VERSION 4
#define QR_SCALE   5
#define QR_QUIET   8

static void draw_qr_on_card(lv_obj_t *card, const char *url)
{
    QRCode qrcode;
    uint8_t qrdata[qrcode_getBufferSize(QR_VERSION)];

    int err = qrcode_initText(&qrcode, qrdata, QR_VERSION, ECC_LOW, url);
    if (err < 0) {
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_text(lbl, url);
        lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
        lv_obj_set_style_text_font(lbl, QR_BODY_FONT, 0);
        lv_obj_set_width(lbl, 180);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_center(lbl);
        return;
    }

    int card_px = qrcode.size * QR_SCALE + QR_QUIET * 2;
    lv_obj_set_size(card, card_px, card_px);

    for (int y = 0; y < qrcode.size; y++) {
        for (int x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                lv_obj_t *dot = lv_obj_create(card);
                lv_obj_remove_style_all(dot);
                lv_obj_set_size(dot, QR_SCALE, QR_SCALE);
                lv_obj_set_pos(dot, QR_QUIET + x * QR_SCALE, QR_QUIET + y * QR_SCALE);
                lv_obj_set_style_bg_color(dot, lv_color_black(), 0);
                lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
            }
        }
    }
}

static void back_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *content_area = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *list_screen = mode_cmoz_create(content_area);
    ui_shell_set_content(list_screen);
}

lv_obj_t *mode_cmoz_qr_create(lv_obj_t *parent, int idx)
{
    char url[128];
    char title[80];

    if (idx == -1) {
        snprintf(url, sizeof(url), "https://www.youtube.com/@CMozMaker?sub_confirmation=1");
        snprintf(title, sizeof(title), "Subscribe on YouTube");
    } else if (idx == -2) {
        snprintf(url, sizeof(url), "https://instagram.com/cmoz");
        snprintf(title, sizeof(title), "Follow on Instagram");
    } else {
        const tutorial_entry_t *t = tutorial_list_get(idx);
        if (!t) {
            snprintf(url, sizeof(url), "https://www.tinkertailor.ca");
            snprintf(title, sizeof(title), "TinkerTailor.ca");
        } else {
            snprintf(url, sizeof(url), "https://youtu.be/%s", t->video_id);
            snprintf(title, sizeof(title), "%s", t->title);
        }
    }

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, QR_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_obj_create(panel);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 44, 40);
    lv_obj_set_pos(back_btn, 4, 4);
    lv_obj_set_style_bg_color(back_btn, QR_COLOR_ACT, 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, (void *)parent);

    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0x0a0a0a), 0);
    lv_obj_center(back_lbl);

    lv_obj_t *title_lbl = lv_label_create(panel);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_font(title_lbl, QR_BODY_FONT, 0);
    lv_obj_set_style_text_color(title_lbl, QR_COLOR_ACT, 0);
    lv_obj_set_width(title_lbl, lv_pct(80));
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *card = lv_obj_create(panel);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 10);

    draw_qr_on_card(card, url);

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "Point your phone camera at the QR code");
    lv_obj_set_style_text_color(hint, QR_COLOR_TXT, 0);
    lv_obj_set_style_text_font(hint, QR_BODY_FONT, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    return panel;
}