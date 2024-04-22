#pragma once

#include "../text_view.h"

enum line_editor_flags {
    line_editor_flag_none = 0,
    line_editor_flag_cursor_moved = (1 << 0),
    line_editor_flag_cursor_moved_manually = (1 << 1),
};

enum line_editor_mode {
    line_editor_mode_normal,
    line_editor_mode_selection,
};

typedef struct {
    size_t selection_begin;
    text_pos_t cursor;
    text_view_t text;
    int line_editor_flags;
    enum line_editor_mode line_editor_mode;
} line_editor_t;

void line_editor_create(line_editor_t* m);
void line_editor_clear(line_editor_t* m);
void line_editor_destroy(line_editor_t* m);
void line_editor_draw(line_editor_t* m, ff_typo_t typo,
                      Rectangle bounds, int focus_flags);
