#pragma once

#include <stddef.h>

typedef int c32_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    c32_t* data;
    size_t length;
    size_t capacity;
} utf32_str_t;

utf32_str_t utf32_str_create(void);
void utf32_str_destroy(utf32_str_t* m);
utf32_str_t utf32_str_clone(utf32_str_t* str32);
void utf32_str_copy_utf8(utf32_str_t* m, const char* buffer,
                         size_t len);
void utf32_str_append_utf8(utf32_str_t* m, const char* buffer,
                           size_t len);
void utf32_str_copy(utf32_str_t* m, c32_t* buffer, size_t len);
void utf32_str_read_file(utf32_str_t* m, const char* path);
void utf32_str_delete(utf32_str_t* m, size_t pos, size_t count);
void utf32_str_clear(utf32_str_t* m);
void utf32_str_insert_buf(utf32_str_t* m, size_t pos, c32_t* str,
                          size_t len);
void utf32_str_insert_utf8_buf(utf32_str_t* m, size_t pos,
                               const char* str, size_t len);
void utf32_str_insert_char(utf32_str_t* m, size_t pos, c32_t chr);
c32_t* str32str32(const c32_t* substr, size_t substr_len,
                  const c32_t* str, size_t str_len);

#ifdef __cplusplus
}
#endif
