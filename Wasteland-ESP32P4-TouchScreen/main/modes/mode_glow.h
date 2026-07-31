#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CMozGlow demo — tap an effect, watch the strip react. A live preview
 * of github.com/cmoz/CMozGlow's effects, ported to this board's own
 * WS2812B strip through the glow_engine (see main/leds/glow_engine.c). */
lv_obj_t *mode_glow_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
