#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Default HC-SR04 pin map for this board/project. */
#define HCSR04_TRIG_GPIO 4
#define HCSR04_ECHO_GPIO 5

/* Configure TRIG/ECHO GPIOs. */
esp_err_t hcsr04_init(void);

/* Read one distance sample in centimeters.
 * Returns negative value when no valid echo is captured. */
float hcsr04_read_cm(void);

#ifdef __cplusplus
}
#endif
