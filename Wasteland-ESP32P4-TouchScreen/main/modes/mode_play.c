#include "mode_play.h"
#include "data/component_list.h"
#include "data/creative_prompts.h"

#include "esp_random.h"
#include <stdio.h>
#include <string.h>

#if LV_FONT_UNSCII_16
#define PLAY_FONT (&lv_font_unscii_16)
#else
#define PLAY_FONT LV_FONT_DEFAULT
#endif

#define PLAY_BODY_FONT LV_FONT_DEFAULT

#define PLAY_COLOR_BG        lv_color_hex(0x0a0a0a)
#define PLAY_COLOR_TXT       lv_color_hex(0x9a9a9a)
#define PLAY_COLOR_ACT       lv_color_hex(0xd4823a)
#define PLAY_COLOR_LINE      lv_color_hex(0x2a2a2a)
#define PLAY_COLOR_CARD_BG   lv_color_hex(0x151008)
#define PLAY_COLOR_TAG_BG    lv_color_hex(0x2a1c0c)

#define PLAY_SLOT_COUNT 5

typedef struct {
    lv_obj_t *card;
    lv_obj_t *name_label;
    lv_obj_t *category_label;
    lv_obj_t *detail_label;
    int component_idx;
} play_slot_t;

static struct {
    lv_obj_t *panel;
    play_slot_t slot[PLAY_SLOT_COUNT];
    lv_obj_t *prompt_value_label[PROMPT_CAT_COUNT];
} s;

static void apply_slot_component(int slot, int component_idx)
{
    const component_entry_t *entry = component_list_get(component_idx);
    if (!entry) {
        return;
    }

    s.slot[slot].component_idx = component_idx;
    lv_label_set_text(s.slot[slot].name_label, entry->name);
    lv_label_set_text(s.slot[slot].category_label, component_category_label(entry->category));
    lv_label_set_text(s.slot[slot].detail_label, entry->prompt_desc);
}

static void shuffle_int_array(int *arr, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(esp_random() % (uint32_t)(i + 1));
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* every project needs a brain -- always include exactly one
 * microcontroller, then fill the rest with as many *different*
 * categories as there are remaining slots. with 5 slots and 6
 * categories, one non-microcontroller category gets left out each
 * time, chosen at random, so a run rarely repeats a category. */
static void populate_diverse_slots(void)
{
    component_category_t other_categories[COMPONENT_CAT_COUNT - 1];
    int other_count = 0;
    for (int c = 0; c < COMPONENT_CAT_COUNT; c++) {
        if (c != COMPONENT_CAT_MICROCONTROLLER) {
            other_categories[other_count++] = (component_category_t)c;
        }
    }

    int category_order[COMPONENT_CAT_COUNT - 1];
    for (int i = 0; i < other_count; i++) {
        category_order[i] = i;
    }
    shuffle_int_array(category_order, other_count);

    component_category_t slot_category[PLAY_SLOT_COUNT];
    slot_category[0] = COMPONENT_CAT_MICROCONTROLLER;
    for (int i = 1; i < PLAY_SLOT_COUNT; i++) {
        slot_category[i] = other_categories[category_order[i - 1]];
    }

    /* shuffle which visual slot gets which category so the
     * microcontroller isn't always the top card */
    int slot_order[PLAY_SLOT_COUNT];
    for (int i = 0; i < PLAY_SLOT_COUNT; i++) {
        slot_order[i] = i;
    }
    shuffle_int_array(slot_order, PLAY_SLOT_COUNT);

    for (int i = 0; i < PLAY_SLOT_COUNT; i++) {
        int component_idx = component_list_random_in_category(slot_category[i], -1);
        if (component_idx >= 0) {
            apply_slot_component(slot_order[i], component_idx);
        }
    }
}

static void reroll_slot(int slot)
{
    const component_entry_t *current = component_list_get(s.slot[slot].component_idx);
    if (!current) {
        return;
    }

    int replacement = component_list_random_in_category(current->category, s.slot[slot].component_idx);
    if (replacement < 0) {
        return;
    }

    apply_slot_component(slot, replacement);
}

static void generate_prompts(void)
{
    for (int cat = 0; cat < PROMPT_CAT_COUNT; cat++) {
        int count = creative_prompt_count((prompt_category_t)cat);
        if (count <= 0) {
            continue;
        }
        int idx = (int)(esp_random() % count);
        lv_label_set_text(s.prompt_value_label[cat], creative_prompt_get((prompt_category_t)cat, idx));
    }
}

static void slot_click_event_cb(lv_event_t *e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    reroll_slot(slot);
}

static void refresh_prompts_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    generate_prompts();
}

