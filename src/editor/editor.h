#pragma once
#include <field_fusion/fieldfusion.h>
#include <raylib.h>

#include "../text/text_view.h"
#include "history.h"
#include "line_editor.h"

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_cursor_moved = (1 << 3),
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
    struct editor_history_stack undo_stack;
    struct editor_history_stack redo_stack;
    struct text_position cursor;
    struct text_view text;
    int editor_flags;
    enum editor_mode editor_mode;
    struct text_position selection_begin;
    struct line_editor search_editor;
    struct editor_search_highlights search_highlights;
    size_t match_select_counter;
};

struct editor editor_create(void);
void editor_destroy(struct editor* e);
void editor_draw(struct editor* e, struct ff_typography typo,
                 Rectangle bounds, int focus_flags);
void editor_save_history(struct editor* this,
                         struct editor_history_stack* stack);
void editor_clear(struct editor* e);
