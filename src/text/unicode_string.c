#include "unicode_string.h"

#include <assert.h>
#include <field_fusion/fieldfusion.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

struct string32 string32_create() {
    struct string32 result = {.data = calloc(sizeof(char32_t), 2),
                         .size = 0,
                         .capacity = sizeof(char32_t) * 2};
    return result;
}

void string32_copy_utf8(struct string32* s, const char* buffer,
                        size_t len) {
    size_t required_capacity = sizeof(char32_t) * len;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    ff_utf8_to_utf32(s->data, buffer, len);
    s->size = len;
}

void string32_destroy(struct string32* s) {
    free(s->data);
    s->data = 0;
    s->size = 0;
    s->capacity = 0;
}

void string32_copy(struct string32* s, char32_t* buffer, size_t len) {
    size_t required_capacity = sizeof(char32_t) * len;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    memcpy(s->data, buffer, len);
    s->size = len;
    s->data[s->size] = 0;
}

void string32_delete(struct string32* s, size_t pos, size_t count) {
    memmove(&s->data[pos], &s->data[pos + count],
            (s->size - pos) * sizeof(char32_t));
    s->size -= count;
    s->data[s->size] = 0;
}

void string32_insert_buf(struct string32* s, size_t pos, char32_t* str,
                         size_t len) {
    assert(pos <= s->size);
    size_t required_capacity = (s->size + len) * sizeof(char32_t);

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    memmove(&s->data[pos + len], &s->data[pos],
            (s->size - pos) * sizeof(char32_t));
    memcpy(&s->data[pos], str, len * sizeof(char32_t));

    s->size += len;
}

void string32_insert_char(struct string32* s, size_t pos, char32_t chr) {
    string32_insert_buf(s, pos, &chr, 1);
}
