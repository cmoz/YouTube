#pragma once

#include "lvgl.h"
#include "ui/ui_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mode_cmoz — CMozMaker tutorials + channel bio.
 *
 * Scrollable list of tutorial videos (tap one to see its QR code screen)
 * plus a short channel bio section.
 */

lv_obj_t *mode_cmoz_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif