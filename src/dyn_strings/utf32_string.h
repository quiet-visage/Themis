#pragma once
#include <uchar.h>

struct string32 {
    char32_t* data;
    size_t size;
    size_t capacity;
};

struct string32 string32_create(void);
void string32_destroy(struct string32* s);
struct string32 string32_clone(struct string32* str32);
void string32_copy_utf8(struct string32* s, const char* buffer,
                        size_t len);
void string32_copy(struct string32* s, char32_t* buffer, size_t len);
void string32_file(struct string32* s, const char* path);
void string32_delete(struct string32* s, size_t pos, size_t count);
void string32_clear(struct string32* this);
void string32_insert_buf(struct string32* s, size_t pos,
                         char32_t* str, size_t len);
void string32_insert_buf_utf8(struct string32* s, size_t pos,
                              const char* str, size_t len);
void string32_insert_char(struct string32* s, size_t pos,
                          char32_t chr);
