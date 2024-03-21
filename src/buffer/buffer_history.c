#include "buffer_history.h"

#include <assert.h>
#include <stdlib.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

struct buffer_history buffer_history_create() {
    return (struct buffer_history){
        .data = calloc(2, sizeof(struct buffer_history_item)),
        .length = 0,
        .capacity = 2 * sizeof(struct buffer_history_item)};
}

void buffer_history_destroy(struct buffer_history* m) {
    while (m->length) buffer_history_pop(m);
    free(m->data);
}

void buffer_history_copy_and_push(struct buffer_history* m,
                                  struct utf32_str* str,
                                  struct text_position cursor) {
    size_t required_capacity =
        (m->length + 1) * sizeof(struct buffer_history_item);

    while (required_capacity > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }

    m->data[m->length].str = utf32_str_clone(str);
    m->data[m->length].cursor = cursor;
    m->length += 1;
}

void buffer_history_pop(struct buffer_history* m) {
    assert(m->length > 0);
    m->length -= 1;
    utf32_str_destroy(&m->data[m->length].str);
}

void buffer_history_clear(struct buffer_history* m) {
    while (m->length > 0) buffer_history_pop(m);
}

struct buffer_history_item* buffer_history_top(
    struct buffer_history* m) {
    assert(m->length > 0);
    return &m->data[m->length - 1];
}
