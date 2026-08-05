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

static const char *s_body_part[] = {
    "Head/hat accessory",
    "Wrist/arm band",
    "Finger/ring",
    "Ear/earring",
    "Chest/necklace",
    "Ankle/shoe",
    "Belt/waist",
    "Clothing patch/badge",
    "Shoulder piece",
    "Back pack attachment",
    "Glasses/eyewear",
    "Glove/hand",
    "Full bodysuit integration",
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
    "Temperature responsive",
    "Motion activated",
    "Proximity sensing",
    "Tilt/orientation based",
    "Breath controlled",
    "Heart rate responsive",
};

static const char *s_creative_twist[] = {
    "Do the opposite of your first idea",
    "Make it rainbow/multicolor",
    "What if it was waterproof?",
    "Make it communicate with another device",
    "Add a secret feature",
    "Make it solve a daily annoyance",
    "What would a 5-year-old do with this?",
    "Turn it into wearable art",
    "Make it educational",
    "Add gamification",
    "Make it tell a story",
    "Give it a personality",
    "Make it collaborative (requires 2+ people)",
    "Add randomness/unpredictability",
    "Make it reversible (works two ways)",
    "What if it was invisible?",
    "Make it modular/customizable",
    "Add a countdown or timer element",
    "Make it grow or evolve over time",
    "What if animals could use it?",
};

static const char *s_theme[] = {
    "Cyberpunk/neon",
    "Steampunk/Victorian",
    "Nature/organic",
    "Space/sci-fi",
    "Underwater/aquatic",
    "Fantasy/magical",
    "Minimalist/zen",
    "Maximalist/chaotic",
    "Retro/vintage",
    "Industrial/mechanical",
    "Goth/dark",
    "Pastel/kawaii",
    "Holographic/iridescent",
    "Bioluminescent",
    "Crystal/gemstone inspired",
};

static const char *s_materials[] = {
    "Make it soft and squishy",
    "Make it metallic and shiny",
    "Use unexpected materials",
    "Make it fuzzy or furry",
    "Transparent or translucent",
    "Rough and textured",
    "Smooth and polished",
    "Flexible and bendable",
    "Make it glow in the dark",
    "Use recycled materials",
};

static const char *s_function[] = {
    "Make it solve a problem",
    "Make it completely useless (but fun!)",
    "Make it helpful for accessibility",
    "Make it a teaching tool",
    "Make it a game",
    "Make it decorative",
    "Make it functional AND beautiful",
    "Make it a conversation starter",
    "Make it therapeutic/calming",
    "Make it energizing/exciting",
    "Make it measure something",
    "Make it remind you of something",
    "Make it help you focus",
    "Make it help you relax",
};

static const char *s_audience[] = {
    "Make it for kids",
    "Make it for seniors",
    "Make it for pets",
    "Make it for makers",
    "Make it for non-technical people",
    "Make it for performers/artists",
    "Make it for athletes",
    "Make it for outdoor enthusiasts",
    "Make it for homebodies",
};

static const char *s_time_season[] = {
    "Make it seasonal (specific season)",
    "Make it for nighttime use",
    "Make it for daytime use",
    "Make it holiday-themed",
    "Make it weather-reactive",
    "Make it astronomy-related (moon/stars)",
};

static const char *s_emotion[] = {
    "Make it joyful and playful",
    "Make it mysterious and intriguing",
    "Make it calming and peaceful",
    "Make it energetic and exciting",
    "Make it whimsical and silly",
    "Make it elegant and sophisticated",
    "Make it rebellious and bold",
    "Make it nostalgic",
    "Make it futuristic",
};

static const char *s_challenge[] = {
    "Use only 3 colors",
    "Must work without WiFi",
    "Must last 1 week on battery",
    "Must be silent",
    "Must make noise",
    "No screen/display allowed",
    "Must use all analog components",
    "Must fit in your pocket",
    "Use only recycled parts",
    "Must work outdoors",
    "Build it in under 2 hours",
};

static const char *s_inspiration[] = {
    "Inspired by nature",
    "Inspired by architecture",
    "Inspired by a song or music",
    "Inspired by a movie/book",
    "Inspired by street art",
    "Inspired by fashion runways",
    "Inspired by video games",
    "Inspired by your local community",
    "Inspired by a memory",
    "Inspired by a dream",
};

static const char *s_social[] = {
    "For people with visual impairments",
    "For people with hearing impairments",
    "For people with limited mobility",
    "For cognitive accessibility",
    "For forgetfulness/organization",
    "To reduce environmental impact",
    "To promote recycling or upcycling",
    "For elderly or aging users",
    "For mental health awareness",
    "For community safety",
    "For disaster/emergency preparedness",
    "For educational access",
    "For language barriers/translation",
    "For low-resource communities",
    "To combat social isolation",
    "For public health monitoring",
    "To increase civic engagement",
    "For sustainable energy use",
    "To support caregivers",
    "For inclusive play/recreation",
    "For family communication",
    "For personal safety",
    "To promote physical activity",
    "To assist with daily living tasks",
    "To enhance remote communication",
    "To support independent living",
    "To promote digital literacy",
    "To encourage creativity and making",
    "To preserve cultural heritage",
};

static const char *s_category_labels[PROMPT_CAT_COUNT] = {
    "SCALE",
    "BODY PART",
    "INTERACTION",
    "CREATIVE TWIST",
    "THEME",
    "MATERIALS",
    "FUNCTION",
    "AUDIENCE",
    "TIME / SEASON",
    "EMOTION",
    "CHALLENGE",
    "INSPIRATION",
    "SOCIAL IMPACT",
};

static const struct {
    const char **items;
    int count;
} s_tables[PROMPT_CAT_COUNT] = {
    [PROMPT_CAT_SCALE]          = { s_scale,          sizeof(s_scale) / sizeof(s_scale[0]) },
    [PROMPT_CAT_BODY_PART]      = { s_body_part,      sizeof(s_body_part) / sizeof(s_body_part[0]) },
    [PROMPT_CAT_INTERACTION]    = { s_interaction,    sizeof(s_interaction) / sizeof(s_interaction[0]) },
    [PROMPT_CAT_CREATIVE_TWIST] = { s_creative_twist, sizeof(s_creative_twist) / sizeof(s_creative_twist[0]) },
    [PROMPT_CAT_THEME]          = { s_theme,          sizeof(s_theme) / sizeof(s_theme[0]) },
    [PROMPT_CAT_MATERIALS]      = { s_materials,      sizeof(s_materials) / sizeof(s_materials[0]) },
    [PROMPT_CAT_FUNCTION]       = { s_function,       sizeof(s_function) / sizeof(s_function[0]) },
    [PROMPT_CAT_AUDIENCE]       = { s_audience,       sizeof(s_audience) / sizeof(s_audience[0]) },
    [PROMPT_CAT_TIME_SEASON]    = { s_time_season,    sizeof(s_time_season) / sizeof(s_time_season[0]) },
    [PROMPT_CAT_EMOTION]        = { s_emotion,        sizeof(s_emotion) / sizeof(s_emotion[0]) },
    [PROMPT_CAT_CHALLENGE]      = { s_challenge,      sizeof(s_challenge) / sizeof(s_challenge[0]) },
    [PROMPT_CAT_INSPIRATION]    = { s_inspiration,    sizeof(s_inspiration) / sizeof(s_inspiration[0]) },
    [PROMPT_CAT_SOCIAL]         = { s_social,         sizeof(s_social) / sizeof(s_social[0]) },
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
