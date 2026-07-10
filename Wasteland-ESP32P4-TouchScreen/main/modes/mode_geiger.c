#include "mode_geiger.h"

#include "esp_random.h"
#include <math.h>
#include <stdio.h>
#include "audio/audio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "geiger_log.h"
#include "leds/ws2812.h"

/* ------------------------------------------------------------------ */
/* look constants — dark chassis with orange instrument outlines       */
/* ------------------------------------------------------------------ */

#define GEIGER_COLOR_ACCENT      lv_color_hex(0x00CFFF)
#define GEIGER_COLOR_CASE        lv_color_hex(0x0D0D0D)
#define GEIGER_COLOR_CASE_TRIM   GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_DIAL_BG     lv_color_hex(0x141414)
#define GEIGER_COLOR_DIAL_BORDER GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_TICK        GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_NEEDLE      lv_color_hex(0x7FE8FF)
#define GEIGER_COLOR_PIVOT       lv_color_hex(0x1C1C1C)
#define GEIGER_COLOR_DIAL_LABEL  GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_KNOB        lv_color_hex(0x1A1A1A)
#define GEIGER_COLOR_KNOB_ACTIVE lv_color_hex(0x2A2A2A)
#define GEIGER_COLOR_TEXT_DARK   GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_TEXT_MUTED  lv_color_hex(0x47BFE8)
#define GEIGER_COLOR_LED_OFF     lv_color_hex(0x261A12)
#define GEIGER_COLOR_LED_ON      lv_color_hex(0x7FF4FF)
#define GEIGER_COLOR_RAD_SYMBOL  GEIGER_COLOR_ACCENT
#define GEIGER_COLOR_RAD_GLOW    lv_color_hex(0x0D3E58)
#define GEIGER_COLOR_SPIKE       lv_color_hex(0x33D7FF)
#define GEIGER_COLOR_CPM_FLASH   lv_color_hex(0xCCF7FF)

#define CASE_W          640
#define CASE_H          420
#define DIAL_DIAMETER   280
#define DIAL_PIVOT_X    132.0f
#define DIAL_PIVOT_Y    130.0f
#define NEEDLE_LEN      130.0f
#define TICK_R1         108.0f
#define TICK_R2         124.0f
#define GEIGER_DEG2RAD  0.017453293f  /* pi / 180 */

#define HISTORY_LEN              40
#define HISTORY_PUSH_INTERVAL_MS 1000
#define HISTORY_CHART_MAX_CPM    2000

#define DISTANCE_NEAR_CM         2.0f
#define DISTANCE_FAR_CM          204.0f
#define DISTANCE_WARN_HOT_CM     12.0f
#define DISTANCE_WARN_NEAR_CM    36.0f
#define DISTANCE_MIN_CPM         0.0f
/* max is deliberately high -- at point-blank range a real counter's
 * clicks blur into a near-continuous, unpleasant crackle rather than
 * discrete ticks */
#define DISTANCE_MAX_CPM         1800.0f
#define DISTANCE_SMOOTHING_FAST_ALPHA 0.72f
#define DISTANCE_SMOOTHING_SLOW_ALPHA 0.28f
#define DISTANCE_POLL_INTERVAL_MS    50

#define SPIKE_MIN_INTERVAL_MS    25000
#define SPIKE_MAX_INTERVAL_MS    60000
#define SPIKE_DURATION_MS        5000
#define SPIKE_MIN_TARGET_CPM     220.0f
#define SPIKE_MAX_TARGET_CPM     420.0f

#define CPM_FLASH_DURATION_MS    110

/* fixed warm-up delay shown on entering Geiger mode -- independent of
 * whether the sensor already has cached data, since the HC-SR04 task
 * runs continuously in the background and usually has a reading within
 * milliseconds of boot. deliberately padded past the sensor's own
 * warm-up (~1.5s) to also cover the click timer's real ramp-up to a
 * responsive cadence, which in practice takes 3-4s -- otherwise this
 * message disappears before the counter is actually keeping up. */
#define CALIBRATION_DURATION_MS  7000

/* ------------------------------------------------------------------ */
/* module state — one active Geiger mode instance at a time            */
/* ------------------------------------------------------------------ */

static struct {
    lv_obj_t *root;
    lv_obj_t *cpm_label;
    lv_obj_t *count_label;
    lv_obj_t *indicator;
    lv_obj_t *needle;
    lv_point_precise_t needle_pts[2];

    lv_obj_t *history_chart;
    lv_chart_series_t *history_series;
    lv_timer_t *history_timer;
    lv_obj_t *sensor_value_label;

    lv_obj_t *spike_label;
    lv_timer_t *spike_timer;
    lv_timer_t *spike_end_timer;
    bool spike_active;
    float pre_spike_target;

    lv_timer_t *cpm_flash_timer;
    int last_cpm_int;

    lv_timer_t *click_timer;
    lv_timer_t *rate_timer;
    lv_timer_t *indicator_hide_timer;
    lv_timer_t *calibration_timer;
    int64_t click_wait_start_us;    /* when the current click_timer wait began */
    uint32_t click_wait_period_ms;  /* total length of the current click_timer wait */

    float rate_cpm;          /* current simulated counts-per-minute */
    float rate_cpm_target;   /* where rate_cpm is drifting toward */
    float last_distance_cm;
    float distance_proximity;
    uint32_t total_counts;
    int64_t last_ui_update_us;
    bool calibrated;         /* false until the distance sensor has produced a first reading */

    mode_geiger_click_source_t click_source; /* NULL = default sim */
    mode_geiger_distance_source_t distance_source;

    /* "alive old device" flourishes -- see build_crt_overlay() */
    lv_obj_t *scan_beam_core;
    lv_obj_t *scan_beam_trail1;
    lv_obj_t *scan_beam_trail2;

    lv_obj_t *click_flash;
    lv_timer_t *click_flash_timer;

    lv_timer_t *jitter_timer;
    float needle_jitter_deg;

    lv_obj_t *danger_tint;

    lv_obj_t *flicker_overlay;
    lv_timer_t *flicker_timer;
    lv_timer_t *flicker_end_timer;
} s;

typedef enum {
    KNOB_ACTION_ZERO = 0,
    KNOB_ACTION_CHECK,
} knob_action_t;

static void update_labels(void);
static void update_needle(void);
static void update_led_scale(void);

/* ------------------------------------------------------------------ */
/* default click-timing simulation                                     */
/* ------------------------------------------------------------------ */

/* Poisson-process interval -- real decay events (and so real Geiger
 * clicks) arrive at random, not evenly spaced, even at a high average
 * rate. That randomness is what makes a hot source sound like unpleasant
 * crackle instead of a mechanical metronome. */
static uint32_t poisson_interval_ms(float cpm, float floor_ms, float cap_ms)
{
    float mean_interval_ms = 60000.0f / (cpm > 1.0f ? cpm : 1.0f);
    float u = ((float)(esp_random() % 100000) + 1.0f) / 100001.0f;
    float interval = -logf(u) * mean_interval_ms;

    if (interval < floor_ms) {
        interval = floor_ms;
    }
    if (interval > cap_ms) {
        interval = cap_ms;
    }
    return (uint32_t)interval;
}

static uint32_t default_click_source(void)
{
    return poisson_interval_ms(s.rate_cpm, 60.0f, 8000.0f);
}

