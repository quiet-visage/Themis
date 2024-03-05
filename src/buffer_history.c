#include "buffer_history.h"

#include <assert.h>
#include <stdlib.h>

#include "dyn_strings/utf32_string.h"

struct buffer_history buffer_history_create() {
    return (struct buffer_history){
        .data = calloc(2, sizeof(struct utf32_str)),
        .length = 0,
        .capacity = 2 * sizeof(struct utf32_str)};
}

void buffer_history_destroy(struct buffer_history* this) {
    while (this->length) buffer_history_pop(this);
    free(this->data);
}

void buffer_history_copy_and_push(struct buffer_history* this,
                                  struct utf32_str* data) {
    size_t required_capacity =
        (this->length + 1) * sizeof(struct utf32_str);

    while (required_capacity > this->capacity) {
        this->capacity *= 2;
        this->data = realloc(this->data, this->capacity);
        assert(this->data);
    }

    this->data[this->length] = utf32_str_clone(data);
    this->length += 1;
}

void buffer_history_pop(struct buffer_history* this) {
    assert(this->length > 0);
    this->length -= 1;
    utf32_str_destroy(&this->data[this->length]);
}

struct utf32_str* buffer_history_top(struct buffer_history* this) {
    assert(this->length > 0);
    return &this->data[this->length - 1];
}
