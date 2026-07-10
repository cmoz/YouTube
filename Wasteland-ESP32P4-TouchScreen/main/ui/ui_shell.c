#include "ui_shell.h"

#include "esp_random.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ui_shell";

/* ---- layout constants (tuned for the 1024x600 CrowPanel Advanced) ---- */
#define UI_SHELL_SCREEN_W       1024
#define UI_SHELL_SCREEN_H       600
#define UI_SHELL_BEZEL_MARGIN   8
#define UI_SHELL_NAV_HEIGHT     64
#define UI_SHELL_NAV_GAP        6
#define UI_SHELL_NAV_BTN_COUNT  UI_SHELL_MODE_COUNT

/* mono font — enable LV_FONT_UNSCII_16 in menuconfig for the real mono
 * look; falls back to the default LVGL font if it isn't enabled so this
 * file still compiles either way. */
#if LV_FONT_UNSCII_16
#define UI_SHELL_FONT (&lv_font_unscii_16)
#else
#define UI_SHELL_FONT LV_FONT_DEFAULT
#endif

#define UI_SHELL_COLOR_BG        lv_color_hex(0x0a0a0a)
#define UI_SHELL_COLOR_BEZEL     lv_color_hex(0x2a2a2a)
#define UI_SHELL_COLOR_NAV_BG    lv_color_hex(0x141414)
#define UI_SHELL_COLOR_NAV_TXT   lv_color_hex(0x9a9a9a)
#define UI_SHELL_COLOR_NAV_ACT   lv_color_hex(0xe0e0e0)
#define UI_SHELL_COLOR_GLITCH    lv_color_hex(0xd8d8d8)

#define UI_SHELL_COLOR_BOLT      lv_color_hex(0x4a4a4a)

static const char *s_nav_labels[UI_SHELL_MODE_COUNT] = {
    "TOOLS", "PLAY", "GEIGER", "LOG", "CMOZMAKER", "DEMO"
};

static struct {
    lv_obj_t *screen;
    lv_obj_t *bezel;
    lv_obj_t *content_area;
    lv_obj_t *content;          /* currently-mounted mode content, owned by caller data */
    lv_obj_t *nav_bar;
    lv_obj_t *nav_btn[UI_SHELL_MODE_COUNT];
    lv_obj_t *nav_label[UI_SHELL_MODE_COUNT];
    lv_obj_t *glitch_overlay;

    lv_obj_t *mute_toggle;
    lv_obj_t *mute_label;
    bool muted;
    ui_shell_mute_cb_t mute_cb;
    void *mute_cb_user_data;

    ui_shell_mode_t active_mode;
    ui_shell_mode_cb_t mode_cb;
    void *mode_cb_user_data;

    ui_shell_glitch_cb_t glitch_cb;
    void *glitch_cb_user_data;

    lv_timer_t *glitch_timer;
    lv_timer_t *glitch_hide_timer;
} s;

/* ------------------------------------------------------------------ */
/* glitch effect                                                       */
/* ------------------------------------------------------------------ */

static void glitch_hide_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_obj_add_flag(s.glitch_overlay, LV_OBJ_FLAG_HIDDEN);
    /* snap chrome back to its resting offset */
    lv_obj_set_pos(s.bezel, 0, 0);
    s.glitch_hide_timer = NULL;
}

void ui_shell_glitch_trigger(void)
{
    if (!s.glitch_overlay) {
        return;
    }

    /* brief static flash */
    lv_obj_set_style_bg_opa(s.glitch_overlay, LV_OPA_10, 0);
    lv_obj_clear_flag(s.glitch_overlay, LV_OBJ_FLAG_HIDDEN);

    /* small jitter on the whole bezel so it reads as a signal hiccup,
     * not just a flash */
int32_t jx = (int32_t)(esp_random() % 3) - 1;
int32_t jy = (int32_t)(esp_random() % 3) - 1;
    lv_obj_set_pos(s.bezel, jx, jy);

    if (s.glitch_hide_timer) {
        lv_timer_del(s.glitch_hide_timer);
    }
    uint32_t hide_delay_ms = 30 + (esp_random() % 60);
    s.glitch_hide_timer = lv_timer_create(glitch_hide_cb, hide_delay_ms, NULL);
    lv_timer_set_repeat_count(s.glitch_hide_timer, 1);

    if (s.glitch_cb) {
        s.glitch_cb(s.glitch_cb_user_data);
    }
}