static float distance_cm_to_proximity(float cm)
{
    if (cm < DISTANCE_NEAR_CM) cm = DISTANCE_NEAR_CM;
    if (cm > DISTANCE_FAR_CM) cm = DISTANCE_FAR_CM;

    float t = (cm - DISTANCE_NEAR_CM) / (DISTANCE_FAR_CM - DISTANCE_NEAR_CM);
    return 1.0f - t;
}

static float smooth_distance_cm(float previous_cm, float current_cm)
{
    if (previous_cm < 0.0f) {
        return current_cm;
    }

    float delta = current_cm - previous_cm;
    float magnitude = fabsf(delta);
    float alpha = (delta < 0.0f) ? DISTANCE_SMOOTHING_FAST_ALPHA
                                 : DISTANCE_SMOOTHING_SLOW_ALPHA;

    if (magnitude > 30.0f) {
        alpha = 0.9f;
    } else if (magnitude > 12.0f && delta < 0.0f) {
        alpha = 0.82f;
    }

    return previous_cm + delta * alpha;
}

void mode_geiger_set_click_source(mode_geiger_click_source_t source)
{
    s.click_source = source;
}

void mode_geiger_set_distance_source(mode_geiger_distance_source_t source)
{
    s.distance_source = source;
    if (!source && s.sensor_value_label) {
        lv_label_set_text(s.sensor_value_label, "--");
    }
}

static float distance_cm_to_cpm(float cm)
{
    float proximity = distance_cm_to_proximity(cm);
    /* steep curve -- stays quiet/sparse across most of the range and
     * only ramps hard in the last ~20cm, so it reads as "nothing, then
     * occasional pulses, then faster and faster" rather than being
     * busy the moment anything is nearby */
    float weighted = powf(proximity, 8.0f);

    return DISTANCE_MIN_CPM + weighted * (DISTANCE_MAX_CPM - DISTANCE_MIN_CPM);
}

static geiger_log_status_t log_status_for_distance(float cm)
{
    if (cm < 0.0f) {
        return GEIGER_LOG_STATUS_NOMINAL;
    }
    if (cm <= DISTANCE_WARN_HOT_CM) {
        return GEIGER_LOG_STATUS_HOT;
    }
    if (cm <= DISTANCE_WARN_NEAR_CM) {
        return GEIGER_LOG_STATUS_DETECTED;
    }
    return GEIGER_LOG_STATUS_NOMINAL;
}

static void update_proximity_warning(float cm)
{
    if (!s.spike_label) {
        return;
    }

    switch (log_status_for_distance(cm)) {
        case GEIGER_LOG_STATUS_HOT:
            lv_label_set_text(s.spike_label, "WARNING: SOURCE VERY CLOSE");
            lv_obj_clear_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
            break;
        case GEIGER_LOG_STATUS_DETECTED:
            lv_label_set_text(s.spike_label, "WARNING: SOURCE DETECTED");
            lv_obj_clear_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            lv_obj_add_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
            break;
    }
}

static void update_distance_input(void)
{
    if (!s.distance_source || !s.sensor_value_label) {
        return;
    }

    float cm = s.distance_source();
    if (cm < 0.0f) {
        lv_label_set_text(s.sensor_value_label, "--");
        s.last_distance_cm = -1.0f;
        /* nothing in front of the sensor -- drop the target back to
         * silence instead of leaving it pinned at whatever the last
         * detected object's rate was */
        if (!s.spike_active) {
            s.rate_cpm_target = 0.0f;
        }
    } else {
        s.last_distance_cm = smooth_distance_cm(s.last_distance_cm, cm);

        float smoothed_cm = s.last_distance_cm;
        s.distance_proximity = distance_cm_to_proximity(smoothed_cm);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f cm", smoothed_cm);
        lv_label_set_text(s.sensor_value_label, buf);

        if (!s.spike_active) {
            s.rate_cpm_target = distance_cm_to_cpm(smoothed_cm);
        }
    }

    /* the calibration warm-up gate is on a fixed timer, not on sensor
     * readiness -- the sensor task runs continuously in the background
     * and usually already has a reading, so gating on that alone would
     * make the warm-up message (and click hold-off) invisible */
    if (!s.calibrated) {
        lv_label_set_text(s.spike_label, "CALIBRATING...");
        lv_obj_clear_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (s.last_distance_cm < 0.0f) {
        lv_obj_add_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (!s.spike_active) {
        s.rate_cpm += (s.rate_cpm_target - s.rate_cpm) * 0.35f;
    }
    update_proximity_warning(s.last_distance_cm);
}

static uint32_t next_click_delay_ms(void)
{
    if (s.click_source) {
        return s.click_source();
    }
    if (s.distance_source) {
        if (!s.calibrated) {
            return DISTANCE_POLL_INTERVAL_MS;
        }
        /* driven by the smoothed rate, not the raw target -- so cadence
         * ramps in with the displayed number instead of snapping to
         * whatever the sensor sees the instant calibration finishes.
         * low floor is what lets point-blank range blur into a
         * near-continuous crackle */
        return poisson_interval_ms(s.rate_cpm, 12.0f, 8000.0f);
    }
    return default_click_source();
}

/* (re)arms click_timer for `period_ms` starting *now* -- only call this
 * at the moments LVGL itself resets the timer's internal last-run clock:
 * on creation, or from inside click_timer_cb right as it fires. */
static void arm_click_timer(uint32_t period_ms)
{
    lv_timer_set_period(s.click_timer, period_ms);
    s.click_wait_start_us = esp_timer_get_time();
    s.click_wait_period_ms = period_ms;
}

/* click_timer's wait is a single random draw taken when it was last
 * armed, based on the rate at that moment -- it does NOT get re-drawn
 * as the rate changes while it's still ticking down. Normally that's
 * fine, but when the distance sensor's target rate jumps up (object
 * suddenly close), the timer can be left counting down a stale, long
 * wait from when the rate was near zero -- which is what caused clicks
 * to hang back for seconds after a hand appeared. Called every 50ms
 * poll: re-draw a candidate wait off the *current* rate and, if it's
 * shorter than what's left of the pending wait, shrink the timer down
 * to it. Never lengthens the wait -- a falling rate is left to finish
 * out its already-queued (and usually already-short) click naturally. */
static void maybe_shorten_click_wait(void)
{
    if (!s.click_timer) {
        return;
    }

    int64_t elapsed_ms = (esp_timer_get_time() - s.click_wait_start_us) / 1000;
    if (elapsed_ms < 0) {
        elapsed_ms = 0;
    }

    uint32_t candidate_total_ms = (uint32_t)elapsed_ms + poisson_interval_ms(s.rate_cpm, 12.0f, 8000.0f);
    if (candidate_total_ms < s.click_wait_period_ms) {
        s.click_wait_period_ms = candidate_total_ms;
        lv_timer_set_period(s.click_timer, candidate_total_ms);
        /* click_wait_start_us intentionally untouched -- the timer
         * hasn't fired, so LVGL's own last-run reference hasn't moved */
    }
}

/* ------------------------------------------------------------------ */
/* rate drift                                                           */
/* ------------------------------------------------------------------ */

static void rate_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (s.distance_source) {
        update_distance_input();
        if (s.calibrated) {
            s.rate_cpm += (s.rate_cpm_target - s.rate_cpm) * 0.25f;
            maybe_shorten_click_wait();
            update_labels();
            update_needle();
            update_led_scale();
            s.last_ui_update_us = esp_timer_get_time();
        }
        lv_timer_set_period(s.rate_timer, DISTANCE_POLL_INTERVAL_MS);
        return;
    }

    /* don't stomp on an in-progress spike — it manages its own target */
    if (!s.spike_active) {
        s.rate_cpm_target = 8.0f + (float)(esp_random() % 8200) / 100.0f;
    }

    uint32_t next_ms = 8000 + (esp_random() % 12000);
    lv_timer_set_period(s.rate_timer, next_ms);
}

/* ------------------------------------------------------------------ */
/* "hot object" spike events — ambient narrative                       */
/* ------------------------------------------------------------------ */

static void spike_end_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    s.rate_cpm_target = s.pre_spike_target;
    s.spike_active = false;
    lv_obj_add_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
    s.spike_end_timer = NULL;
}

