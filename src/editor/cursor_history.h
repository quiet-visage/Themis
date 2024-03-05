#pragma once
#include "../highlighter/highlighter.h"

struct cursor_history {
    struct text_position* data;
    size_t length;
    size_t capacity;
};

void cursor_history_create(struct cursor_history* o);
void cursor_history_destroy(struct cursor_history* o);
void cursor_history_push(struct cursor_history* o,
                         struct text_position cursor);
void cursor_history_pop(struct cursor_history* o);
struct text_position cursor_history_top(struct cursor_history* o);
