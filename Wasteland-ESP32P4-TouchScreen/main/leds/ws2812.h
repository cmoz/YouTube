#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default WS2812B strip pin/count for this board/project. */
#define WS2812_GPIO      3
#define WS2812_LED_COUNT 18

/* Configure the strip's RMT channel. Safe to call once at startup;
 * ws2812_set_pixel/clear/refresh are no-ops if this hasn't succeeded. */
esp_err_t ws2812_init(void);

void ws2812_set_pixel(int index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_clear(void);

/* Pushes whatever's been set via ws2812_set_pixel out to the strip. */
void ws2812_refresh(void);

#ifdef __cplusplus
}
#endif
