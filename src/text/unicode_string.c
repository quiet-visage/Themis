#include "unicode_string.h"

#include <assert.h>
#include <field_fusion/fieldfusion.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

struct string32 string32_create(void) {
    struct string32 result = {.data = calloc(sizeof(char32_t), 2),
                              .size = 0,
                              .capacity = sizeof(char32_t) * 2};
    return result;
}

struct string32 string32_clone(struct string32* str32) {
    struct string32 result = {
        .data = malloc(str32->capacity),
        .capacity = str32->capacity,
        .size = str32->size};
    memcpy(result.data, str32->data, str32->capacity);
    return result;
}

void string32_copy_utf8(struct string32* s, const char* buffer,
                        size_t len) {
    size_t required_capacity = sizeof(char32_t) * len;
    size_t previous_capacity = s->capacity;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    int failed = ff_utf8_to_utf32(s->data, buffer, len);

    if (failed) {
        s->size = 0;
        if (s->capacity > previous_capacity) {
            s->capacity = previous_capacity;
            s->data = realloc(s->data, s->capacity);
        }
    } else {
        s->size = len;
    }
}

void string32_copy_file(struct string32* s, const char* path) {
    FILE* file = fopen(path, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char file_contents[file_size + 1];
    file_contents[file_size] = 0;
    fread(file_contents, 1, file_size, file);
    fclose(file);

    string32_copy_utf8(s, file_contents, file_size);
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

    memcpy(s->data, buffer, len * sizeof(char32_t));
    s->size = len;
    s->data[s->size] = 0;
}

void string32_delete(struct string32* s, size_t pos, size_t count) {
    memmove(&s->data[pos], &s->data[pos + count],
            (s->size - pos) * sizeof(char32_t));
    s->size -= count;
    s->data[s->size] = 0;
}

void string32_insert_buf(struct string32* s, size_t pos,
                         char32_t* str, size_t len) {
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

void string32_insert_char(struct string32* s, size_t pos,
                          char32_t chr) {
    string32_insert_buf(s, pos, &chr, 1);
}
