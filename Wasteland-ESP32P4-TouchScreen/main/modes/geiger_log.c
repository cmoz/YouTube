#include "geiger_log.h"

#include "esp_timer.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define GEIGER_LOG_CAPACITY        24
#define GEIGER_LOG_MIN_INTERVAL_MS 3000

static struct {
    geiger_log_entry_t entries[GEIGER_LOG_CAPACITY];
    int count;
    int64_t last_push_us;
    geiger_log_status_t last_status;
    bool have_status;
} s;

void geiger_log_sample(float cpm, float distance_cm, geiger_log_status_t status)
{
    int64_t now_us = esp_timer_get_time();
    bool status_changed = !s.have_status || status != s.last_status;
    bool interval_elapsed = !s.have_status ||
        (now_us - s.last_push_us) >= ((int64_t)GEIGER_LOG_MIN_INTERVAL_MS * 1000);

    if (!status_changed && !interval_elapsed) {
        return;
    }

    geiger_log_entry_t entry = {
        .uptime_ms = (uint32_t)(now_us / 1000),
        .cpm = cpm,
        .distance_cm = distance_cm,
        .status = status,
    };

    if (s.count == GEIGER_LOG_CAPACITY) {
        memmove(&s.entries[0], &s.entries[1], sizeof(s.entries[0]) * (GEIGER_LOG_CAPACITY - 1));
        s.entries[GEIGER_LOG_CAPACITY - 1] = entry;
    } else {
        s.entries[s.count++] = entry;
    }

    s.last_push_us = now_us;
    s.last_status = status;
    s.have_status = true;
}

void geiger_log_clear(void)
{
    s.count = 0;
    s.have_status = false;
}

int geiger_log_count(void)
{
    return s.count;
}

const geiger_log_entry_t *geiger_log_get(int idx)
{
    if (idx < 0 || idx >= s.count) {
        return NULL;
    }
    return &s.entries[idx];
}

void geiger_log_format_line(const geiger_log_entry_t *entry, char *buf, size_t buf_size)
{
    uint32_t total_s = entry->uptime_ms / 1000;
    uint32_t hh = (total_s / 3600) % 100; /* cosmetic clamp -- sessions won't run 100h */
    uint32_t mm = (total_s / 60) % 60;
    uint32_t ss = total_s % 60;

    char dist_buf[8];
    if (entry->distance_cm >= 0.0f) {
        snprintf(dist_buf, sizeof(dist_buf), "%3.0fCM", entry->distance_cm);
    } else {
        snprintf(dist_buf, sizeof(dist_buf), "  --");
    }

    const char *tag;
    switch (entry->status) {
        case GEIGER_LOG_STATUS_HOT:      tag = "  !! SOURCE VERY CLOSE"; break;
        case GEIGER_LOG_STATUS_DETECTED: tag = "  ! SOURCE DETECTED";    break;
        default:                         tag = "";                      break;
    }

    snprintf(buf, buf_size, "[%02lu:%02lu:%02lu]  %4d CPM  %s%s",
              (unsigned long)hh, (unsigned long)mm, (unsigned long)ss,
              (int)(entry->cpm + 0.5f), dist_buf, tag);
}
