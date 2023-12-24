#pragma once
#include <text/editor.h>
#include <text/unicode_string.h>
#include <uchar.h>

#define FUZZY_MENU_OPTION_NAME_CAP 256
#define FUZZY_MENU_OPTIONS_CAP 256

typedef struct {
    int menu_bg;
    int editor_bg;
    int editor_fg;
    int option_fg;
    int option_sel_fg;
    int option_sel_bg;
} fuzzy_menu_colors_t;

typedef struct {
    char32_t name[FUZZY_MENU_OPTION_NAME_CAP];
    uint edit_distance;
    size_t name_len;
} fuzzy_menu_option_t;

typedef struct {
    editor_t editor;
    float vertical_scroll;
    ff_glyphs_vector_t glyphs;
    size_t previous_buffer_size;
    size_t selected;
    fuzzy_menu_option_t options[FUZZY_MENU_OPTIONS_CAP];
    size_t options_count;
    motion_t motion;
} fuzzy_menu_t;

fuzzy_menu_t fuzzy_menu_create();
void fuzzy_menu_destroy(fuzzy_menu_t *fm);
void fuzzy_menu_push_option(fuzzy_menu_t *fm, const char32_t *option,
                            size_t len);
const char32_t *fuzzy_menu_perform(fuzzy_menu_t *fm,
                                   ff_typography_t typo,
                                   bool focused);
void fuzzy_menu_reset(fuzzy_menu_t *fm);
void fuzzy_menu_sel_next(fuzzy_menu_t *fm);
void fuzzy_menu_sel_prev(fuzzy_menu_t *fm);
