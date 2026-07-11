// main.c — minimal display/touch bring-up test
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ldo_regulator.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "bsp_display.h"
#include "bsp_illuminate.h"
#include "bsp_i2c.h"
#include "ui/ui_shell.h"
#include "modes/mode_placeholder.h"
#include "modes/mode_geiger.h"
#include "audio/audio.h"
#include "modes/mode_dormant.h"
#include "modes/mode_splash.h"
#include "modes/mode_log.h"
#include "sensors/hcsr04.h"
#include "data/component_list.h" 
#include "modes/mode_play.h"
#include "modes/mode_tools.h"
#include "data/tutorial_list.h"
#include "modes/mode_cmoz.h"
#include "modes/mode_demo.h"
#include "leds/ws2812.h"

static const char *TAG = "MAIN";

static esp_ldo_channel_handle_t ldo3 = NULL;
static esp_ldo_channel_handle_t ldo4 = NULL;

static void init_fail(const char *name, esp_err_t err)
{
    while (1) {
        ESP_LOGE(TAG, "%s init failed: %s", name, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void on_nav_mode(ui_shell_mode_t mode, void *user_data)
{
    (void)user_data;
    lv_obj_t *content = NULL;

    if (mode == UI_SHELL_MODE_GEIGER) {
        content = mode_geiger_create(ui_shell_get_content_area());
    } else if (mode == UI_SHELL_MODE_LOG) {
        content = mode_log_create(ui_shell_get_content_area());
    } else if (mode == UI_SHELL_MODE_PLAY) {
        content = mode_play_create(ui_shell_get_content_area());
    } else if (mode == UI_SHELL_MODE_SCAN) {
        content = mode_tools_create(ui_shell_get_content_area());
    } else if (mode == UI_SHELL_MODE_CMOZ) {
        content = mode_cmoz_create(ui_shell_get_content_area());
    } else if (mode == UI_SHELL_MODE_DEMO) {
        content = mode_demo_create(ui_shell_get_content_area());
    } else {
        content = mode_placeholder_create(ui_shell_get_content_area(), mode);
    }

    ui_shell_set_content(content);
}

#define SPLASH_DURATION_MS 2500

static void splash_timeout_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    ui_shell_set_content(mode_dormant_create(ui_shell_get_content_area()));
}

static void on_mute_toggle(bool muted, void *user_data)
{
    (void)user_data;
    audio_set_muted(muted);
    ESP_LOGI(TAG, "Audio muted: %s", muted ? "yes" : "no");
}

void app_main(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Wasteland display test starting");

    esp_ldo_channel_config_t ldo3_cfg = { .chan_id = 3, .voltage_mv = 2500 };
    err = esp_ldo_acquire_channel(&ldo3_cfg, &ldo3);
    if (err != ESP_OK) init_fail("ldo3", err);

    esp_ldo_channel_config_t ldo4_cfg = { .chan_id = 4, .voltage_mv = 3300 };
    err = esp_ldo_acquire_channel(&ldo4_cfg, &ldo4);
    if (err != ESP_OK) init_fail("ldo4", err);

    ESP_LOGI(TAG, "LDO ok");

    err = i2c_init();
    if (err != ESP_OK) init_fail("i2c", err);
    ESP_LOGI(TAG, "I2C ok");

    err = touch_init();
    if (err != ESP_OK) init_fail("touch", err);
    ESP_LOGI(TAG, "Touch ok");

    err = display_init();
    if (err != ESP_OK) init_fail("display", err);
    ESP_LOGI(TAG, "Display ok");

    err = set_lcd_blight(100);
    if (err != ESP_OK) init_fail("backlight", err);
    ESP_LOGI(TAG, "Backlight ok");

    if (!audio_init()) {
    init_fail("audio", ESP_FAIL);
}
ESP_LOGI(TAG, "Audio ok");

    err = hcsr04_init();
    if (err == ESP_OK) {
        mode_geiger_set_distance_source(hcsr04_read_cm);
        ESP_LOGI(TAG, "HC-SR04 ready on TRIG=%d ECHO=%d", HCSR04_TRIG_GPIO, HCSR04_ECHO_GPIO);
    } else {
        ESP_LOGW(TAG, "HC-SR04 init failed: %s (DIST stays simulated)", esp_err_to_name(err));
    }

    err = ws2812_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WS2812B ready on GPIO=%d, %d LEDs", WS2812_GPIO, WS2812_LED_COUNT);
    } else {
        ESP_LOGW(TAG, "WS2812B init failed: %s (LED scale stays dark)", esp_err_to_name(err));
    }

    if (lvgl_port_lock(0)) {
        ui_shell_init();
        ui_shell_set_mode_cb(on_nav_mode, NULL);
        ui_shell_set_mute_cb(on_mute_toggle, NULL);
        ui_shell_set_content(mode_splash_create(ui_shell_get_content_area()));
        ui_shell_skip_reveal();
        lv_timer_t *splash_timer = lv_timer_create(splash_timeout_cb, SPLASH_DURATION_MS, NULL);
        lv_timer_set_repeat_count(splash_timer, 1);
        lvgl_port_unlock();
    }

ESP_LOGI(TAG, "UI shell created");
//ESP_LOGI(TAG, "component_list count: %d", component_list_count());
ESP_LOGI(TAG, "tutorial_list count: %d", tutorial_list_count());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}