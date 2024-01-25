#pragma once

#include "../editor/history.h"
#include "../text/text_view.h"

enum line_editor_flags {
    line_editor_flag_none = 0,
    line_editor_flag_cursor_moved = (1 << 0)
};

enum line_editor_mode {
    line_editor_mode_normal,
    line_editor_mode_selection,
};

struct line_editor {
    struct editor_history_stack undo_stack;
    struct editor_history_stack redo_stack;
    struct text_position selection_begin;
    struct text_position cursor;
    struct text_view text;
    int line_editor_flags;
    enum line_editor_mode line_editor_mode;
};

struct line_editor line_editor_create(void);
void line_editor_clear(struct line_editor* e);
void line_editor_destroy(struct line_editor* e);
void line_editor_draw(struct line_editor* e,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags);
