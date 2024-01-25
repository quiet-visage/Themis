#include "history.h"

#define STACK_PREALLOC_LENGTH 4

struct editor_history editor_history_create(
    struct utf32_str* buffer, struct text_position cursor) {
    return (struct editor_history){
        .text_buffer = utf32_str_clone(buffer), .cursor = cursor};
}

void editor_history_destroy(struct editor_history* this) {
    utf32_str_destroy(&this->text_buffer);
}

struct editor_history_stack editor_history_stack_create(void) {
    return (struct editor_history_stack){
        .data = calloc(sizeof(struct editor_history),
                       STACK_PREALLOC_LENGTH),
        .size = 0,
        .capacity =
            sizeof(struct editor_history) * STACK_PREALLOC_LENGTH};
}

void editor_history_stack_destroy(struct editor_history_stack* this) {
    for (size_t i = 0; i < this->size; i += 1) {
        editor_history_destroy(&this->data[i]);
    }
    free(this->data);
    this->data = NULL;
    this->capacity = 0;
}

void editor_history_stack_push(struct editor_history_stack* this,
                               struct editor_history history) {
    size_t required_cap =
        (this->size + 1) * sizeof(struct editor_history);
    while (required_cap > this->capacity) {
        this->capacity *= 2;
        this->data = realloc(this->data, this->capacity);
        assert(this->data);
    }

    this->data[this->size++] = history;
}

void editor_history_stack_pop(struct editor_history_stack* this) {
    assert(this->size > 0);
    editor_history_destroy(&this->data[this->size-1]);
    this->size -= 1;
}

struct editor_history* editor_history_stack_top(
    struct editor_history_stack* this) {
    assert(this->size > 0);
    return &this->data[this->size - 1];
}
