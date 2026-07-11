#pragma once

#include "lvgl.h"
#include "ui/ui_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Builds a simple centered label showing the mode name, parented under
 * the given content area. Stand-in until each mode gets its real UI. */
lv_obj_t *mode_placeholder_create(lv_obj_t *parent, ui_shell_mode_t mode);

#ifdef __cplusplus
}
#endif