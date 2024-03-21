#pragma once

#include "../highlighter/highlighter.h"
#include "../text_view.h"
#include "search_mod.h"

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_cursor_moved = (1 << 0),
    editor_flag_cursor_moved_manually = (1 << 1),
};

struct editor_search_highlights {
    struct selection* highlights;
    size_t length;
    size_t capactiy;
};

enum editor_mode {
    editor_mode_normal,
    editor_mode_selection,
    editor_mode_search,
};

struct editor {
    struct text_position cursor;
    struct text_view text;
    int editor_flags;
    enum editor_mode editor_mode;
    struct text_position selection_begin;
    struct search_mod search_mod;
};

void editor_create(struct editor* m);
void editor_destroy(struct editor* m);
void editor_save_undo(struct editor* m);
void editor_draw(struct editor* m, struct ff_typography typo,
                 Rectangle bounds, int focus_flags);
void editor_reset_mode(struct editor* m);
void editor_clear(struct editor* m);