static void spike_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (s.distance_source) {
        s.spike_active = false;
        lv_timer_set_period(s.spike_timer, SPIKE_MAX_INTERVAL_MS);
        return;
    }

    if (!s.spike_active) {
        s.pre_spike_target = s.rate_cpm_target;
        float span = SPIKE_MAX_TARGET_CPM - SPIKE_MIN_TARGET_CPM;
        s.rate_cpm_target = SPIKE_MIN_TARGET_CPM
                             + ((float)(esp_random() % 1000) / 1000.0f) * span;
        s.spike_active = true;
        lv_obj_clear_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);

        if (s.spike_end_timer) {
            lv_timer_del(s.spike_end_timer);
        }
        s.spike_end_timer = lv_timer_create(spike_end_cb, SPIKE_DURATION_MS, NULL);
        lv_timer_set_repeat_count(s.spike_end_timer, 1);
    }

    uint32_t range = SPIKE_MAX_INTERVAL_MS - SPIKE_MIN_INTERVAL_MS;
    uint32_t next_ms = SPIKE_MIN_INTERVAL_MS + (esp_random() % range);
    lv_timer_set_period(s.spike_timer, next_ms);
}

/* ------------------------------------------------------------------ */
/* rolling history trail                                                */
/* ------------------------------------------------------------------ */

static void history_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    int32_t v = (int32_t)(s.rate_cpm + 0.5f);
    if (v > HISTORY_CHART_MAX_CPM) {
        v = HISTORY_CHART_MAX_CPM; /* clamp so a spike doesn't blow the axis */
    }
    lv_chart_set_next_value(s.history_chart, s.history_series, v);

    if (!s.distance_source || s.calibrated) {
        geiger_log_sample(s.rate_cpm, s.last_distance_cm, log_status_for_distance(s.last_distance_cm));
    }
}

static lv_obj_t *build_history_chart(lv_obj_t *parent, lv_coord_t width)
{
    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_remove_style_all(chart);
    lv_obj_set_size(chart, width, 46);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, HISTORY_LEN);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, HISTORY_CHART_MAX_CPM);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_line_opa(chart, LV_OPA_70, LV_PART_ITEMS);

    lv_chart_series_t *series = lv_chart_add_series(chart, GEIGER_COLOR_TEXT_MUTED,
                                                      LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < HISTORY_LEN; i++) {
        lv_chart_set_next_value(chart, series, 0);
    }

    /* point markers may show as small dots depending on LVGL defaults —
     * tune LV_PART_INDICATOR sizing here later if they look wrong on device */

    s.history_series = series;
    return chart;
}

/* ------------------------------------------------------------------ */
/* calibration warm-up                                                  */
/* ------------------------------------------------------------------ */

static void calibration_done_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    s.calibrated = true;
    s.calibration_timer = NULL;

    if (s.last_distance_cm >= 0.0f) {
        update_proximity_warning(s.last_distance_cm);
    } else {
        lv_obj_add_flag(s.spike_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ------------------------------------------------------------------ */
/* click / display update                                              */
/* ------------------------------------------------------------------ */

static void indicator_hide_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_obj_set_style_bg_color(s.indicator, GEIGER_COLOR_LED_OFF, 0);
    s.indicator_hide_timer = NULL;
}

static void flash_indicator(uint32_t duration_ms)
{
    lv_obj_set_style_bg_color(s.indicator, GEIGER_COLOR_LED_ON, 0);
    if (s.indicator_hide_timer) {
        lv_timer_del(s.indicator_hide_timer);
    }
    s.indicator_hide_timer = lv_timer_create(indicator_hide_cb, duration_ms, NULL);
    lv_timer_set_repeat_count(s.indicator_hide_timer, 1);
}

#define CLICK_FLASH_OPA          LV_OPA_10
#define CLICK_FLASH_DURATION_MS  90

static void click_flash_hide_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_obj_set_style_bg_opa(s.click_flash, LV_OPA_TRANSP, 0);
    s.click_flash_timer = NULL;
}

/* soft whole-screen glow pulse synced to each detected click -- subtle
 * (10% opacity) so it reads as the case reacting rather than a strobe */
static void flash_screen(void)
{
    if (!s.click_flash) {
        return;
    }
    lv_obj_set_style_bg_opa(s.click_flash, CLICK_FLASH_OPA, 0);
    if (s.click_flash_timer) {
        lv_timer_del(s.click_flash_timer);
    }
    s.click_flash_timer = lv_timer_create(click_flash_hide_cb, CLICK_FLASH_DURATION_MS, NULL);
    lv_timer_set_repeat_count(s.click_flash_timer, 1);
}

#define FLICKER_MIN_INTERVAL_MS 5000
#define FLICKER_MAX_INTERVAL_MS 11000
#define FLICKER_DIP_OPA          LV_OPA_40
#define FLICKER_DIP_MIN_MS       50
#define FLICKER_DIP_MAX_MS       140

static void flicker_end_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_obj_set_style_bg_opa(s.flicker_overlay, LV_OPA_TRANSP, 0);
    s.flicker_end_timer = NULL;
}

/* an old tube warming up unevenly -- the whole case dips dark for a
 * fraction of a second at random, widely spaced intervals. re-rolls its
 * own next interval each time, same trick as ui_shell's glitch timer. */
static void flicker_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    lv_obj_set_style_bg_opa(s.flicker_overlay, FLICKER_DIP_OPA, 0);
    if (s.flicker_end_timer) {
        lv_timer_del(s.flicker_end_timer);
    }
    uint32_t dip_ms = FLICKER_DIP_MIN_MS + (esp_random() % (FLICKER_DIP_MAX_MS - FLICKER_DIP_MIN_MS));
    s.flicker_end_timer = lv_timer_create(flicker_end_cb, dip_ms, NULL);
    lv_timer_set_repeat_count(s.flicker_end_timer, 1);

    uint32_t next_ms = FLICKER_MIN_INTERVAL_MS + (esp_random() % (FLICKER_MAX_INTERVAL_MS - FLICKER_MIN_INTERVAL_MS));
    lv_timer_set_period(s.flicker_timer, next_ms);
}

static void cpm_flash_hide_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    lv_obj_set_style_text_color(s.cpm_label, GEIGER_COLOR_TEXT_DARK, 0);
    s.cpm_flash_timer = NULL;
}

