#pragma once
#include <stddef.h>
#include <uchar.h>

struct line {
    size_t start;
    size_t end;
};

struct buffer_lines {
    struct line* data;
    size_t length;
    size_t capacity;
};

size_t line_len(struct line*);
struct buffer_lines buffer_lines_create(void);
void buffer_lines_destroy(struct buffer_lines* o);
void buffer_lines_clear(struct buffer_lines* o);
void buffer_lines_update(struct buffer_lines* o, const char32_t* str,
                         const size_t str_len);
