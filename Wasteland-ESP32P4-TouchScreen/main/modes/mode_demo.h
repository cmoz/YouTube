#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Demo reel — showcases the panel's screen + LED strip color range with
 * a rotating rainbow chase, animated swatches, and a bouncing marker.
 * Pure eye-candy, no sensor/state readout. */
lv_obj_t *mode_demo_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