static void update_labels(void)
{
    char buf[16];
    int cpm_int = (int)(s.rate_cpm + 0.5f);
    snprintf(buf, sizeof(buf), "%d", cpm_int);
    lv_label_set_text(s.cpm_label, buf);

    if (cpm_int != s.last_cpm_int) {
        s.last_cpm_int = cpm_int;
        lv_obj_set_style_text_color(s.cpm_label, GEIGER_COLOR_CPM_FLASH, 0);
        if (s.cpm_flash_timer) {
            lv_timer_del(s.cpm_flash_timer);
        }
        s.cpm_flash_timer = lv_timer_create(cpm_flash_hide_cb, CPM_FLASH_DURATION_MS, NULL);
        lv_timer_set_repeat_count(s.cpm_flash_timer, 1);
    }

    snprintf(buf, sizeof(buf), "COUNTS: %lu", (unsigned long)s.total_counts);
    lv_label_set_text(s.count_label, buf);
}

/* moves the needle to reflect the current rate_cpm, mapped across a
 * -60..+60 degree sweep (0 CPM = full left, 100+ CPM = full right).
 * needle_jitter_deg (re-rolled by jitter_timer_cb) is folded in here so
 * the needle never sits dead still, even at 0 CPM -- reads as a live
 * instrument rather than a static drawing. */
static void update_needle(void)
{
    float t = s.rate_cpm / DISTANCE_MAX_CPM;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float angle_deg = -60.0f + t * 120.0f + s.needle_jitter_deg;
    float rad = angle_deg * GEIGER_DEG2RAD;

    s.needle_pts[0].x = (int32_t)DIAL_PIVOT_X;
    s.needle_pts[0].y = (int32_t)DIAL_PIVOT_Y;
    s.needle_pts[1].x = (int32_t)(DIAL_PIVOT_X + sinf(rad) * NEEDLE_LEN);
    s.needle_pts[1].y = (int32_t)(DIAL_PIVOT_Y - cosf(rad) * NEEDLE_LEN);

    lv_line_set_points(s.needle, s.needle_pts, 2);
}

#define NEEDLE_JITTER_MAX_DEG   1.2f
#define NEEDLE_JITTER_PERIOD_MS 180

static void jitter_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    float u = ((float)(esp_random() % 2001) - 1000.0f) / 1000.0f; /* -1..1 */
    s.needle_jitter_deg = u * NEEDLE_JITTER_MAX_DEG;
    update_needle();
}

/* green -> yellow -> red across the bar position, dim enough to read as
 * an instrument readout rather than a floodlight. three explicit bands:
 * green ramps in fast at the bottom, yellow now owns most of the scale
 * (roughly the middle 55%), and red only takes over right at the top.
 * yellow is also warmer/brighter (R above G, not just R==G) than a flat
 * (160,160) mix so it visually pops as its own color instead of reading
 * as a washed-out green. */
#define LED_SCALE_GREEN_END  0.20f  /* green -> yellow ramp ends here */
#define LED_SCALE_YELLOW_END 0.75f  /* yellow hold ends, red ramp begins */
#define LED_SCALE_YELLOW_R   200    /* yellow's red channel -- above base green for a warm gold, not a flat green+red mix */
#define LED_SCALE_BASE_G     160

static void led_scale_color(float t, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (t < LED_SCALE_GREEN_END) {
        float k = t / LED_SCALE_GREEN_END;
        *r = (uint8_t)(k * LED_SCALE_YELLOW_R);
        *g = LED_SCALE_BASE_G;
    } else if (t < LED_SCALE_YELLOW_END) {
        *r = LED_SCALE_YELLOW_R;
        *g = LED_SCALE_BASE_G;
    } else {
        float k = (t - LED_SCALE_YELLOW_END) / (1.0f - LED_SCALE_YELLOW_END);
        *r = (uint8_t)(LED_SCALE_YELLOW_R - k * (LED_SCALE_YELLOW_R - 180));
        *g = (uint8_t)((1.0f - k) * LED_SCALE_BASE_G);
    }
    *b = 0;
}

/* pegged at (or essentially at) the top of the scale -- reserved for
 * a tier above the plain "hot" red, since a real source doesn't get
 * meaningfully hotter than max CPM. flashes deep purple/off so it
 * reads as a step up from the steady red, not just more of the same. */
#define LED_MAX_LEVEL_THRESHOLD  0.99f
#define LED_MAX_FLASH_PERIOD_US  250000  /* 250ms per phase -> 2Hz flash */
#define LED_COLOR_MAX_R 90
#define LED_COLOR_MAX_G 0
#define LED_COLOR_MAX_B 160

/* drives the WS2812B strip as a bar-graph twin of the needle: same
 * rate_cpm mapping, updated on the same 50ms cadence so it feels just
 * as responsive. goes solid red across every LED once a source is
 * close enough to trip the on-screen "SOURCE VERY CLOSE" warning, and
 * flashes deep purple once it's pegged at the very top of the scale. */
static void update_led_scale(void)
{
    float level = s.rate_cpm / DISTANCE_MAX_CPM;
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;

    bool at_max = level >= LED_MAX_LEVEL_THRESHOLD;
    bool hot = (s.last_distance_cm >= 0.0f) && (s.last_distance_cm <= DISTANCE_WARN_HOT_CM);
    bool flash_on = ((esp_timer_get_time() / LED_MAX_FLASH_PERIOD_US) % 2) == 0;
    int lit = (int)(level * WS2812_LED_COUNT + 0.5f);

    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        if (at_max) {
            if (flash_on) {
                ws2812_set_pixel(i, LED_COLOR_MAX_R, LED_COLOR_MAX_G, LED_COLOR_MAX_B);
            } else {
                ws2812_set_pixel(i, 0, 0, 0);
            }
            continue;
        }
        if (hot) {
            ws2812_set_pixel(i, 180, 0, 0);
            continue;
        }
        if (i >= lit) {
            ws2812_set_pixel(i, 0, 0, 0);
            continue;
        }
        uint8_t r, g, b;
        led_scale_color((float)i / (float)(WS2812_LED_COUNT - 1), &r, &g, &b);
        ws2812_set_pixel(i, r, g, b);
    }
    ws2812_refresh();
}

/* washes a faint red tint over the whole screen as the reading climbs
 * toward "hot" -- stays invisible below the start threshold, then ramps
 * to a modest max so it reads as mounting alarm rather than a color
 * swap. driven off rate_cpm (not distance) so it works the same whether
 * a real sensor or the plain simulator is feeding the rate. */
#define DANGER_TINT_START_LEVEL 0.55f
#define DANGER_TINT_MAX_OPA     70   /* out of 255 -- stays a wash, never opaque */

static void update_danger_tint(void)
{
    if (!s.danger_tint) {
        return;
    }

    float level = s.rate_cpm / DISTANCE_MAX_CPM;
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;

    float t = (level - DANGER_TINT_START_LEVEL) / (1.0f - DANGER_TINT_START_LEVEL);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    lv_obj_set_style_bg_opa(s.danger_tint, (lv_opa_t)(t * DANGER_TINT_MAX_OPA), 0);
}

static void click_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);

    if (s.distance_source) {
        update_distance_input();
        if (!s.calibrated) {
            arm_click_timer(next_click_delay_ms());
            return;
        }
    }

    s.rate_cpm += (s.rate_cpm_target - s.rate_cpm) * 0.15f;

    s.total_counts++;
    int64_t now_us = esp_timer_get_time();
    if ((now_us - s.last_ui_update_us) >= 120000) {
        update_labels();
        update_needle();
        update_danger_tint();
        s.last_ui_update_us = now_us;
    }

    if (s.distance_source) {
        audio_play_click_tuned(s.distance_proximity);
    } else {
        audio_play_click();
    }

    /* Restore click lamp behavior in the top-left indicator. */
    flash_indicator(36);
    flash_screen();

    arm_click_timer(next_click_delay_ms());
}