static void glitch_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    /* re-roll the next interval each time so it never falls into an
     * obvious rhythm */
    uint32_t next_ms = 6000 + (esp_random() % 9000);
    lv_timer_set_period(s.glitch_timer, next_ms);
    ui_shell_glitch_trigger();
}

/* ------------------------------------------------------------------ */
/* nav bar                                                             */
/* ------------------------------------------------------------------ */

static void nav_btn_event_cb(lv_event_t *e)
{
    ui_shell_mode_t mode = (ui_shell_mode_t)(intptr_t)lv_event_get_user_data(e);
    if (mode == s.active_mode) {
        return; /* already showing this mode — don't rebuild it */
    }
    ui_shell_set_active_mode(mode);
    if (s.mode_cb) {
        s.mode_cb(mode, s.mode_cb_user_data);
    }
}

void ui_shell_set_active_mode(ui_shell_mode_t mode)
{
    if (mode >= UI_SHELL_MODE_COUNT) {
        return;
    }
    s.active_mode = mode;
    for (int i = 0; i < UI_SHELL_MODE_COUNT; i++) {
        lv_color_t c = (i == mode) ? UI_SHELL_COLOR_NAV_ACT : UI_SHELL_COLOR_NAV_TXT;
        lv_obj_set_style_text_color(s.nav_label[i], c, 0);
        lv_obj_set_style_border_width(s.nav_btn[i], (i == mode) ? 1 : 0, 0);
        lv_obj_set_style_border_color(s.nav_btn[i], UI_SHELL_COLOR_NAV_ACT, 0);
        lv_obj_set_style_border_side(s.nav_btn[i], LV_BORDER_SIDE_TOP, 0);
    }
}

void ui_shell_set_mode_cb(ui_shell_mode_cb_t cb, void *user_data)
{
    s.mode_cb = cb;
    s.mode_cb_user_data = user_data;
}

void ui_shell_set_glitch_cb(ui_shell_glitch_cb_t cb, void *user_data)
{
    s.glitch_cb = cb;
    s.glitch_cb_user_data = user_data;
}

/* ------------------------------------------------------------------ */
/* content area                                                        */
/* ------------------------------------------------------------------ */

lv_obj_t *ui_shell_get_content_area(void)
{
    return s.content_area;
}

void ui_shell_set_content(lv_obj_t *content)
{
    if (s.content) {
        lv_obj_del(s.content);
        s.content = NULL;
    }
    s.content = content;
    if (s.content) {
        lv_obj_set_parent(s.content, s.content_area);
    }
}

/* ------------------------------------------------------------------ */
/* reveal sequence                                                     */
/* ------------------------------------------------------------------ */

