#pragma once

#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <sys/types.h>
#include <text/text.h>

typedef enum {
    editor_flag_none = (0 << 1),
    editor_flag_ignore_enter_t = (1 << 1),
} editor_flags_t;

typedef struct {
    char32_t* buffer;
    ulong buffer_size;
    ulong cursor_pos;
} editor_history_t;

typedef struct {
    editor_history_t* data;
    ulong size;
    ulong capacity;
} editor_history_stack_t;

typedef struct {
    editor_history_stack_t undo_stack;
    editor_history_stack_t redo_stack;
    position_t cursor;
    bool cursor_moved;
    text_t text;
    int editor_flags;
} editor_t;

editor_t editor_create(void);
void editor_destroy(editor_t* e);
void editor_draw(editor_t* e, ff_typography_t typo, Rectangle bounds,
                 bool focused);
