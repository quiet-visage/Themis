#include "buffer_lines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

size_t line_len(line_t* m) { return m->end - m->start; }

buffer_lines_t buffer_lines_create(void) {
    return (buffer_lines_t){.data = calloc(sizeof(line_t), 2),
                            .length = 0,
                            .capacity = sizeof(line_t) * 2};
}

void buffer_lines_destroy(buffer_lines_t* m) {
    free(m->data);
    memset(m, 0, sizeof(buffer_lines_t));
}

static void buffer_lines_push(buffer_lines_t* m, line_t line) {
    size_t required_size = sizeof(line_t) * (m->length + 1);

    while (required_size > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }

    m->data[m->length++] = line;
}

void buffer_lines_clear(buffer_lines_t* m) { m->length = 0; }

void buffer_lines_update(buffer_lines_t* m, const c32_t* str,
                         const size_t str_len) {
    buffer_lines_clear(m);

    line_t line = {0};
    for (size_t i = 0; i < str_len; i += 1) {
        if (str[i] != U'\n') continue;
        line.end = i;
        buffer_lines_push(m, line);
        line.start = i + 1;
    }

    line.end = str_len;
    buffer_lines_push(m, line);
}

size_t buffer_lines_get_line_num_from_idx(buffer_lines_t* m,
                                          size_t idx) {
    for (size_t i = 0; i < m->length; i += 1) {
        if (idx >= m->data[i].start && idx <= m->data[i].end)
            return i;
    }

    assert(0 &&
           "didn't find line, probably a bug outside this function");
}
