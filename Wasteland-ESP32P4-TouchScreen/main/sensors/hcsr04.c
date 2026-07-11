#include "sensors/hcsr04.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define HCSR04_START_TIMEOUT_US   15000
#define HCSR04_ECHO_TIMEOUT_US    15000
#define HCSR04_SAMPLE_PERIOD_MS   50
#define HCSR04_TASK_STACK_WORDS   3072

static bool s_inited = false;
static float s_latest_cm = -1.0f;
static TaskHandle_t s_task_handle = NULL;

static float hcsr04_take_reading_cm(void)
{
    gpio_set_level(HCSR04_TRIG_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_level(HCSR04_TRIG_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(HCSR04_TRIG_GPIO, 0);

    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(HCSR04_ECHO_GPIO) == 0) {
        if ((esp_timer_get_time() - t0) > HCSR04_START_TIMEOUT_US) {
            return -1.0f;
        }
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(HCSR04_ECHO_GPIO) == 1) {
        if ((esp_timer_get_time() - echo_start) > HCSR04_ECHO_TIMEOUT_US) {
            return -1.0f;
        }
    }

    float pulse_us = (float)(esp_timer_get_time() - echo_start);
    return pulse_us / 58.0f;
}

static void hcsr04_task(void *arg)
{
    (void)arg;

    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        s_latest_cm = hcsr04_take_reading_cm();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HCSR04_SAMPLE_PERIOD_MS));
    }
}

esp_err_t hcsr04_init(void)
{
    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL << HCSR04_TRIG_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&trig_cfg), "HCSR04", "TRIG gpio config failed");

    gpio_config_t echo_cfg = {
        .pin_bit_mask = (1ULL << HCSR04_ECHO_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&echo_cfg), "HCSR04", "ECHO gpio config failed");

    gpio_set_level(HCSR04_TRIG_GPIO, 0);
    s_latest_cm = -1.0f;
    s_inited = true;

    if (s_task_handle == NULL) {
        BaseType_t ok = xTaskCreate(hcsr04_task, "hcsr04_task", HCSR04_TASK_STACK_WORDS,
                                     NULL, configMAX_PRIORITIES - 6, &s_task_handle);
        if (ok != pdPASS) {
            s_inited = false;
            s_task_handle = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}

float hcsr04_read_cm(void)
{
    if (!s_inited) {
        return -1.0f;
    }
    return s_latest_cm;
}
