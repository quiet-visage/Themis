#pragma once

#include <stddef.h>

typedef struct {
    char* data;
    size_t capacity;
    size_t length;
} utf8_str_t;

utf8_str_t utf8_str_create(void);
void utf8_str_destroy(utf8_str_t* this);
void utf8_str_copy(utf8_str_t* this, const char* buf, size_t buf_len);
utf8_str_t utf8_str_clone(utf8_str_t* str);
void utf8_str_clear(utf8_str_t* this);
