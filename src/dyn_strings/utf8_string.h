#pragma once

#include <stddef.h>

struct utf8_str {
    char* data;
    size_t capacity;
    size_t length;
};

struct utf8_str utf8_str_create(void);
void utf8_str_destroy(struct utf8_str* this);
void utf8_str_copy(struct utf8_str* this, const char* buf,
                   size_t buf_len);
struct utf8_str utf8_str_clone(struct utf8_str* str);
void utf8_str_clear(struct utf8_str* this);
