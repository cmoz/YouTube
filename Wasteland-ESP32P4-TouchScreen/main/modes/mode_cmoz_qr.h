#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mode_cmoz_qr — renders a single QR code full-screen for scan-to-visit.
 *
 * idx >= 0  : tutorial_list_get(idx)'s video
 * idx == -1 : Subscribe (YouTube channel)
 * idx == -2 : Instagram
 */
lv_obj_t *mode_cmoz_qr_create(lv_obj_t *parent, int idx);

#ifdef __cplusplus
}
#endif