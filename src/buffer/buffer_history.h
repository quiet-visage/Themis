#pragma once

#include <stddef.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

struct buffer_history_item {
    struct utf32_str str;
    struct text_position cursor;
};

struct buffer_history {
    struct buffer_history_item* data;
    size_t length;
    size_t capacity;
};

struct buffer_history buffer_history_create();
void buffer_history_destroy(struct buffer_history* m);
void buffer_history_copy_and_push(struct buffer_history* i,
                                  struct utf32_str* str,
                                  struct text_position cursor);
void buffer_history_pop(struct buffer_history* m);
void buffer_history_clear(struct buffer_history* m);
struct buffer_history_item* buffer_history_top(
    struct buffer_history* m);
