#pragma once

#include <uchar.h>

struct utf32_str {
    char32_t* data;
    size_t length;
    size_t capacity;
};

struct utf32_str utf32_str_create(void);
void utf32_str_destroy(struct utf32_str* m);
struct utf32_str utf32_str_clone(struct utf32_str* str32);
void utf32_str_copy_utf8(struct utf32_str* m, const char* buffer,
                         size_t len);
void utf32_str_copy(struct utf32_str* m, char32_t* buffer,
                    size_t len);
void utf32_str_read_file(struct utf32_str* m, const char* path);
void utf32_str_delete(struct utf32_str* m, size_t pos, size_t count);
void utf32_str_clear(struct utf32_str* this);
void utf32_str_insert_buf(struct utf32_str* m, size_t pos,
                          char32_t* str, size_t len);
void utf32_str_insert_utf8_buf(struct utf32_str* m, size_t pos,
                               const char* str, size_t len);
void utf32_str_insert_char(struct utf32_str* m, size_t pos,
                           char32_t chr);
char32_t* str32str32(char32_t* pattern, size_t pattern_length,
                     char32_t* s2, size_t s2_length);
