#pragma once

#include <resources/resources.h>
#include <text/editor.h>
#include <text/unicode_string.h>
#include <uchar.h>

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
    struct editor editor;
    float vertical_scroll;
    struct ff_glyphs_vector glyphs;
    size_t previous_buffer_size;
    size_t selected;
    struct fuzzy_menu_option options[FUZZY_MENU_OPTIONS_CAP];
    size_t options_count;
    struct motion motion;
};

struct fuzzy_menu fuzzy_menu_create();
void fuzzy_menu_destroy(struct fuzzy_menu *fm);
void fuzzy_menu_push_option(struct fuzzy_menu *fm,
                            const char32_t *option, size_t len);
void fuzzy_menu_push_option_with_icon(struct fuzzy_menu *fm,
                                      const char32_t *option,
                                      size_t len, enum icon icon);
const char32_t *fuzzy_menu_perform(struct fuzzy_menu *fm,
                                   struct ff_typography typo,
                                   bool focused);
void fuzzy_menu_reset(struct fuzzy_menu *fm);
void fuzzy_menu_sel_next(struct fuzzy_menu *fm);
void fuzzy_menu_sel_prev(struct fuzzy_menu *fm);
