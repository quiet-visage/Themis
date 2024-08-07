#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../highlighter/highlighter.h"
#include "../text_view.h"
#include "search_mod.h"

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_cursor_moved = (1 << 0),
    editor_flag_cursor_moved_manually = (1 << 1),
};

enum editor_mode {
    editor_mode_normal,
    editor_mode_selection,
    editor_mode_search,
};

typedef struct {
    text_pos_t cursor;
    text_view_t text;
    int editor_flags;
    enum editor_mode editor_mode;
    text_pos_t selection_begin;
    search_mod_t search_mod;
    size_t target_col;
} editor_t;

void editor_create(editor_t* m);
void editor_destroy(editor_t* m);
void editor_save_undo(editor_t* m);
void editor_draw(editor_t* m, ff_typo_t typo, Rectangle bounds,
                 int focus_flags);
void editor_reset_mode(editor_t* m);
void editor_move_cursor(editor_t* m, size_t row, size_t col);
void editor_clear(editor_t* m);

#ifdef __cplusplus
}
#endif
