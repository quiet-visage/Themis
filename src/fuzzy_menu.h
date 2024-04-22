#pragma once

#include "editor/line_editor.h"
#include "resources/resources.h"

#define FUZZY_MENU_OPTION_NAME_CAP 256
#define FUZZY_MENU_OPTIONS_CAP 256

typedef struct {
    c32_t name[FUZZY_MENU_OPTION_NAME_CAP];
    uint edit_distance;
    size_t name_len;
    enum icon icon;
} fuzzy_menu_option_t;

typedef struct {
    line_editor_t editor;
    float vertical_scroll;
    ff_glyph_vec_t glyphs;
    size_t previous_buffer_size;
    size_t selected;
    fuzzy_menu_option_t options[FUZZY_MENU_OPTIONS_CAP];
    size_t options_count;
    motion_t motion;
} fuzzy_menu_t;

typedef struct {
    float bg_x;
    float bg_width;

    float bounds_x;
    float bounds_width;

    float editor_y;
    float editor_bg_y;

    float options_x;
    float options_y;
    float options_bg_y;

    float editor_width;
    float editor_height;
    float editor_bg_height;

    float options_height;
    float options_bg_height;
} fuzzy_menu_dimensions_t;

void fuzzy_menu_create(fuzzy_menu_t *o);
void fuzzy_menu_destroy(fuzzy_menu_t *fm);
void fuzzy_menu_push_option(fuzzy_menu_t *fm, const c32_t *option,
                            size_t len);
void fuzzy_menu_push_option_with_icon(fuzzy_menu_t *fm,
                                      const c32_t *option, size_t len,
                                      enum icon icon);
void fuzzy_menu_reset(fuzzy_menu_t *fm);
void fuzzy_menu_sel_next(fuzzy_menu_t *fm);
void fuzzy_menu_sel_prev(fuzzy_menu_t *fm);
fuzzy_menu_dimensions_t fuzzy_menu_get_dimensions(
    fuzzy_menu_t *fm, ff_typo_t typo, Vector2 window_size);
const c32_t *fuzzy_menu_handle_user_input(fuzzy_menu_t *fm);
bool fuzzy_menu_buffer_changed(fuzzy_menu_t *fm);
void fuzzy_menu_on_buffer_change(fuzzy_menu_t *fm);
void fuzzy_menu_draw_options(fuzzy_menu_t *fm, ff_typo_t typo,
                             fuzzy_menu_dimensions_t dimensions);
void fuzzy_menu_draw_editor(fuzzy_menu_t *fm, ff_typo_t typo,
                            int focus_flags,
                            fuzzy_menu_dimensions_t dimensions);
