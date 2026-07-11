#include "leds/ws2812.h"

#include "led_strip.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "ws2812";
static led_strip_handle_t s_strip;
static bool s_ready = false;

esp_err_t ws2812_init(void)
{
    /* this strip turned out to be a 4-channel RGBW part (SK6812-style,
     * often sold as "WS2812B with W") rather than plain 3-channel
     * WS2812B -- sending only 3 bytes/pixel to a 4-byte/pixel strip
     * shifts every pixel after the first by a byte, smearing colors
     * down the whole strip. GRBW here matches the extra byte; the
     * white channel itself is left at 0 in ws2812_set_pixel so the
     * RGB color mixing is unaffected. */
    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO,
        .max_leds = WS2812_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRBW,
        .led_model = LED_MODEL_SK6812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, /* 10MHz -- gives a 0.1us RMT tick for WS2812 timing */
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        return err;
    }

    s_ready = true;
    led_strip_clear(s_strip);
    return ESP_OK;
}

void ws2812_set_pixel(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_ready) {
        return;
    }
    led_strip_set_pixel_rgbw(s_strip, index, r, g, b, 0);
}

void ws2812_clear(void)
{
    if (!s_ready) {
        return;
    }
    led_strip_clear(s_strip);
}

void ws2812_refresh(void)
{
    if (!s_ready) {
        return;
    }
    led_strip_refresh(s_strip);
}
