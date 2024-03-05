#pragma once
#include <fieldfusion.h>
#include <raylib.h>

#include "../text/text_view.h"
#include "cursor_history.h"
#include "line_editor.h"

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_cursor_moved = (1 << 1),
    editor_flag_cursor_moved_manually = (1 << 2),
};

enum editor_mode {
    editor_mode_normal,
    editor_mode_selection,
    editor_mode_search,
    editor_mode_search_input
};

struct editor_search_highlights {
    struct selection* highlights;
    size_t length;
    size_t capactiy;
};

struct editor {
    struct cursor_history cursor_undo;
    struct cursor_history cursor_redo;
    struct text_position cursor;
    struct text_view text;
    int editor_flags;
    enum editor_mode editor_mode;
    struct text_position selection_begin;
    struct line_editor search_editor;
    struct editor_search_highlights search_highlights;
    size_t match_select_counter;
};

void editor_create(struct editor* this);
void editor_destroy(struct editor* e);
void editor_draw(struct editor* e, struct ff_typography typo,
                 Rectangle bounds, int focus_flags);
void editor_clear(struct editor* e);
