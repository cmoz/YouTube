#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Categories mirror the Maker-O-Matic 3000 component database
 * (microcontroller / sensor / output / fabric / power / other) so the
 * same mental model carries over between the two tools. */
typedef enum {
    COMPONENT_CAT_MICROCONTROLLER = 0,
    COMPONENT_CAT_SENSOR,
    COMPONENT_CAT_OUTPUT,
    COMPONENT_CAT_FABRIC,
    COMPONENT_CAT_POWER,
    COMPONENT_CAT_OTHER,
    COMPONENT_CAT_COUNT,
} component_category_t;

/* One salvageable/buildable component the PLAY roulette can pick from.
 * `prompt_desc` is a short plain-English phrase meant for feeding an
 * AI prompt later (e.g. "an RGB LED strip"), separate from the
 * display name so UI label and prompt wording can diverge. */
typedef struct {
    const char *name;         /* short display name, e.g. "WS2812B LED" */
    const char *prompt_desc;  /* AI-prompt phrasing, e.g. "an addressable RGB LED" */
    component_category_t category;
} component_entry_t;

int component_list_count(void);
const component_entry_t *component_list_get(int index);

/* index of a random component in the given category, or -1 if that
 * category is empty. `exclude_index` is skipped if there's another
 * candidate available (pass -1 to not exclude anything). */
int component_list_random_in_category(component_category_t category, int exclude_index);

const char *component_category_label(component_category_t category);

#ifdef __cplusplus
}
#endif
