#pragma once
#include <raylib.h>

#include "fieldfusion.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"

struct layout {
    float text_spacing;
    float padding;
    float gap;
    float search_box_width;
};

struct color_scheme {
    int bg;
    int fg;
    int text_mute;
    int text_sel_bg;
    int selected_fg;
    int selected_bg;
    int surface0_bg;
    int surface1_bg;
    int highlight_fg;
    int syntax[token_kind_count_t];
};

struct editor_keybinds {
    seq_group_id_t mode_normal;
    seq_group_id_t mode_selection;
    seq_group_id_t mode_search;
};

struct line_editor_keybinds {
    seq_group_id_t mode_normal;
    seq_group_id_t mode_selection;
};

struct keybind_groups {
    struct editor_keybinds editor;
    struct line_editor_keybinds line_editor;
    seq_group_id_t pane;
    seq_group_id_t fuzzy_menu;
    seq_group_id_t main;
    seq_group_id_t file_picker;
    seq_group_id_t file_editor;
};

struct config {
    struct layout layout;
    struct color_scheme color_scheme;
    struct keybind_groups keybinds;
    unsigned char scroll_off;
    struct ff_typography typo;
};

extern struct config g_cfg;

void config_init(void);
