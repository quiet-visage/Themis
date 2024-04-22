#include "buffer_history.h"

#include <assert.h>
#include <stdlib.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

buffer_history_t buffer_history_create() {
    return (buffer_history_t){
        .data = calloc(2, sizeof(buffer_history_item_t)),
        .length = 0,
        .capacity = 2 * sizeof(buffer_history_item_t)};
}

void buffer_history_destroy(buffer_history_t* m) {
    while (m->length) buffer_history_pop(m);
    free(m->data);
}

void buffer_history_copy_and_push(buffer_history_t* m,
                                  utf32_str_t* str,
                                  text_pos_t cursor) {
    size_t required_capacity =
        (m->length + 1) * sizeof(buffer_history_item_t);

    while (required_capacity > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }

    m->data[m->length].str = utf32_str_clone(str);
    m->data[m->length].cursor = cursor;
    m->length += 1;
}

void buffer_history_pop(buffer_history_t* m) {
    assert(m->length > 0);
    m->length -= 1;
    utf32_str_destroy(&m->data[m->length].str);
}

void buffer_history_clear(buffer_history_t* m) {
    while (m->length > 0) buffer_history_pop(m);
}

buffer_history_item_t* buffer_history_top(buffer_history_t* m) {
    assert(m->length > 0);
    return &m->data[m->length - 1];
}
