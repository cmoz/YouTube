#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GEIGER_LOG_STATUS_NOMINAL = 0,
    GEIGER_LOG_STATUS_DETECTED,
    GEIGER_LOG_STATUS_HOT,
} geiger_log_status_t;

typedef struct {
    uint32_t uptime_ms;
    float cpm;
    float distance_cm;   /* negative if there was no sensor reading */
    geiger_log_status_t status;
} geiger_log_entry_t;

/* Records a reading if it's worth logging -- either the alert status
 * just changed, or enough time has passed since the last logged sample.
 * Cheap to call often (e.g. once a second); most calls are no-ops, so
 * callers don't need to throttle it themselves. */
void geiger_log_sample(float cpm, float distance_cm, geiger_log_status_t status);

void geiger_log_clear(void);
int geiger_log_count(void);

/* Oldest-first; idx 0 is the oldest entry still retained. */
const geiger_log_entry_t *geiger_log_get(int idx);

/* Formats one entry as a single terminal-style line, e.g.
 * "[00:03:41]   42 CPM   8CM  ! SOURCE DETECTED" */
void geiger_log_format_line(const geiger_log_entry_t *entry, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
