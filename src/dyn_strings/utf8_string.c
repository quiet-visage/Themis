#include "utf8_string.h"

#include <stdlib.h>
#include <string.h>

struct utf8_str utf8_str_create(void) {
    return (struct utf8_str){
        .data = calloc(1, 2), .capacity = 2, .length = 0};
}

void utf8_str_destroy(struct utf8_str* this) { free(this->data); }

void utf8_str_copy(struct utf8_str* this, const char* buf,
                   size_t buf_len) {
    size_t required_cap = buf_len + 1;

    while (required_cap > this->capacity) {
        this->capacity *= 2;
        this->data = realloc(this->data, this->capacity);
    }

    this->data[buf_len] = 0;
    this->length = buf_len;
    memcpy(this->data, buf, buf_len);
}

void utf8_str_clear(struct utf8_str* this) { this->length = 0; }
