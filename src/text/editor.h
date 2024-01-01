#pragma once

#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <sys/types.h>
#include <text/text.h>

#include "unicode_string.h"

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_ignore_enter_t = (1 << 1),
    editor_flag_single_line_mode_t = (1 << 2),
};

struct editor_history {
    struct string32 text_buffer;
    struct text_position cursor;
};

struct editor_history_stack {
    struct editor_history* data;
    ulong size;
    ulong capacity;
};

enum editor_mode {
    editor_mode_none,
    editor_mode_selection,
};

struct editor {
    struct editor_history_stack undo_stack;
    struct editor_history_stack redo_stack;
    struct text_position cursor;
    bool cursor_moved;
    struct text text;
    int editor_flags;
    enum editor_mode editor_mode;
    struct text_position selection_begin;
};

struct editor editor_create(void);
void editor_destroy(struct editor* e);
void editor_draw(struct editor* e, struct ff_typography typo,
                 Rectangle bounds, int focus_flags);
void editor_save_undo(struct editor* e);
void editor_clear(struct editor* e);
