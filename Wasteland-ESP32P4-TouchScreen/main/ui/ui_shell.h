#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ui_shell — persistent Salvage Terminal chrome.
 *
 * Owns the dark bezel, the swappable central content area, and the
 * bottom nav row (SCAN / PLAY / GEIGER / LOG). Individual modes create
 * their own lv_obj content and hand it to ui_shell_set_content(); the
 * shell takes care of showing/hiding/deleting whatever was there before.
 *
 * All calls into this module must happen while the LVGL port mutex is
 * held (bsp_display_lock() / lvgl_port_lock() depending on your BSP),
 * same as any other direct LVGL call.
 */

typedef enum {
    UI_SHELL_MODE_SCAN = 0,
    UI_SHELL_MODE_PLAY,
    UI_SHELL_MODE_GEIGER,
    UI_SHELL_MODE_LOG,
    UI_SHELL_MODE_CMOZ,
    UI_SHELL_MODE_DEMO,
    UI_SHELL_MODE_COUNT
} ui_shell_mode_t;

/* Called when the user taps a nav button. new_content is NOT created for
 * you — the mode owner is expected to build its lv_obj tree and call
 * ui_shell_set_content() from within (or shortly after) this callback. */
typedef void (*ui_shell_mode_cb_t)(ui_shell_mode_t mode, void *user_data);

/* Called each time the glitch timer fires a visible glitch. Wire this to
 * pulse the 18x NeoPixel strip once that pin is decided. Runs on the
 * LVGL task, so keep it short (queue/notify, don't block). */
typedef void (*ui_shell_glitch_cb_t)(void *user_data);

typedef void (*ui_shell_mute_cb_t)(bool muted, void *user_data);

/* Builds the bezel, content container, and nav bar on the active screen.
 * Call once, after your display/LVGL port bring-up. Starts hidden/dead
 * (see ui_shell_play_reveal()) unless ui_shell_skip_reveal() is called. */
void ui_shell_init(void);

/* The container modes should parent their widgets under. */
lv_obj_t *ui_shell_get_content_area(void);

/* Swaps the central content. Deletes whatever object was previously set
 * via this function (pass NULL to just clear it). */
void ui_shell_set_content(lv_obj_t *content);

/* Registers the nav-tap callback. */
void ui_shell_set_mode_cb(ui_shell_mode_cb_t cb, void *user_data);

/* Highlights the given nav button as active (doesn't fire the callback). */
void ui_shell_set_active_mode(ui_shell_mode_t mode);

/* Registers the glitch-sync callback (e.g. NeoPixel pulse). */
void ui_shell_set_glitch_cb(ui_shell_glitch_cb_t cb, void *user_data);

/* Fires a single glitch immediately, outside the normal random timer.
 * Useful for scripted moments (e.g. during the reveal sequence). */
void ui_shell_glitch_trigger(void);

/* Plays the "boots looking dead, then wakes" reveal animation on the
 * bezel/nav (screen starts dark/inert, then chrome fades/glitches in).
 * Call once after ui_shell_init(). The actual wake *trigger* — i.e.
 * detecting the specific touch sequence — is deliberately left to the
 * caller (main.c), since it will likely hook raw touch events rather
 * than LVGL widget events. Call this once you've decided the device
 * should wake. */
void ui_shell_play_reveal(void);

/* Skips straight to the awake state, chrome fully visible. Handy while
 * iterating on mode screens so you don't replay the reveal every flash. */
void ui_shell_skip_reveal(void);

/* Registers a callback fired when the corner mute toggle is tapped.
 * The shell tracks its own visual muted/unmuted state; this callback
 * is where the caller (main.c) actually silences audio globally. */
void ui_shell_set_mute_cb(ui_shell_mute_cb_t cb, void *user_data);

/* Current muted state as shown by the toggle. */
bool ui_shell_is_muted(void);

#ifdef __cplusplus
}
#endif