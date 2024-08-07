#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <fieldfusion.h>
#include <stddef.h>

typedef struct {
    size_t start;
    size_t end;
} line_t;

typedef struct {
    line_t* data;
    size_t length;
    size_t capacity;
} buffer_lines_t;

size_t line_len(line_t* m);
buffer_lines_t buffer_lines_create(void);
size_t buffer_lines_get_line_num_from_idx(buffer_lines_t* m,
                                          size_t idx);
void buffer_lines_destroy(buffer_lines_t* m);
void buffer_lines_clear(buffer_lines_t* m);
void buffer_lines_update(buffer_lines_t* m, const c32_t* str,
                         const size_t str_len);

#ifdef __cplusplus
}
#endif
