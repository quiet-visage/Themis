#pragma once
#include "text/text_view.h"

#define BUFFER_NAME_CAP 0x100

struct buffer_history {
    struct utf32_str* data;
    size_t length;
    size_t capacity;
};

struct buffer {
    char buffer_name[BUFFER_NAME_CAP];
    struct utf32_str string;
    struct text_view preview;
    struct buffer_history undo_history;
    struct buffer_history redo_history;
};

void buffer_save_undo(struct buffer* o);
void buffer_save_redo(struct buffer* o);
void buffer_undo(struct buffer* o);
void buffer_redo(struct buffer* o);
void buffer_destroy(struct buffer* o);
