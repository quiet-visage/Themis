#include "buffer_lines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

size_t line_len(struct line* m) { return m->end - m->start; }

struct buffer_lines buffer_lines_create(void) {
    return (struct buffer_lines){
        .data = calloc(sizeof(struct line), 2),
        .length = 0,
        .capacity = sizeof(struct line) * 2};
}

void buffer_lines_destroy(struct buffer_lines* m) {
    free(m->data);
    memset(m, 0, sizeof(struct buffer_lines));
}

static void buffer_lines_push(struct buffer_lines* m,
                              struct line line) {
    size_t required_size = sizeof(struct line) * (m->length + 1);

    while (required_size > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }

    m->data[m->length++] = line;
}

void buffer_lines_clear(struct buffer_lines* m) { m->length = 0; }

void buffer_lines_update(struct buffer_lines* m, const char32_t* str,
                         const size_t str_len) {
    buffer_lines_clear(m);

    struct line line = {0};
    for (size_t i = 0; i < str_len; i += 1) {
        if (str[i] != U'\n') continue;
        line.end = i;
        buffer_lines_push(m, line);
        line.start = i + 1;
    }

    line.end = str_len;
    buffer_lines_push(m, line);
}

size_t buffer_lines_get_line_num_from_idx(struct buffer_lines* m,
                                          size_t idx) {
    for (size_t i = 0; i < m->length; i += 1) {
        if (idx >= m->data[i].start && idx <= m->data[i].end)
            return i;
    }

    assert(0 &&
           "didn't find line, probably a bug outside this function");
}
