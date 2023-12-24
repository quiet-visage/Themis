#pragma once
#include <uchar.h>

typedef struct {
    char32_t* data;
    size_t size;
    size_t capacity;
} string32_t;

string32_t string32_create();
void string32_destroy(string32_t* s);
void string32_copy_utf8(string32_t* s, const char* buffer,
                        size_t len);
void string32_copy(string32_t* s, char32_t* buffer, size_t len);
void string32_delete(string32_t* s, size_t pos, size_t count);
void string32_insert_buf(string32_t* s, size_t pos, char32_t* str,
                         size_t len);
void string32_insert_char(string32_t* s, size_t pos, char32_t chr);
