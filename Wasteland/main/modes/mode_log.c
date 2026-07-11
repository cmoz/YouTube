#include "mode_log.h"
#include "geiger_log.h"
#include <stdio.h>
#include <string.h>

#if LV_FONT_UNSCII_16
#define LOG_FONT (&lv_font_unscii_16)
#else
#define LOG_FONT LV_FONT_DEFAULT
#endif

#define LOG_BODY_FONT LV_FONT_DEFAULT

#define LOG_COLOR_BG        lv_color_hex(0x0a0a0a)
#define LOG_COLOR_TXT       lv_color_hex(0x9a9a9a)
#define LOG_COLOR_ACT       lv_color_hex(0xd4823a)
#define LOG_COLOR_LINE      lv_color_hex(0x2a2a2a)

typedef enum {
    LOG_TAB_BOARD = 0,
    LOG_TAB_GEIGER,
    LOG_TAB_ACTIVITY,
    LOG_TAB_COUNT
} log_tab_t;

static const char *s_tab_labels[LOG_TAB_COUNT] = { "  board", "geiger", "activity" };

static struct {
    lv_obj_t *panel;
    lv_obj_t *subnav;
    lv_obj_t *tab_btn[LOG_TAB_COUNT];
    lv_obj_t *tab_label[LOG_TAB_COUNT];
    lv_obj_t *content_area;
    lv_obj_t *content;
    log_tab_t active_tab;

    lv_obj_t *geiger_log_label;
    lv_timer_t *geiger_refresh_timer;
} s;

static void set_active_tab(log_tab_t tab);

static lv_obj_t *build_board_panel(lv_obj_t *parent)
{
    lv_obj_t *scroll = lv_obj_create(parent);
    lv_obj_remove_style_all(scroll);
    lv_obj_set_size(scroll, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_set_style_pad_all(scroll, 12, 0);

    lv_obj_t *label = lv_label_create(scroll);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_font(label, LOG_BODY_FONT, 0);
    lv_obj_set_style_text_color(label, LOG_COLOR_TXT, 0);
    lv_obj_set_style_text_line_space(label, 6, 0);

    lv_label_set_text(label,
        "board: CrowPanel Advanced 7\" ESP32-P4 HMI AI Display, rev v1.3\n\n"
        "soc: ESP32-P4, RISC-V dual-core (HP up to 400MHz + LP core)\n"
        "wireless: ESP32-C6-MINI-1 co-processor, Wi-Fi 6, BT 5.3\n"
        "memory: 16MB flash, 32MB PSRAM\n"
        "display: 1024x600 IPS, capacitive multitouch (GT911)\n"
        "audio: onboard single-mic array, NS4168 I2S path\n\n"
        "i2s audio pins (elecrow reference):\n"
        "  lrclk 21   bclk 22   sdata 23   ctrl 30\n\n"
        "project extras:\n"
        "  18x neopixel strip - wired, gpio tbd\n"
        "  hc-sr04 ultrasonic - planned, unwired\n\n"
        "source: elecrow.com, search \"CrowPanel Advanced 7 ESP32-P4\"\n\n"
        "build notes so far:\n"
        "  - fixed a purple tint caused by a byte-swap bug in the display driver\n"
        "  - all screen updates are now safely locked so nothing draws\n"
        "    mid-update from another task\n"
        "  - tapping the current tab twice no longer duplicates timers\n"
        "  - click sounds trail off properly instead of looping forever"
    );

    return scroll;
}

static lv_obj_t *build_placeholder_panel(lv_obj_t *parent, const char *title, const char *body)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(panel);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, LOG_FONT, 0);
    lv_obj_set_style_text_color(t, LOG_COLOR_ACT, 0);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *b = lv_label_create(panel);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(b, lv_pct(90));
    lv_label_set_text(b, body);
    lv_obj_set_style_text_font(b, LOG_BODY_FONT, 0);
    lv_obj_set_style_text_color(b, LOG_COLOR_TXT, 0);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 0, 32);

    return panel;
}

#define GEIGER_LOG_TEXT_CAP (24 * 64 + 128)

static void refresh_geiger_log_label(lv_timer_t *t)
{
    lv_obj_t *label = t ? lv_timer_get_user_data(t) : s.geiger_log_label;
    if (!label) {
        return;
    }

    int count = geiger_log_count();
    if (count == 0) {
        lv_label_set_text(label,
            "GEIGER COUNTER LOG\n"
            "-------------------\n\n"
            "no readings logged yet.\n\n"
            "point the sensor at a target on the geiger screen\n"
            "to begin logging.");
        return;
    }

    static char buf[GEIGER_LOG_TEXT_CAP];
    size_t used = (size_t)snprintf(buf, sizeof(buf), "GEIGER COUNTER LOG\n-------------------\n\n");

    for (int i = 0; i < count && used + 64 < sizeof(buf); i++) {
        const geiger_log_entry_t *entry = geiger_log_get(i);
        if (!entry) {
            continue;
        }
        char line[64];
        geiger_log_format_line(entry, line, sizeof(line));
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "%s\n", line);
    }

    lv_label_set_text(label, buf);
}

