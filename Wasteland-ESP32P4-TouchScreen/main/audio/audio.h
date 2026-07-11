#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Globally mutes/unmutes all audio. When muted, audio_play_click()
 * becomes a no-op — nothing gets queued or rendered. */
void audio_set_muted(bool muted);
bool audio_is_muted(void);

/*
 * audio — owns the I2S output channel and amp enable pin, and runs a
 * dedicated task that renders short click/beep bursts on request.
 *
 * Call audio_init() once, after display bring-up. After that,
 * audio_play_click() is safe to call from any task (including the
 * LVGL task) — it just posts a request and returns immediately,
 * so it never blocks on I2S DMA timing.
 */

bool audio_init(void);

/* Requests a short click/beep burst. Non-blocking — safe to call from
 * lv_timer callbacks or anywhere else that must not stall. */
void audio_play_click(void);

/* Requests a click with proximity-driven emphasis. intensity is clamped
 * to 0..1, where 0 is the lowest/softest click and 1 is the sharpest,
 * highest click. */
void audio_play_click_tuned(float intensity);

/* Immediately stops any in-progress/queued click sound. Call this when
 * leaving a mode that was using audio, so nothing lingers after the
 * mode's own cleanup runs. */
void audio_stop(void);

#ifdef __cplusplus
}
#endif