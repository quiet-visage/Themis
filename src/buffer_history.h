#pragma once

#include <stddef.h>

#include "dyn_strings/utf32_string.h"

struct buffer_history {
    struct utf32_str* data;
    size_t length;
    size_t capacity;
};

struct buffer_history buffer_history_create();
void buffer_history_destroy(struct buffer_history* this);
void buffer_history_copy_and_push(struct buffer_history* this,
                                  struct utf32_str* data);
void buffer_history_pop(struct buffer_history* this);
struct utf32_str* buffer_history_top(struct buffer_history* this);
