#include "cursor_history.h"

#include <assert.h>

void cursor_history_push(cursor_history_t* o, text_pos_t item) {
    size_t required_capacity = (o->length + 1) * sizeof(text_pos_t);

    while (required_capacity > o->capacity) {
        o->capacity *= 2;
        o->data = realloc(o->data, o->capacity);
        assert(o->data);
    }

    o->data[o->length] = item;
    o->length += 1;
}

void cursor_history_pop(cursor_history_t* o) {
    assert(o->length);
    o->length -= 1;
}

void cursor_history_create(cursor_history_t* o) {
    o->data = calloc(2, sizeof(text_pos_t));
    o->length = 0;
    o->capacity = 2 * sizeof(text_pos_t);
}

void cursor_history_destroy(cursor_history_t* o) { free(o->data); }

text_pos_t cursor_history_top(cursor_history_t* o) {
    assert(o->length);
    return o->data[o->length - 1];
}
