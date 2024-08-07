#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <raylib.h>

#include "fieldfusion.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"

typedef struct {
    float text_spacing;
    float padding;
    float gap;
    float search_box_width;
} layout_t;

typedef struct {
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
} color_scheme_t;

typedef struct {
    seq_group_id_t mode_normal;
    seq_group_id_t mode_selection;
    seq_group_id_t mode_search;
} editor_keybinds_t;

typedef struct {
    seq_group_id_t mode_normal;
    seq_group_id_t mode_selection;
} line_editor_keybinds_t;

typedef struct {
    editor_keybinds_t editor;
    line_editor_keybinds_t line_editor;
    seq_group_id_t pane;
    seq_group_id_t fuzzy_menu;
    seq_group_id_t main;
    seq_group_id_t file_picker;
    seq_group_id_t file_editor;
} keybind_groups_t;

typedef struct {
    layout_t layout;
    color_scheme_t color_scheme;
    keybind_groups_t keybinds;
    unsigned char scroll_off;
    ff_typo_t typo;
    float scr_proj[4][4];
} config_t;

extern config_t g_cfg;

void config_init(void);

#ifdef __cplusplus
}
#endif
