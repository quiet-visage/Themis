#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

typedef struct {
    utf32_str_t str;
    text_pos_t cursor;
} buffer_history_item_t;

typedef struct {
    buffer_history_item_t* data;
    size_t length;
    size_t capacity;
} buffer_history_t;

buffer_history_t buffer_history_create();
void buffer_history_destroy(buffer_history_t* m);
void buffer_history_copy_and_push(buffer_history_t* i,
                                  utf32_str_t* str,
                                  text_pos_t cursor);
void buffer_history_pop(buffer_history_t* m);
void buffer_history_clear(buffer_history_t* m);
buffer_history_item_t* buffer_history_top(buffer_history_t* m);

#ifdef __cplusplus
}
#endif
