#include "data/creative_prompts.h"
#include <stddef.h>

static const char *s_scale[] = {
    "Make it tiny/wearable",
    "Make it desk/tabletop sized",
    "Make it portable",
    "Make it fit in a matchbox",
    "Make it palm-sized",
    "Make it oversized (comically large)",
};

static const char *s_interaction[] = {
    "Add sound/music",
    "Make it respond to touch",
    "Make it glow or light up",
    "Add movement/vibration",
    "Make it reactive to environment",
    "Gesture controlled",
    "Voice activated",
    "Squeeze/pressure sensitive",
    "Motion activated",
    "Proximity sensing",
};

static const char *s_theme[] = {
    "Cyberpunk/neon",
    "Steampunk/Victorian",
    "Nature/organic",
    "Space/sci-fi",
    "Underwater/aquatic",
    "Fantasy/magical",
    "Minimalist/zen",
    "Retro/vintage",
    "Industrial/mechanical",
    "Bioluminescent",
};

static const char *s_social[] = {
    "For people with visual impairments",
    "For people with hearing impairments",
    "For people with limited mobility",
    "For forgetfulness/organization",
    "To reduce environmental impact",
    "For elderly or aging users",
    "For mental health awareness",
    "For community safety",
    "For disaster/emergency preparedness",
    "To combat social isolation",
};

static const char *s_power[] = {
    "Battery powered",
    "Solar powered",
    "USB powered",
    "Hand-cranked",
    "Kinetic/motion-charged",
    "Wireless charging",
};

static const char *s_materials[] = {
    "3D printed",
    "Laser cut",
    "Repurposed/upcycled",
    "Conductive fabric",
    "Cardboard/paper",
    "Found objects/junk drawer",
};

static const char *s_constraint[] = {
    "No screws or glue",
    "One button only",
    "Must fit in a pocket",
    "Build it in under an hour",
    "Only use what's already in your bin",
    "No soldering allowed",
};

static const char *s_category_labels[PROMPT_CAT_COUNT] = {
    "SCALE",
    "INTERACTION",
    "THEME",
    "SOCIAL IMPACT",
    "POWER SOURCE",
    "MATERIALS",
    "CONSTRAINT",
};

static const struct {
    const char **items;
    int count;
} s_tables[PROMPT_CAT_COUNT] = {
    [PROMPT_CAT_SCALE]       = { s_scale,       sizeof(s_scale) / sizeof(s_scale[0]) },
    [PROMPT_CAT_INTERACTION] = { s_interaction, sizeof(s_interaction) / sizeof(s_interaction[0]) },
    [PROMPT_CAT_THEME]       = { s_theme,       sizeof(s_theme) / sizeof(s_theme[0]) },
    [PROMPT_CAT_SOCIAL]      = { s_social,      sizeof(s_social) / sizeof(s_social[0]) },
    [PROMPT_CAT_POWER]       = { s_power,       sizeof(s_power) / sizeof(s_power[0]) },
    [PROMPT_CAT_MATERIALS]   = { s_materials,   sizeof(s_materials) / sizeof(s_materials[0]) },
    [PROMPT_CAT_CONSTRAINT]  = { s_constraint,  sizeof(s_constraint) / sizeof(s_constraint[0]) },
};

const char *prompt_category_label(prompt_category_t category)
{
    if (category < 0 || category >= PROMPT_CAT_COUNT) {
        return "";
    }
    return s_category_labels[category];
}

int creative_prompt_count(prompt_category_t category)
{
    if (category < 0 || category >= PROMPT_CAT_COUNT) {
        return 0;
    }
    return s_tables[category].count;
}

const char *creative_prompt_get(prompt_category_t category, int index)
{
    if (category < 0 || category >= PROMPT_CAT_COUNT) {
        return NULL;
    }
    if (index < 0 || index >= s_tables[category].count) {
        return NULL;
    }
    return s_tables[category].items[index];
}