static void refresh_all_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    populate_diverse_slots();
}

static void panel_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    memset(&s, 0, sizeof(s));
}

static lv_obj_t *build_small_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_border_color(btn, PLAY_COLOR_ACT, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_pad_hor(btn, 10, 0);
    lv_obj_set_style_pad_ver(btn, 6, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, PLAY_COLOR_ACT, 0);
    lv_obj_set_style_text_font(label, PLAY_BODY_FONT, 0);

    return btn;
}

static lv_obj_t *build_slot_card(lv_obj_t *parent, int slot_index)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    /* fixed height rather than LV_SIZE_CONTENT + flex_grow -- avoids
     * relying on LVGL's content-size/flex interplay for a wrapped
     * label inside a grown flex item, which is untested territory in
     * this codebase. every other layout here uses fixed or purely
     * percentage sizing; keep this one consistent with that. */
    lv_obj_set_size(card, lv_pct(100), 72);
    lv_obj_set_style_bg_color(card, PLAY_COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, PLAY_COLOR_ACT, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_radius(card, 4, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, slot_click_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)slot_index);

    lv_obj_t *header_row = lv_obj_create(card);
    lv_obj_remove_style_all(header_row);
    lv_obj_set_size(header_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *name_label = lv_label_create(header_row);
    lv_label_set_text(name_label, "");
    lv_obj_set_style_text_color(name_label, PLAY_COLOR_ACT, 0);
    lv_obj_set_style_text_font(name_label, PLAY_FONT, 0);

    lv_obj_t *category_label = lv_label_create(header_row);
    lv_label_set_text(category_label, "");
    lv_obj_set_style_text_color(category_label, PLAY_COLOR_TXT, 0);
    lv_obj_set_style_bg_color(category_label, PLAY_COLOR_TAG_BG, 0);
    lv_obj_set_style_bg_opa(category_label, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(category_label, 6, 0);
    lv_obj_set_style_pad_ver(category_label, 2, 0);
    lv_obj_set_style_radius(category_label, 3, 0);

    lv_obj_t *detail_label = lv_label_create(card);
    lv_label_set_long_mode(detail_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(detail_label, lv_pct(100));
    lv_label_set_text(detail_label, "");
    lv_obj_set_style_text_color(detail_label, PLAY_COLOR_TXT, 0);
    lv_obj_set_style_text_font(detail_label, PLAY_BODY_FONT, 0);
    lv_obj_clear_flag(detail_label, LV_OBJ_FLAG_CLICKABLE);

    s.slot[slot_index].card = card;
    s.slot[slot_index].name_label = name_label;
    s.slot[slot_index].category_label = category_label;
    s.slot[slot_index].detail_label = detail_label;

    return card;
}

static lv_obj_t *build_prompt_row(lv_obj_t *parent, prompt_category_t category)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_bottom(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, prompt_category_label(category));
    lv_obj_set_style_text_color(label, PLAY_COLOR_TXT, 0);
    lv_obj_set_style_text_font(label, PLAY_BODY_FONT, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);

    lv_obj_t *value = lv_label_create(row);
    lv_label_set_long_mode(value, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(value, lv_pct(100));
    lv_label_set_text(value, "");
    lv_obj_set_style_text_color(value, PLAY_COLOR_ACT, 0);
    lv_obj_set_style_text_font(value, PLAY_FONT, 0);
    lv_obj_set_style_pad_top(value, 2, 0);

    s.prompt_value_label[category] = value;

    return row;
}

lv_obj_t *mode_play_create(lv_obj_t *parent)
{
    memset(&s, 0, sizeof(s));

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, PLAY_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(panel, panel_delete_cb, LV_EVENT_DELETE, NULL);
    s.panel = panel;

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "COMPONENT ROULETTE");
    lv_obj_set_style_text_font(title, PLAY_FONT, 0);
    lv_obj_set_style_text_color(title, PLAY_COLOR_ACT, 0);

    lv_obj_t *hairline = lv_obj_create(panel);
    lv_obj_remove_style_all(hairline);
    lv_obj_set_size(hairline, lv_pct(100), 1);
    lv_obj_set_style_bg_color(hairline, PLAY_COLOR_LINE, 0);
    lv_obj_set_style_bg_opa(hairline, LV_OPA_COVER, 0);
    lv_obj_set_style_margin_top(hairline, 6, 0);
    lv_obj_set_style_margin_bottom(hairline, 8, 0);

    /* body: 1/3 component column, 2/3 idea column */
    lv_obj_t *body = lv_obj_create(panel);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(body, 16, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *left_col = lv_obj_create(body);
    lv_obj_remove_style_all(left_col);
    lv_obj_set_size(left_col, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(left_col, 1);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left_col, 4, 0);
    /* vertical scroll as a safety net -- the 5 fixed-height cards are
     * sized to fit typical content-area heights, but if a future font
     * or layout change makes them run long, this lets it scroll
     * instead of silently clipping */
    lv_obj_set_scroll_dir(left_col, LV_DIR_VER);

    lv_obj_t *left_title = lv_label_create(left_col);
    lv_label_set_text(left_title, "COMPONENTS (TAP TO SWAP)");
    lv_obj_set_style_text_color(left_title, PLAY_COLOR_TXT, 0);
    lv_obj_set_style_text_font(left_title, PLAY_BODY_FONT, 0);

    for (int i = 0; i < PLAY_SLOT_COUNT; i++) {
        build_slot_card(left_col, i);
    }

    lv_obj_t *left_footer = lv_obj_create(left_col);
    lv_obj_remove_style_all(left_footer);
    lv_obj_set_size(left_footer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_footer, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(left_footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_footer, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(left_footer, LV_OBJ_FLAG_SCROLLABLE);

    build_small_button(left_footer, "REFRESH ALL", refresh_all_btn_event_cb);

    lv_obj_t *right_col = lv_obj_create(body);
    lv_obj_remove_style_all(right_col);
    lv_obj_set_size(right_col, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(right_col, 2);
    lv_obj_set_style_bg_color(right_col, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(right_col, PLAY_COLOR_LINE, 0);
    lv_obj_set_style_border_width(right_col, 1, 0);
    lv_obj_set_style_radius(right_col, 4, 0);
    lv_obj_set_style_pad_all(right_col, 16, 0);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *right_header = lv_obj_create(right_col);
    lv_obj_remove_style_all(right_header);
    lv_obj_set_size(right_header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_bottom(right_header, 14, 0);
    lv_obj_set_flex_flow(right_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(right_header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *right_title = lv_label_create(right_header);
    lv_label_set_text(right_title, "IDEA HELPERS");
    lv_obj_set_style_text_font(right_title, PLAY_FONT, 0);
    lv_obj_set_style_text_color(right_title, PLAY_COLOR_ACT, 0);

    build_small_button(right_header, "REFRESH", refresh_prompts_btn_event_cb);

    for (int cat = 0; cat < PROMPT_CAT_COUNT; cat++) {
        build_prompt_row(right_col, (prompt_category_t)cat);
    }

    populate_diverse_slots();
    generate_prompts();

    return panel;
}
