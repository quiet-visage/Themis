#include "utf8_string.h"

#include <stdlib.h>
#include <string.h>

utf8_str_t utf8_str_create(void) {
    return (utf8_str_t){
        .data = calloc(1, 2), .capacity = 2, .length = 0};
}

void utf8_str_destroy(utf8_str_t* this) { free(this->data); }

void utf8_str_copy(utf8_str_t* this, const char* buf,
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

void utf8_str_clear(utf8_str_t* this) { this->length = 0; }
