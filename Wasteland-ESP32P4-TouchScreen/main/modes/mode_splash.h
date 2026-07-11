#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Boot splash — bright cobalt/yellow title card shown for a few seconds
 * before the shell settles into the dormant "NO SIGNAL" screen. Pure
 * LVGL shapes/text, no image assets. */
lv_obj_t *mode_splash_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