void ui_shell_skip_reveal(void)
{
    lv_obj_clear_flag(s.bezel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.nav_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s.bezel, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(s.nav_bar, LV_OPA_COVER, 0);

/*     if (!s.glitch_timer) {
        uint32_t first_ms = 6000 + (esp_random() % 9000);
        s.glitch_timer = lv_timer_create(glitch_timer_cb, first_ms, NULL);
    } */
}

static void reveal_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void reveal_anim_ready_cb(lv_anim_t *a)
{
    LV_UNUSED(a);
    /* a couple of hard glitch flashes as it "locks on" before settling */
    ui_shell_glitch_trigger();
    ui_shell_glitch_trigger();

/*         if (!s.glitch_timer) {
            uint32_t first_ms = 6000 + (esp_random() % 9000);
            s.glitch_timer = lv_timer_create(glitch_timer_cb, first_ms, NULL);
        } */
}

void ui_shell_play_reveal(void)
{
    lv_obj_clear_flag(s.bezel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.nav_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(s.bezel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(s.nav_bar, LV_OPA_TRANSP, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s.bezel);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 900);
    lv_anim_set_exec_cb(&a, reveal_anim_cb);
    lv_anim_set_ready_cb(&a, reveal_anim_ready_cb);
    lv_anim_start(&a);

    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, s.nav_bar);
    lv_anim_set_values(&a2, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a2, 900);
    lv_anim_set_exec_cb(&a2, reveal_anim_cb);
    lv_anim_start(&a2);
}

/* ------------------------------------------------------------------ */
/* init                                                                 */
/* ------------------------------------------------------------------ */

static void build_bezel(void)
{
    s.bezel = lv_obj_create(s.screen);
    lv_obj_remove_style_all(s.bezel);
    lv_obj_set_size(s.bezel, UI_SHELL_SCREEN_W, UI_SHELL_SCREEN_H);
    lv_obj_set_pos(s.bezel, 0, 0);
    lv_obj_set_style_bg_color(s.bezel, UI_SHELL_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.bezel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s.bezel, UI_SHELL_COLOR_BEZEL, 0);
    lv_obj_set_style_border_width(s.bezel, UI_SHELL_BEZEL_MARGIN, 0);
    lv_obj_set_style_border_opa(s.bezel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s.bezel, LV_OBJ_FLAG_SCROLLABLE);
}

static void build_corner_bolts(void)
{
    const int32_t bolt_size = 5;
    const int32_t inset = UI_SHELL_BEZEL_MARGIN / 2;

    lv_align_t corners[4] = {
        LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT,
        LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *bolt = lv_obj_create(s.screen);
        lv_obj_remove_style_all(bolt);
        lv_obj_set_size(bolt, bolt_size, bolt_size);
        lv_obj_set_style_radius(bolt, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(bolt, UI_SHELL_COLOR_BOLT, 0);
        lv_obj_set_style_bg_opa(bolt, LV_OPA_COVER, 0);
        lv_obj_clear_flag(bolt, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(bolt, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(bolt, corners[i], 
                     (corners[i] == LV_ALIGN_TOP_LEFT || corners[i] == LV_ALIGN_BOTTOM_LEFT) ? inset : -inset,
                     (corners[i] == LV_ALIGN_TOP_LEFT || corners[i] == LV_ALIGN_TOP_RIGHT) ? inset : -inset);
    }
}

static void build_content_area(void)
{
    int32_t content_h = UI_SHELL_SCREEN_H
                         - (2 * UI_SHELL_BEZEL_MARGIN)
                         - UI_SHELL_NAV_HEIGHT
                         - UI_SHELL_NAV_GAP;

    s.content_area = lv_obj_create(s.screen);
    lv_obj_remove_style_all(s.content_area);
    lv_obj_set_size(s.content_area,
                     UI_SHELL_SCREEN_W - (2 * UI_SHELL_BEZEL_MARGIN),
                     content_h);
    lv_obj_align(s.content_area, LV_ALIGN_TOP_MID, 0, UI_SHELL_BEZEL_MARGIN);
    lv_obj_set_style_bg_color(s.content_area, UI_SHELL_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.content_area, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s.content_area, LV_OBJ_FLAG_SCROLLABLE);
}

static void build_nav_bar(void)
{
    s.nav_bar = lv_obj_create(s.screen);
    lv_obj_remove_style_all(s.nav_bar);
    lv_obj_set_size(s.nav_bar,
                     UI_SHELL_SCREEN_W - (2 * UI_SHELL_BEZEL_MARGIN),
                     UI_SHELL_NAV_HEIGHT);
    lv_obj_align(s.nav_bar, LV_ALIGN_BOTTOM_MID, 0, -UI_SHELL_BEZEL_MARGIN);
    lv_obj_set_style_bg_color(s.nav_bar, UI_SHELL_COLOR_NAV_BG, 0);
    lv_obj_set_style_bg_opa(s.nav_bar, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(s.nav_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s.nav_bar,
                           LV_FLEX_ALIGN_END,     /* pack cluster to the right */
                           LV_FLEX_ALIGN_CENTER,
                           LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_right(s.nav_bar, 16, 0);
    lv_obj_set_style_pad_column(s.nav_bar, 6, 0);
    lv_obj_clear_flag(s.nav_bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < UI_SHELL_MODE_COUNT; i++) {
        if (i > 0) {
            lv_obj_t *dot = lv_label_create(s.nav_bar);
            lv_label_set_text(dot, ".");
            lv_obj_set_style_text_font(dot, UI_SHELL_FONT, 0);
            lv_obj_set_style_text_color(dot, UI_SHELL_COLOR_NAV_TXT, 0);
            lv_obj_set_style_opa(dot, LV_OPA_50, 0);
        }

        lv_obj_t *btn = lv_obj_create(s.nav_bar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, UI_SHELL_NAV_HEIGHT);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED,
                             (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, s_nav_labels[i]);
        lv_obj_set_style_text_font(label, UI_SHELL_FONT, 0);
        lv_obj_set_style_text_color(label, UI_SHELL_COLOR_NAV_TXT, 0);
        lv_obj_center(label);

        s.nav_btn[i] = btn;
        s.nav_label[i] = label;
    }
}

static void update_mute_visual(void);
static void mute_toggle_event_cb(lv_event_t *e);

static void build_mute_toggle(void)
{
    s.mute_toggle = lv_obj_create(s.screen);
    lv_obj_remove_style_all(s.mute_toggle);
    lv_obj_set_size(s.mute_toggle, 48, 40);
    lv_obj_align(s.mute_toggle, LV_ALIGN_TOP_RIGHT,
                 -(UI_SHELL_BEZEL_MARGIN + 6), UI_SHELL_BEZEL_MARGIN + 6);
    lv_obj_set_style_bg_color(s.mute_toggle, UI_SHELL_COLOR_NAV_BG, 0);
    lv_obj_set_style_bg_opa(s.mute_toggle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s.mute_toggle, 4, 0);
    lv_obj_add_flag(s.mute_toggle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s.mute_toggle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s.mute_toggle, mute_toggle_event_cb, LV_EVENT_CLICKED, NULL);

    s.mute_label = lv_label_create(s.mute_toggle);
    lv_obj_set_style_text_font(s.mute_label, LV_FONT_DEFAULT, 0);
    lv_obj_center(s.mute_label);

    s.muted = false;
    update_mute_visual();
}

static void build_glitch_overlay(void)
{
    s.glitch_overlay = lv_obj_create(s.screen);
    lv_obj_remove_style_all(s.glitch_overlay);
    lv_obj_set_size(s.glitch_overlay, UI_SHELL_SCREEN_W, UI_SHELL_SCREEN_H);
    lv_obj_set_pos(s.glitch_overlay, 0, 0);
    lv_obj_set_style_bg_color(s.glitch_overlay, UI_SHELL_COLOR_GLITCH, 0);
    lv_obj_set_style_bg_opa(s.glitch_overlay, LV_OPA_30, 0);
    lv_obj_clear_flag(s.glitch_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.glitch_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s.glitch_overlay, LV_OBJ_FLAG_HIDDEN);
    /* stays on top of bezel + content, below nothing */
    lv_obj_move_foreground(s.glitch_overlay);
}

/* ------------------------------------------------------------------ */
/* mute toggle                                                         */
/* ------------------------------------------------------------------ */

static void update_mute_visual(void)
{
    if (s.muted) {
        lv_label_set_text(s.mute_label, LV_SYMBOL_MUTE);
        lv_obj_set_style_text_color(s.mute_label, lv_color_hex(0xe0a0a0), 0);
        lv_obj_set_style_border_color(s.mute_toggle, lv_color_hex(0xe0a0a0), 0);
        lv_obj_set_style_border_width(s.mute_toggle, 1, 0);
    } else {
        lv_label_set_text(s.mute_label, LV_SYMBOL_VOLUME_MAX);
        lv_obj_set_style_text_color(s.mute_label, UI_SHELL_COLOR_NAV_TXT, 0);
        lv_obj_set_style_border_width(s.mute_toggle, 0, 0);
    }
}

static void mute_toggle_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s.muted = !s.muted;
    update_mute_visual();
    if (s.mute_cb) {
        s.mute_cb(s.muted, s.mute_cb_user_data);
    }
}

void ui_shell_set_mute_cb(ui_shell_mute_cb_t cb, void *user_data)
{
    s.mute_cb = cb;
    s.mute_cb_user_data = user_data;
}

bool ui_shell_is_muted(void)
{
    return s.muted;
}

void ui_shell_init(void)
{
    memset(&s, 0, sizeof(s));

    s.screen = lv_scr_act();
    lv_obj_set_style_bg_color(s.screen, UI_SHELL_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s.screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s.screen, LV_OBJ_FLAG_SCROLLABLE);

build_bezel();
build_corner_bolts();
    build_content_area();
    build_nav_bar();
    build_glitch_overlay();
    build_mute_toggle();

    /* starts dark/inert — caller decides when to run the reveal */
    lv_obj_add_flag(s.bezel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.nav_bar, LV_OBJ_FLAG_HIDDEN);

    s.active_mode = UI_SHELL_MODE_COUNT;  /* sentinel: nothing active yet, so the first tap on any tab (including SCAN) still fires */

    ESP_LOGI(TAG, "shell built: %dx%d, content %dx%d",
             UI_SHELL_SCREEN_W, UI_SHELL_SCREEN_H,
             (int)lv_obj_get_width(s.content_area),
             (int)lv_obj_get_height(s.content_area));
}