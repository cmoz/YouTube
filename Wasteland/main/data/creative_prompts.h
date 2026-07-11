#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Trimmed subset of the Maker-O-Matic 3000 creative_prompts.json,
 * ported so PLAY mode's idea panel follows the same structure:
 * scale / interaction / theme / social impact. */
typedef enum {
    PROMPT_CAT_SCALE = 0,
    PROMPT_CAT_INTERACTION,
    PROMPT_CAT_THEME,
    PROMPT_CAT_SOCIAL,
    PROMPT_CAT_COUNT,
} prompt_category_t;

const char *prompt_category_label(prompt_category_t category);
int creative_prompt_count(prompt_category_t category);
const char *creative_prompt_get(prompt_category_t category, int index);

#ifdef __cplusplus
}
#endif
