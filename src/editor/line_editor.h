#pragma once

#include "../text_view.h"
#include "cursor_history.h"

enum line_editor_flags {
    line_editor_flag_none = 0,
    line_editor_flag_cursor_moved = (1 << 0),
    line_editor_flag_cursor_moved_manually = (1 << 1),
};

enum line_editor_mode {
    line_editor_mode_normal,
    line_editor_mode_selection,
};

struct line_editor {
    size_t selection_begin;
    struct text_position cursor;
    struct text_view text;
    int line_editor_flags;
    enum line_editor_mode line_editor_mode;
};

void line_editor_create(struct line_editor* m);
void line_editor_clear(struct line_editor* m);
void line_editor_destroy(struct line_editor* m);
void line_editor_draw(struct line_editor* m,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags);
