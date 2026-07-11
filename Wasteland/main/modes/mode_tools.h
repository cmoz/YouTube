#pragma once

#include "lvgl.h"
#include "ui/ui_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mode_tools — maker calculators (LED resistor, power, 4/5-band colour
 * code), tabbed the same way mode_log's board/geiger/activity tabs work.
 */

lv_obj_t *mode_tools_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif