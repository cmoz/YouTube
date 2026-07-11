#include "audio.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"
#include <math.h>

static const char *TAG = "audio";
static bool s_muted = false;

#define AUDIO_GPIO_LRCLK   21
#define AUDIO_GPIO_BCLK    22
#define AUDIO_GPIO_SDATA   23
#define AUDIO_GPIO_CTRL    30   /* amp enable — ACTIVE LOW (0 = on, 1 = off) */

#define SAMPLE_RATE        44100
#define CLICK_DURATION_MS  4
#define CLICK_SAMPLES       ((SAMPLE_RATE * CLICK_DURATION_MS) / 1000)

typedef enum {
    AUDIO_EVENT_CLICK = 0,
} audio_event_t;

typedef struct {
    audio_event_t type;
    float intensity;
} audio_msg_t;

static i2s_chan_handle_t s_tx_chan;
static QueueHandle_t s_audio_queue;
static bool s_initialized = false;

static void audio_amp_enable(bool enable)
{
    gpio_set_direction(AUDIO_GPIO_CTRL, GPIO_MODE_OUTPUT);
    int level = enable ? 0 : 1; /* inverted: LOW turns amp on */
    gpio_set_level(AUDIO_GPIO_CTRL, level);
}

static esp_err_t i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_GPIO_BCLK,
            .ws   = AUDIO_GPIO_LRCLK,
            .dout = AUDIO_GPIO_SDATA,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    err = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_channel_enable(s_tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/* real Geiger-Muller audio output is a raw pulse straight to a speaker:
 * a near-instant broadband transient with essentially no pitch and no
 * ring-out, not a decaying "note". keep this short and mostly noise. */
/* the I2S TX DMA ring below is dma_desc_num(6) * dma_frame_num(240) =
 * 1440 frames (~32.6ms @ 44.1kHz) and does NOT auto-insert silence when
 * writes stop -- it just keeps looping whatever's still sitting in it.
 * every click must be followed by enough silence to fully overwrite
 * that ring, or a stale click fragment can loop forever once the queue
 * goes idle -- which is what let clicking survive mute / leaving the
 * Geiger page. keep CLICK_DURATION_MS + SILENCE_TAIL_MS comfortably
 * above ~33ms. */
#define SILENCE_TAIL_MS    40
#define SILENCE_TAIL_SAMPLES ((SAMPLE_RATE * SILENCE_TAIL_MS) / 1000)

static void render_click(float intensity)
{
    static int16_t buf[CLICK_SAMPLES * 2];
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    float freq_hz = 1800.0f + intensity * 3400.0f;
    float tone_mix = 0.06f + intensity * 0.18f; /* mostly noise, but with an audible higher-pitched edge */
    float gain = 1.5f + intensity * 0.6f; /* boosted overall loudness */

    float prev_noise = 0.0f;
    for (int i = 0; i < CLICK_SAMPLES; i++) {
        /* steep exponential decay -- instant peak, gone almost as fast.
         * very little transient tail, just a sharp pop. */
        float envelope = expf(-11.0f * (float)i / (float)CLICK_SAMPLES);

        float noise = (float)((int32_t)(esp_random() % 20000) - 10000);
        /* differentiate the noise (crude high-pass) so it crackles
         * instead of reading as flat broadband hiss */
        float noise_hp = noise - prev_noise;
        prev_noise = noise;

        float phase = 2.0f * (float)M_PI * freq_hz * ((float)i / (float)SAMPLE_RATE);
        float tone = sinf(phase) * 18000.0f;
        float mixed = ((1.0f - tone_mix) * noise_hp * 0.95f + tone_mix * tone) * envelope * gain;
        /* clamp -- mixed can now exceed int16 range at full gain, and an
         * unclamped cast wraps to garbage instead of saturating cleanly */
        if (mixed > 32767.0f) mixed = 32767.0f;
        if (mixed < -32768.0f) mixed = -32768.0f;
        int16_t sample = (int16_t)mixed;

        buf[i * 2]     = sample;
        buf[i * 2 + 1] = sample;
    }

    size_t written = 0;
    i2s_channel_write(s_tx_chan, buf, sizeof(buf), &written, portMAX_DELAY);

    /* fill the DMA ring with silence afterward — otherwise the I2S
     * peripheral just keeps looping this click's audio forever once
     * we stop feeding it new data */
    static int16_t silence[SILENCE_TAIL_SAMPLES * 2] = { 0 };
    i2s_channel_write(s_tx_chan, silence, sizeof(silence), &written, portMAX_DELAY);
}

static void audio_task(void *arg)
{
    (void)arg;
    audio_msg_t msg;

    while (1) {
        if (xQueueReceive(s_audio_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.type == AUDIO_EVENT_CLICK) {
                render_click(msg.intensity);
            }
        }
    }
}

bool audio_init(void)
{
    if (s_initialized) {
        return true;
    }

    audio_amp_enable(false); /* stay muted until I2S is running */

    if (i2s_init() != ESP_OK) {
        return false;
    }

    audio_amp_enable(true);

    s_audio_queue = xQueueCreate(16, sizeof(audio_msg_t));
    if (s_audio_queue == NULL) {
        ESP_LOGE(TAG, "failed to create audio queue");
        return false;
    }

    BaseType_t ok = xTaskCreate(audio_task, "audio_task", 4096, NULL,
                                 configMAX_PRIORITIES - 5, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create audio task");
        return false;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "audio initialized");
    return true;
}

void audio_stop(void)
{
    if (!s_initialized) {
        return;
    }
    xQueueReset(s_audio_queue);
}

void audio_play_click(void)
{
    audio_play_click_tuned(0.0f);
}

void audio_play_click_tuned(float intensity)
{
    if (!s_initialized || s_muted) {
        return;
    }
    audio_msg_t msg = {
        .type = AUDIO_EVENT_CLICK,
        .intensity = intensity,
    };
    xQueueSend(s_audio_queue, &msg, 0); /* non-blocking, drop if full */
}

void audio_set_muted(bool muted)
{
    s_muted = muted;
    if (muted) {
        audio_stop();
    }
}

bool audio_is_muted(void)
{
    return s_muted;
}