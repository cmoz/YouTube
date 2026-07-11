#pragma once

#include "lvgl.h"
#include "ui/ui_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * mode_play — "component roulette" project-idea generator.
 *
 * Left column shows 5 randomly-picked components as tappable cards —
 * tapping one swaps it for another component in the same category.
 * Right column shows a set of randomized creative prompts (scale,
 * interaction, theme, social impact) that can be refreshed independently
 * of the components, mirroring the Maker-O-Matic 3000 desktop tool.
 */

lv_obj_t *mode_play_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif