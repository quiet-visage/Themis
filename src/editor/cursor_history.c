#include "cursor_history.h"

#include <assert.h>

void cursor_history_push(struct cursor_history* o,
                         struct text_position item) {
    size_t required_capacity =
        (o->length + 1) * sizeof(struct text_position);

    while (required_capacity > o->capacity) {
        o->capacity *= 2;
        o->data = realloc(o->data, o->capacity);
        assert(o->data);
    }

    o->data[o->length] = item;
    o->length += 1;
}

void cursor_history_pop(struct cursor_history* o) {
    assert(o->length);
    o->length -= 1;
}

void cursor_history_create(struct cursor_history* o) {
    o->data = calloc(2, sizeof(struct text_position));
    o->length = 0;
    o->capacity = 2 * sizeof(struct text_position);
}

void cursor_history_destroy(struct cursor_history* o) {
    free(o->data);
}

struct text_position cursor_history_top(struct cursor_history* o) {
    assert(o->length);
    return o->data[o->length - 1];
}
