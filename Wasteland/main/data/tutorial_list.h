#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *title;
    const char *video_id;   /* YouTube video ID, e.g. "s4t1PC_Bwgo" */
} tutorial_entry_t;

int tutorial_list_count(void);
const tutorial_entry_t *tutorial_list_get(int index);

#ifdef __cplusplus
}
#endif