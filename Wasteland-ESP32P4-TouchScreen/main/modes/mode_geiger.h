#pragma once

#include "lvgl.h"
#include "ui/ui_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mode_geiger — simulated Geiger counter readout.
 *
 * Drives a click-rate (CPM) display with a pulsing indicator, plus a
 * background rate that drifts over time so it feels alive rather than
 * flat-random. The click TIMING source is swappable: by default it's
 * a randomized exponential simulation, but mode_geiger_set_click_source()
 * lets you later swap in a real source (e.g. onboard mic RMS energy)
 * without touching the display/animation code at all.
 */

/* Returns the delay in ms until the next simulated "click". A real
 * sensor-driven source would return shorter delays when it detects
 * more activity (e.g. louder ambient sound), longer delays when quiet. */
typedef uint32_t (*mode_geiger_click_source_t)(void);

/* Returns measured distance in centimeters. Return a negative value when
 * unavailable/invalid so the UI can show "--". */
typedef float (*mode_geiger_distance_source_t)(void);

/* Builds the Geiger mode content, parented under `parent`. Starts its
 * own internal lv_timers to drive clicks and rate drift; these are
 * automatically stopped when the returned object is deleted (e.g. via
 * ui_shell_set_content() swapping to a different mode), so the caller
 * doesn't need to do any manual cleanup. */
lv_obj_t *mode_geiger_create(lv_obj_t *parent);

/* Overrides the click-timing source. Pass NULL to restore the default
 * randomized simulation. Safe to call before or while the mode is
 * active — takes effect starting with the next scheduled click. */
void mode_geiger_set_click_source(mode_geiger_click_source_t source);

/* Optional distance feed (e.g. HC-SR04). When set, DIST readout is updated
 * and CPM target tracks distance (closer => higher CPM). Pass NULL to
 * restore pure simulated drift behavior. */
void mode_geiger_set_distance_source(mode_geiger_distance_source_t source);

#ifdef __cplusplus
}
#endif