/* ------------------------------------------------------------------ */
/* dial construction helpers                                            */
/* ------------------------------------------------------------------ */

static void add_tick(lv_obj_t *dial, lv_point_precise_t *pts, float angle_deg)
{
    float rad = angle_deg * GEIGER_DEG2RAD;

    pts[0].x = (int32_t)(DIAL_PIVOT_X + sinf(rad) * TICK_R1);
    pts[0].y = (int32_t)(DIAL_PIVOT_Y - cosf(rad) * TICK_R1);
    pts[1].x = (int32_t)(DIAL_PIVOT_X + sinf(rad) * TICK_R2);
    pts[1].y = (int32_t)(DIAL_PIVOT_Y - cosf(rad) * TICK_R2);

    lv_obj_t *line = lv_line_create(dial);
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, GEIGER_COLOR_TICK, 0);
    lv_obj_set_style_line_opa(line, LV_OPA_80, 0);
    lv_obj_set_style_line_rounded(line, false, 0);
}

static lv_obj_t *build_dial(lv_obj_t *parent)
{
    lv_obj_t *dial = lv_obj_create(parent);
    lv_obj_remove_style_all(dial);
    lv_obj_set_size(dial, DIAL_DIAMETER, DIAL_DIAMETER);
    lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dial, GEIGER_COLOR_DIAL_BG, 0);
    lv_obj_set_style_bg_opa(dial, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dial, GEIGER_COLOR_DIAL_BORDER, 0);
    lv_obj_set_style_border_width(dial, 6, 0);
    lv_obj_clear_flag(dial, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *inner_ring = lv_obj_create(dial);
    lv_obj_remove_style_all(inner_ring);
    lv_obj_set_size(inner_ring, DIAL_DIAMETER - 18, DIAL_DIAMETER - 18);
    lv_obj_set_style_radius(inner_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(inner_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(inner_ring, GEIGER_COLOR_DIAL_BORDER, 0);
    lv_obj_set_style_border_width(inner_ring, 1, 0);
    lv_obj_set_style_border_opa(inner_ring, LV_OPA_30, 0);
    lv_obj_center(inner_ring);

    static lv_point_precise_t tick_pts[5][2];
    float tick_angles[5] = { -60.0f, -30.0f, 0.0f, 30.0f, 60.0f };
    for (int i = 0; i < 5; i++) {
        add_tick(dial, tick_pts[i], tick_angles[i]);
    }

    static lv_point_precise_t minor_tick_pts[8][2];
    float minor_angles[8] = { -50.0f, -40.0f, -20.0f, -10.0f, 10.0f, 20.0f, 40.0f, 50.0f };
    for (int i = 0; i < 8; i++) {
        float rad = minor_angles[i] * GEIGER_DEG2RAD;
        minor_tick_pts[i][0].x = (int32_t)(DIAL_PIVOT_X + sinf(rad) * (TICK_R1 + 8.0f));
        minor_tick_pts[i][0].y = (int32_t)(DIAL_PIVOT_Y - cosf(rad) * (TICK_R1 + 8.0f));
        minor_tick_pts[i][1].x = (int32_t)(DIAL_PIVOT_X + sinf(rad) * TICK_R2);
        minor_tick_pts[i][1].y = (int32_t)(DIAL_PIVOT_Y - cosf(rad) * TICK_R2);

        lv_obj_t *line = lv_line_create(dial);
        lv_line_set_points(line, minor_tick_pts[i], 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, GEIGER_COLOR_TICK, 0);
        lv_obj_set_style_line_opa(line, LV_OPA_60, 0);
        lv_obj_set_style_line_rounded(line, false, 0);
    }

    s.needle = lv_line_create(dial);
    lv_obj_set_style_line_width(s.needle, 3, 0);
    lv_obj_set_style_line_color(s.needle, GEIGER_COLOR_NEEDLE, 0);
    lv_obj_set_style_line_rounded(s.needle, true, 0);

    lv_obj_t *pivot = lv_obj_create(dial);
    lv_obj_remove_style_all(pivot);
    lv_obj_set_size(pivot, 18, 18);
    lv_obj_set_style_radius(pivot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pivot, GEIGER_COLOR_PIVOT, 0);
    lv_obj_set_style_bg_opa(pivot, LV_OPA_COVER, 0);
    lv_obj_set_pos(pivot, (int32_t)DIAL_PIVOT_X - 9, (int32_t)DIAL_PIVOT_Y - 9);

    lv_obj_t *dial_caption = lv_label_create(dial);
    lv_label_set_text(dial_caption, "CPM");
    lv_obj_set_style_text_color(dial_caption, GEIGER_COLOR_DIAL_LABEL, 0);
    lv_obj_align(dial_caption, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Reintroduce a soft top gloss for a glass-covered dial feel. */
    lv_obj_t *dial_gloss = lv_obj_create(dial);
    lv_obj_remove_style_all(dial_gloss);
    lv_obj_set_size(dial_gloss, DIAL_DIAMETER - 64, DIAL_DIAMETER / 3);
    lv_obj_set_style_radius(dial_gloss, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dial_gloss, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(dial_gloss, LV_OPA_10, 0);
    lv_obj_set_style_border_width(dial_gloss, 0, 0);
    lv_obj_align(dial_gloss, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_clear_flag(dial_gloss, LV_OBJ_FLAG_CLICKABLE);

    return dial;
}

static void knob_event_cb(lv_event_t *e)
{
    lv_obj_t *knob = lv_event_get_target_obj(e);
    knob_action_t action = (knob_action_t)(intptr_t)lv_event_get_user_data(e);

    if (lv_event_get_code(e) == LV_EVENT_PRESSED) {
        lv_obj_set_style_bg_color(knob, GEIGER_COLOR_KNOB_ACTIVE, 0);
        lv_obj_set_style_outline_width(knob, 2, 0);
        lv_obj_set_style_outline_color(knob, GEIGER_COLOR_ACCENT, 0);
        lv_obj_set_style_outline_opa(knob, LV_OPA_60, 0);
        return;
    }

    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        lv_obj_set_style_bg_color(knob, GEIGER_COLOR_KNOB, 0);
        lv_obj_set_style_outline_width(knob, 1, 0);
        lv_obj_set_style_outline_color(knob, GEIGER_COLOR_CASE_TRIM, 0);
        lv_obj_set_style_outline_opa(knob, LV_OPA_40, 0);
        return;
    }

    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (action == KNOB_ACTION_ZERO) {
            s.total_counts = 0;
            update_labels();
            flash_indicator(140);
        } else {
            audio_play_click();
            flash_indicator(220);
        }
    }
}

static void build_knob(lv_obj_t *parent, const char *label_text, knob_action_t action)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *knob = lv_obj_create(col);
    lv_obj_remove_style_all(knob);
    lv_obj_set_size(knob, 64, 64);
    lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(knob, GEIGER_COLOR_KNOB, 0);
    lv_obj_set_style_bg_opa(knob, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_color(knob, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_grad_dir(knob, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(knob, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(knob, 2, 0);
    lv_obj_set_style_outline_width(knob, 1, 0);
    lv_obj_set_style_outline_pad(knob, 1, 0);
    lv_obj_set_style_outline_color(knob, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_outline_opa(knob, LV_OPA_40, 0);
    lv_obj_add_flag(knob, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(knob, knob_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)action);
    lv_obj_add_event_cb(knob, knob_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)action);
    lv_obj_add_event_cb(knob, knob_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)action);
    lv_obj_clear_flag(knob, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *glint = lv_obj_create(knob);
    lv_obj_remove_style_all(glint);
    lv_obj_set_size(glint, 32, 12);
    lv_obj_set_style_radius(glint, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(glint, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(glint, LV_OPA_20, 0);
    lv_obj_set_style_border_width(glint, 0, 0);
    lv_obj_align(glint, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_clear_flag(glint, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label = lv_label_create(col);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_top(label, 8, 0);
}

static void build_radiation_badge(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_hor(panel, 10, 0);
    lv_obj_set_style_pad_ver(panel, 8, 0);
    lv_obj_set_style_pad_row(panel, 4, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_RIGHT, -18, -154);

    lv_obj_t *top_label = lv_label_create(panel);
    lv_label_set_text(top_label, "NUKA CHECK");
    lv_obj_set_style_text_color(top_label, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(top_label, &lv_font_montserrat_14, 0);

    lv_obj_t *badge = lv_obj_create(panel);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 72, 72);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(badge, GEIGER_COLOR_RAD_SYMBOL, 0);
    lv_obj_set_style_border_width(badge, 2, 0);
    lv_obj_set_style_outline_width(badge, 2, 0);
    lv_obj_set_style_outline_pad(badge, 2, 0);
    lv_obj_set_style_outline_color(badge, GEIGER_COLOR_RAD_GLOW, 0);
    lv_obj_set_style_outline_opa(badge, LV_OPA_30, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bottom_label = lv_label_create(panel);
    lv_label_set_text(bottom_label, "RAD");
    lv_obj_set_style_text_color(bottom_label, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(bottom_label, &lv_font_montserrat_14, 0);

    /* true trefoil proportions: 60-degree blades, 60-degree gaps,
     * evenly spaced. lv_arc still draws these as bands not tapered
     * wedges — closer than before, but not pixel-true. see note above
     * on a bitmap icon for exact fidelity. */
    static const int32_t rad_center_deg[3] = { 90, 210, 330 };
    static const int32_t rad_half_width_deg = 27;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *arc = lv_arc_create(badge);
        lv_obj_set_size(arc, 52, 52);
        lv_obj_center(arc);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, GEIGER_COLOR_RAD_SYMBOL, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_arc_set_range(arc, 0, 360);
        lv_arc_set_angles(arc, rad_center_deg[i] - rad_half_width_deg,
                                rad_center_deg[i] + rad_half_width_deg);
    }

    lv_obj_t *dot = lv_obj_create(badge);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, GEIGER_COLOR_RAD_SYMBOL, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_center(dot);
}

/* ------------------------------------------------------------------ */
/* CRT overlay -- old-tube scanlines + edge vignette. Built last so it  */
/* sits on top of every other case element (dial, badge, chart, etc.). */
/* ------------------------------------------------------------------ */

#define CRT_SCANLINE_SPACING_PX 4
#define CRT_SCANLINE_COUNT      140  /* covers ~560px; harmless if it overshoots the ~514px content area, extras are just clipped */
#define CRT_SCANLINE_OPA        LV_OPA_20
#define CRT_VIGNETTE_LAYER_OPA  LV_OPA_20

#define SCAN_BEAM_TRAVEL_PX     540  /* a bit past the ~514px content area so it fully exits before wrapping */
#define SCAN_BEAM_PERIOD_MS     3200
#define SCAN_BEAM_TRAIL1_GAP_PX 6
#define SCAN_BEAM_TRAIL2_GAP_PX 14

static void scan_beam_anim_cb(void *var, int32_t v)
{
    LV_UNUSED(var);
    lv_obj_set_y(s.scan_beam_core, v);
    lv_obj_set_y(s.scan_beam_trail1, v - SCAN_BEAM_TRAIL1_GAP_PX);
    lv_obj_set_y(s.scan_beam_trail2, v - SCAN_BEAM_TRAIL2_GAP_PX);
}

static void build_crt_overlay(lv_obj_t *parent)
{
    lv_obj_t *overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);

    /* vignette: layered solid-color circles centered ON each corner (so
     * only their inward quarter is actually visible), stacked biggest to
     * smallest so the overlap compounds into a soft falloff toward the
     * corner. True radial gradients need LV_USE_DRAW_SW_COMPLEX_GRADIENTS,
     * which isn't enabled in this build -- setting one silently fell back
     * to a flat opaque fill and blanked the whole screen, so plain layered
     * circles (same primitive already used for the corner bolts / old
     * grime spots) are the reliable option here. */
    static const lv_align_t vignette_corners[4] = {
        LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT, LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT
    };
    static const int32_t vignette_radii[3] = { 260, 170, 100 };

    for (int c = 0; c < 4; c++) {
        bool left = (vignette_corners[c] == LV_ALIGN_TOP_LEFT || vignette_corners[c] == LV_ALIGN_BOTTOM_LEFT);
        bool top  = (vignette_corners[c] == LV_ALIGN_TOP_LEFT || vignette_corners[c] == LV_ALIGN_TOP_RIGHT);

        for (int i = 0; i < 3; i++) {
            int32_t r = vignette_radii[i];
            lv_obj_t *blob = lv_obj_create(overlay);
            lv_obj_remove_style_all(blob);
            lv_obj_set_size(blob, r, r);
            lv_obj_set_style_radius(blob, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(blob, lv_color_black(), 0);
            lv_obj_set_style_bg_opa(blob, CRT_VIGNETTE_LAYER_OPA, 0);
            lv_obj_clear_flag(blob, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(blob, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_align(blob, vignette_corners[c], left ? -r / 2 : r / 2, top ? -r / 2 : r / 2);
        }
    }

    /* danger tint -- full-screen red wash, opacity driven live by
     * update_danger_tint() as rate_cpm climbs. starts fully transparent. */
    s.danger_tint = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.danger_tint);
    lv_obj_set_size(s.danger_tint, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s.danger_tint, 0, 0);
    lv_obj_set_style_bg_color(s.danger_tint, lv_color_hex(0xcc1414), 0);
    lv_obj_set_style_bg_opa(s.danger_tint, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s.danger_tint, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.danger_tint, LV_OBJ_FLAG_CLICKABLE);

    /* click flash -- full-screen white pulse, snapped visible then faded
     * back out by flash_screen()/click_flash_hide_cb(). starts hidden. */
    s.click_flash = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.click_flash);
    lv_obj_set_size(s.click_flash, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s.click_flash, 0, 0);
    lv_obj_set_style_bg_color(s.click_flash, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s.click_flash, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s.click_flash, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.click_flash, LV_OBJ_FLAG_CLICKABLE);

    /* scanlines: thin horizontal rules stacked top to bottom over the
     * whole case -- reads as interference across bright elements (dial,
     * needle, text) same as it would on a real CRT, and stays subtle
     * over the already-near-black case background. */
    for (int i = 0; i < CRT_SCANLINE_COUNT; i++) {
        lv_obj_t *line = lv_obj_create(overlay);
        lv_obj_remove_style_all(line);
        lv_obj_set_size(line, lv_pct(100), 1);
        lv_obj_set_pos(line, 0, i * CRT_SCANLINE_SPACING_PX);
        lv_obj_set_style_bg_color(line, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(line, CRT_SCANLINE_OPA, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    }

    /* scan beam -- a bright line sweeping top to bottom on a loop, like
     * an old scanner/CRT refresh pass, with two fading trail lines above
     * it. sits above the scanlines so it reads as the brightest thing
     * on screen. */
    s.scan_beam_core = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.scan_beam_core);
    lv_obj_set_size(s.scan_beam_core, lv_pct(100), 2);
    lv_obj_set_style_bg_color(s.scan_beam_core, GEIGER_COLOR_NEEDLE, 0);
    lv_obj_set_style_bg_opa(s.scan_beam_core, LV_OPA_50, 0);
    lv_obj_clear_flag(s.scan_beam_core, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.scan_beam_core, LV_OBJ_FLAG_CLICKABLE);

    s.scan_beam_trail1 = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.scan_beam_trail1);
    lv_obj_set_size(s.scan_beam_trail1, lv_pct(100), 2);
    lv_obj_set_style_bg_color(s.scan_beam_trail1, GEIGER_COLOR_NEEDLE, 0);
    lv_obj_set_style_bg_opa(s.scan_beam_trail1, LV_OPA_20, 0);
    lv_obj_clear_flag(s.scan_beam_trail1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.scan_beam_trail1, LV_OBJ_FLAG_CLICKABLE);

    s.scan_beam_trail2 = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.scan_beam_trail2);
    lv_obj_set_size(s.scan_beam_trail2, lv_pct(100), 2);
    lv_obj_set_style_bg_color(s.scan_beam_trail2, GEIGER_COLOR_NEEDLE, 0);
    lv_obj_set_style_bg_opa(s.scan_beam_trail2, LV_OPA_10, 0);
    lv_obj_clear_flag(s.scan_beam_trail2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.scan_beam_trail2, LV_OBJ_FLAG_CLICKABLE);

    lv_anim_t beam_anim;
    lv_anim_init(&beam_anim);
    lv_anim_set_var(&beam_anim, s.scan_beam_core);
    lv_anim_set_values(&beam_anim, 0, SCAN_BEAM_TRAVEL_PX);
    lv_anim_set_time(&beam_anim, SCAN_BEAM_PERIOD_MS);
    lv_anim_set_repeat_count(&beam_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&beam_anim, scan_beam_anim_cb);
    lv_anim_start(&beam_anim);

    /* flicker overlay -- topmost of all, dims literally everything else
     * (including the scan beam) for a brief random dip. built last, and
     * starts fully transparent between dips. */
    s.flicker_overlay = lv_obj_create(overlay);
    lv_obj_remove_style_all(s.flicker_overlay);
    lv_obj_set_size(s.flicker_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s.flicker_overlay, 0, 0);
    lv_obj_set_style_bg_color(s.flicker_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s.flicker_overlay, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(s.flicker_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s.flicker_overlay, LV_OBJ_FLAG_CLICKABLE);
}

static void retention_scrub_delete_cb(lv_timer_t *t)
{
    lv_obj_t *scrub = (lv_obj_t *)lv_timer_get_user_data(t);
    if (scrub) {
        lv_obj_del(scrub);
    }
    lv_timer_del(t);
}

/* ------------------------------------------------------------------ */
/* lifecycle                                                            */
/* ------------------------------------------------------------------ */

static void root_delete_cb(lv_event_t *e)
{
    lv_obj_t *old_root = lv_event_get_target_obj(e);
    lv_obj_t *parent = old_root ? lv_obj_get_parent(old_root) : NULL;

    if (parent) {
        lv_obj_t *scrub = lv_obj_create(parent);
        lv_obj_remove_style_all(scrub);
        lv_obj_set_size(scrub, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(scrub, lv_color_hex(0x161616), 0);
        lv_obj_set_style_bg_opa(scrub, LV_OPA_COVER, 0);
        lv_obj_clear_flag(scrub, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(scrub, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_center(scrub);

        lv_timer_t *scrub_timer = lv_timer_create(retention_scrub_delete_cb, 90, scrub);
        lv_timer_set_repeat_count(scrub_timer, 1);
    }

    ESP_LOGI("mode_geiger", "cleanup fired: click_timer=%p rate_timer=%p", (void*)s.click_timer, (void*)s.rate_timer);
    audio_stop();
    ws2812_clear();
    ws2812_refresh();
    if (s.click_timer) {
        lv_timer_del(s.click_timer);
        s.click_timer = NULL;
    }
    if (s.rate_timer) {
        lv_timer_del(s.rate_timer);
        s.rate_timer = NULL;
    }
    if (s.indicator_hide_timer) {
        lv_timer_del(s.indicator_hide_timer);
        s.indicator_hide_timer = NULL;
    }
    if (s.history_timer) {
        lv_timer_del(s.history_timer);
        s.history_timer = NULL;
    }
    if (s.spike_timer) {
        lv_timer_del(s.spike_timer);
        s.spike_timer = NULL;
    }
    if (s.spike_end_timer) {
        lv_timer_del(s.spike_end_timer);
        s.spike_end_timer = NULL;
    }
    if (s.cpm_flash_timer) {
        lv_timer_del(s.cpm_flash_timer);
        s.cpm_flash_timer = NULL;
    }
    if (s.calibration_timer) {
        lv_timer_del(s.calibration_timer);
        s.calibration_timer = NULL;
    }
    if (s.jitter_timer) {
        lv_timer_del(s.jitter_timer);
        s.jitter_timer = NULL;
    }
    if (s.click_flash_timer) {
        lv_timer_del(s.click_flash_timer);
        s.click_flash_timer = NULL;
    }
    if (s.flicker_timer) {
        lv_timer_del(s.flicker_timer);
        s.flicker_timer = NULL;
    }
    if (s.flicker_end_timer) {
        lv_timer_del(s.flicker_end_timer);
        s.flicker_end_timer = NULL;
    }
    s.root = NULL;
}

lv_obj_t *mode_geiger_create(lv_obj_t *parent)
{
    s.rate_cpm = 0.0f;
    s.rate_cpm_target = 0.0f;
    s.last_distance_cm = -1.0f;
    s.distance_proximity = 0.0f;
    s.total_counts = 0;
    s.last_ui_update_us = 0;
    s.spike_active = false;
    s.last_cpm_int = (int)(s.rate_cpm + 0.5f);
    s.calibrated = !s.distance_source; /* no live sensor means nothing to wait on */
    s.needle_jitter_deg = 0.0f;
    /* click_source intentionally left as-is */

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    s.root = root;

    /* case */
    lv_obj_t *geiger_case = lv_obj_create(root);
    lv_obj_remove_style_all(geiger_case);
    lv_obj_set_size(geiger_case, lv_pct(100), lv_pct(100));
    lv_obj_align(geiger_case, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(geiger_case, 0, 0);
    lv_obj_set_style_bg_color(geiger_case, GEIGER_COLOR_CASE, 0);
    lv_obj_set_style_bg_opa(geiger_case, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(geiger_case, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(geiger_case, 3, 0);
    lv_obj_set_style_pad_all(geiger_case, 24, 0);
    lv_obj_set_flex_flow(geiger_case, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(geiger_case, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(geiger_case, LV_OBJ_FLAG_SCROLLABLE);

    build_radiation_badge(geiger_case);

    /* left column: dial + big number */
    lv_obj_t *left_col = lv_obj_create(geiger_case);
    lv_obj_remove_style_all(left_col);
    lv_obj_set_size(left_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_translate_x(left_col, -150, 0);
    lv_obj_set_style_translate_y(left_col, -14, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dial_box = lv_obj_create(left_col);
    lv_obj_remove_style_all(dial_box);
    lv_obj_set_size(dial_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(dial_box, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(dial_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dial_box, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(dial_box, 1, 0);
    lv_obj_set_style_radius(dial_box, 0, 0);
    lv_obj_set_style_pad_all(dial_box, 12, 0);
    lv_obj_set_flex_flow(dial_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(dial_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(dial_box, 8, 0);
    lv_obj_clear_flag(dial_box, LV_OBJ_FLAG_SCROLLABLE);

    build_dial(dial_box);

    lv_obj_t *cpm_label = lv_label_create(dial_box);
    lv_label_set_text(cpm_label, "0");
    lv_obj_set_style_text_color(cpm_label, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(cpm_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_pad_top(cpm_label, 4, 0);
    s.cpm_label = cpm_label;

    s.history_chart = build_history_chart(root, lv_pct(100));
    lv_obj_add_flag(s.history_chart, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(s.history_chart, LV_ALIGN_BOTTOM_MID, 0, -10);

    /* right column: knobs + counts */
    lv_obj_t *right_col = lv_obj_create(geiger_case);
    lv_obj_remove_style_all(right_col);
    lv_obj_set_size(right_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(right_col, 20, 0);
    lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *control_box = lv_obj_create(right_col);
    lv_obj_remove_style_all(control_box);
    lv_obj_set_size(control_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(control_box, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(control_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(control_box, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(control_box, 1, 0);
    lv_obj_set_style_radius(control_box, 0, 0);
    lv_obj_set_style_pad_all(control_box, 12, 0);
    lv_obj_set_flex_flow(control_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(control_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(control_box, 8, 0);
    lv_obj_clear_flag(control_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *control_title = lv_label_create(control_box);
    lv_label_set_text(control_title, "CONTROLS");
    lv_obj_set_style_text_color(control_title, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(control_title, &lv_font_montserrat_16, 0);

    lv_obj_t *knob_row = lv_obj_create(control_box);
    lv_obj_remove_style_all(knob_row);
    lv_obj_set_size(knob_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(knob_row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(knob_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(knob_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(knob_row, 40, 0);
    lv_obj_clear_flag(knob_row, LV_OBJ_FLAG_SCROLLABLE);

    build_knob(knob_row, "ZERO", KNOB_ACTION_ZERO);
    build_knob(knob_row, "CHECK", KNOB_ACTION_CHECK);

    lv_obj_t *count_label = lv_label_create(right_col);
    lv_label_set_text(count_label, "COUNTS: 0");
    lv_obj_set_style_text_color(count_label, GEIGER_COLOR_TEXT_MUTED, 0);
    s.count_label = count_label;

    /* activity LED, top-left of case, ignores flex flow */
    lv_obj_t *indicator = lv_obj_create(geiger_case);
    lv_obj_remove_style_all(indicator);
    lv_obj_set_size(indicator, 18, 18);
    lv_obj_set_style_radius(indicator, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(indicator, GEIGER_COLOR_LED_OFF, 0);
    lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
    lv_obj_clear_flag(indicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(indicator, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(indicator, LV_ALIGN_TOP_LEFT, 8, 8);
    s.indicator = indicator;

    /* sensor readout placeholder, top-right of case, ignores flex flow.
     * sized generously (well clear of the badge, which anchors off the
     * *bottom*-right corner instead) so the measurement reads at a
     * glance. */
    lv_obj_t *sensor_box = lv_obj_create(geiger_case);
    lv_obj_remove_style_all(sensor_box);
    lv_obj_set_size(sensor_box, 190, 80);
    lv_obj_set_style_bg_color(sensor_box, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(sensor_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sensor_box, GEIGER_COLOR_CASE_TRIM, 0);
    lv_obj_set_style_border_width(sensor_box, 1, 0);
    lv_obj_set_style_border_opa(sensor_box, LV_OPA_60, 0);
    lv_obj_set_style_pad_all(sensor_box, 8, 0);
    lv_obj_clear_flag(sensor_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(sensor_box, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(sensor_box, LV_ALIGN_TOP_RIGHT, -8, 26);

    lv_obj_t *sensor_title = lv_label_create(sensor_box);
    lv_label_set_text(sensor_title, "DIST");
    lv_obj_set_style_text_color(sensor_title, GEIGER_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(sensor_title, &lv_font_montserrat_16, 0);
    lv_obj_align(sensor_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *sensor_value = lv_label_create(sensor_box);
    lv_label_set_text(sensor_value, "--");
    lv_obj_set_style_text_color(sensor_value, GEIGER_COLOR_TEXT_DARK, 0);
    lv_obj_set_style_text_font(sensor_value, &lv_font_montserrat_32, 0);
    lv_obj_align(sensor_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    s.sensor_value_label = sensor_value;

    /* warning banner for random spikes or live proximity detection */
    lv_obj_t *spike_label = lv_label_create(geiger_case);
    lv_label_set_text(spike_label, "WARNING: SOURCE DETECTED");
    lv_obj_set_style_text_color(spike_label, GEIGER_COLOR_SPIKE, 0);
    lv_obj_set_style_text_font(spike_label, &lv_font_montserrat_16, 0);
    lv_obj_add_flag(spike_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(spike_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(spike_label, LV_ALIGN_TOP_MID, 0, 8);
    s.spike_label = spike_label;

    update_labels();
    update_needle();

    if (s.distance_source) {
        /* show calibrating / first reading right away instead of
         * waiting for the first timer tick to fire */
        update_distance_input();
    }

    uint32_t initial_click_period_ms = next_click_delay_ms();
    s.click_timer = lv_timer_create(click_timer_cb, initial_click_period_ms, NULL);
    s.click_wait_start_us = esp_timer_get_time();
    s.click_wait_period_ms = initial_click_period_ms;
    s.rate_timer = lv_timer_create(rate_timer_cb, 8000 + (esp_random() % 12000), NULL);
    s.history_timer = lv_timer_create(history_timer_cb, HISTORY_PUSH_INTERVAL_MS, NULL);

    uint32_t first_spike_ms = SPIKE_MIN_INTERVAL_MS
                               + (esp_random() % (SPIKE_MAX_INTERVAL_MS - SPIKE_MIN_INTERVAL_MS));
    s.spike_timer = lv_timer_create(spike_timer_cb, first_spike_ms, NULL);

    if (s.distance_source) {
        s.calibration_timer = lv_timer_create(calibration_done_cb, CALIBRATION_DURATION_MS, NULL);
        lv_timer_set_repeat_count(s.calibration_timer, 1);
    }

    s.jitter_timer = lv_timer_create(jitter_timer_cb, NEEDLE_JITTER_PERIOD_MS, NULL);

    build_crt_overlay(root);
    update_danger_tint();

    uint32_t first_flicker_ms = FLICKER_MIN_INTERVAL_MS
                                  + (esp_random() % (FLICKER_MAX_INTERVAL_MS - FLICKER_MIN_INTERVAL_MS));
    s.flicker_timer = lv_timer_create(flicker_timer_cb, first_flicker_ms, NULL);

    return root;
}