static lv_obj_t *build_geiger_log_panel(lv_obj_t *parent)
{
    lv_obj_t *scroll = lv_obj_create(parent);
    lv_obj_remove_style_all(scroll);
    lv_obj_set_size(scroll, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
    lv_obj_set_style_pad_all(scroll, 12, 0);

    lv_obj_t *label = lv_label_create(scroll);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_font(label, LOG_BODY_FONT, 0);
    lv_obj_set_style_text_color(label, LOG_COLOR_TXT, 0);
    lv_obj_set_style_text_line_space(label, 4, 0);

    s.geiger_log_label = label;
    refresh_geiger_log_label(NULL);

    s.geiger_refresh_timer = lv_timer_create(refresh_geiger_log_label, 1000, label);

    return scroll;
}

static void tab_btn_event_cb(lv_event_t *e)
{
    log_tab_t tab = (log_tab_t)(intptr_t)lv_event_get_user_data(e);
    if (tab == s.active_tab) {
        return;
    }
    set_active_tab(tab);
}

static void set_active_tab(log_tab_t tab)
{
    s.active_tab = tab;

    for (int i = 0; i < LOG_TAB_COUNT; i++) {
        lv_color_t c = (i == tab) ? LOG_COLOR_ACT : LOG_COLOR_TXT;
        lv_obj_set_style_text_color(s.tab_label[i], c, 0);
    }

    if (s.content) {
        lv_obj_del(s.content);
        s.content = NULL;
    }

    /* the geiger refresh timer is independent of the LVGL widget tree --
     * deleting s.content above does not touch it, so it must always be
     * torn down explicitly whenever we leave that tab */
    if (s.geiger_refresh_timer) {
        lv_timer_del(s.geiger_refresh_timer);
        s.geiger_refresh_timer = NULL;
    }
    s.geiger_log_label = NULL;

    switch (tab) {
        case LOG_TAB_BOARD:
            s.content = build_board_panel(s.content_area);
            break;
        case LOG_TAB_GEIGER:
            s.content = build_geiger_log_panel(s.content_area);
            break;
        case LOG_TAB_ACTIVITY:
            s.content = build_placeholder_panel(s.content_area, "activity log",
                "nothing logged yet.\n\n"
                "this will fill up with what's been happening on\n"
                "the device - menu taps and the like.");
            break;
        default:
            break;
    }
}

static void panel_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    /* safety net for leaving Log mode entirely while still on the geiger
     * tab -- the timer isn't a child of the widget tree, so it survives
     * s.panel's deletion unless we stop it here */
    if (s.geiger_refresh_timer) {
        lv_timer_del(s.geiger_refresh_timer);
        s.geiger_refresh_timer = NULL;
    }
    s.geiger_log_label = NULL;
    s.panel = NULL;
}

lv_obj_t *mode_log_create(lv_obj_t *parent)
{
    memset(&s, 0, sizeof(s));

    s.panel = lv_obj_create(parent);
    lv_obj_remove_style_all(s.panel);
    lv_obj_set_size(s.panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s.panel, LOG_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.panel, LV_OPA_COVER, 0);
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

    for (int i = 0; i < LOG_TAB_COUNT; i++) {
        if (i > 0) {
            lv_obj_t *dot = lv_label_create(s.subnav);
            lv_label_set_text(dot, ".");
            lv_obj_set_style_text_font(dot, LOG_FONT, 0);
            lv_obj_set_style_text_color(dot, LOG_COLOR_TXT, 0);
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
        lv_obj_set_style_text_font(label, LOG_FONT, 0);
        lv_obj_set_style_text_color(label, LOG_COLOR_TXT, 0);
        lv_obj_center(label);

        s.tab_btn[i] = btn;
        s.tab_label[i] = label;
    }

    lv_obj_t *hairline = lv_obj_create(s.panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(100), 1);
    lv_obj_set_style_bg_color(hairline, LOG_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_set_style_margin_bottom(hairline, 6, 0);

    s.content_area = lv_obj_create(s.panel);
    lv_obj_remove_style_all(s.content_area);
    lv_obj_set_width(s.content_area, lv_pct(100));
    lv_obj_set_flex_grow(s.content_area, 1);
    lv_obj_set_style_bg_opa(s.content_area, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s.content_area, LV_OBJ_FLAG_SCROLLABLE);

    s.active_tab = LOG_TAB_COUNT; /* sentinel so the first switch always runs */
    set_active_tab(LOG_TAB_BOARD);

    return s.panel;
}