#pragma once

#include <uchar.h>

#include "../editor/line_editor.h"
#include "../resources/resources.h"

#define FUZZY_MENU_OPTION_NAME_CAP 256
#define FUZZY_MENU_OPTIONS_CAP 256

struct fuzzy_menu_colors {
    int menu_bg;
    int editor_bg;
    int editor_fg;
    int option_fg;
    int option_sel_fg;
    int option_sel_bg;
};

struct fuzzy_menu_option {
    char32_t name[FUZZY_MENU_OPTION_NAME_CAP];
    uint edit_distance;
    size_t name_len;
    enum icon icon;
};

struct fuzzy_menu {
    struct line_editor editor;
    struct utf32_str editor_buffer;
    float vertical_scroll;
    struct ff_glyphs_vector glyphs;
    size_t previous_buffer_size;
    size_t selected;
    struct fuzzy_menu_option options[FUZZY_MENU_OPTIONS_CAP];
    size_t options_count;
    struct motion motion;
};

struct fuzzy_menu_dimensions {
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
};

void fuzzy_menu_create(struct fuzzy_menu *o);
void fuzzy_menu_destroy(struct fuzzy_menu *fm);
void fuzzy_menu_push_option(struct fuzzy_menu *fm,
                            const char32_t *option, size_t len);
void fuzzy_menu_push_option_with_icon(struct fuzzy_menu *fm,
                                      const char32_t *option,
                                      size_t len, enum icon icon);
const char32_t *fuzzy_menu_perform(struct fuzzy_menu *fm,
                                   struct ff_typography typo,
                                   int focus_flags);
void fuzzy_menu_reset(struct fuzzy_menu *fm);
void fuzzy_menu_sel_next(struct fuzzy_menu *fm);
void fuzzy_menu_sel_prev(struct fuzzy_menu *fm);
struct fuzzy_menu_dimensions fuzzy_menu_get_dimensions(
    struct fuzzy_menu *fm, struct ff_typography typo,
    Vector2 window_size);
const char32_t *fuzzy_menu_handle_interactions(struct fuzzy_menu *fm);
bool fuzzy_menu_buffer_changed(struct fuzzy_menu *fm);
void fuzzy_menu_on_buffer_change(struct fuzzy_menu *fm);
void fuzzy_menu_draw_options(struct fuzzy_menu *fm,
                             struct ff_typography typo,
                             struct fuzzy_menu_dimensions dimensions);
void fuzzy_menu_draw_editor(struct fuzzy_menu *fm,
                            struct ff_typography typo,
                            int focus_flags,
                            struct fuzzy_menu_dimensions dimensions);
