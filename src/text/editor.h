#pragma once

#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <sys/types.h>
#include <text/text.h>

enum editor_flags {
    editor_flag_none = (0 << 1),
    editor_flag_ignore_enter_t = (1 << 1),
};

struct editor_history {
    char32_t* buffer;
    ulong buffer_size;
    ulong cursor_pos;
};

struct editor_history_stack {
    struct editor_history* data;
    ulong size;
    ulong capacity;
};

struct editor {
    struct editor_history_stack undo_stack;
    struct editor_history_stack redo_stack;
    struct text_position cursor;
    bool cursor_moved;
    struct text text;
    int editor_flags;
};

struct editor editor_create(void);
void editor_destroy(struct editor* e);
void editor_draw(struct editor* e, struct ff_typography typo,
                 Rectangle bounds, bool focused);
