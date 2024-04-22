#pragma once
#include "../highlighter/highlighter.h"

typedef struct {
    text_pos_t* data;
    size_t length;
    size_t capacity;
} cursor_history_t;

void cursor_history_create(cursor_history_t* o);
void cursor_history_destroy(cursor_history_t* o);
void cursor_history_push(cursor_history_t* o, text_pos_t cursor);
void cursor_history_pop(cursor_history_t* o);
text_pos_t cursor_history_top(cursor_history_t* o);
