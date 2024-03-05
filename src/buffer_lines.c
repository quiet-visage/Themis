#include "buffer_lines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

size_t line_len(struct line* o) { return o->end - o->start; }

struct buffer_lines buffer_lines_create(void) {
    return (struct buffer_lines){
        .data = calloc(sizeof(struct line), 2),
        .length = 0,
        .capacity = sizeof(struct line) * 2};
}

void buffer_lines_destroy(struct buffer_lines* o) {
    free(o->data);
    memset(o, 0, sizeof(struct buffer_lines));
}

static void buffer_lines_push(struct buffer_lines* o,
                              struct line line) {
    size_t required_size = sizeof(struct line) * (o->length + 1);

    while (required_size > o->capacity) {
        o->capacity *= 2;
        o->data = realloc(o->data, o->capacity);
        assert(o->data);
    }

    o->data[o->length++] = line;
}

void buffer_lines_clear(struct buffer_lines* o) {
    o->length = 0;
}

void buffer_lines_update(struct buffer_lines* o, const char32_t* str,
                         const size_t str_len) {
    buffer_lines_clear(o);

    struct line line = {0};
    for (size_t i = 0; i < str_len; i += 1) {
        if (str[i] != U'\n') continue;
        line.end = i;
        buffer_lines_push(o, line);
        line.start = i + 1;
    }

    line.end = str_len;
    buffer_lines_push(o, line);
}
