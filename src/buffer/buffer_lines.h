#pragma once
#include <fieldfusion.h>
#include <stddef.h>

struct line {
    size_t start;
    size_t end;
};

struct buffer_lines {
    struct line* data;
    size_t length;
    size_t capacity;
};

size_t line_len(struct line* m);
struct buffer_lines buffer_lines_create(void);
size_t buffer_lines_get_line_num_from_idx(struct buffer_lines* m,
                                          size_t idx);
void buffer_lines_destroy(struct buffer_lines* m);
void buffer_lines_clear(struct buffer_lines* m);
void buffer_lines_update(struct buffer_lines* m, const c32_t* str,
                         const size_t str_len);
