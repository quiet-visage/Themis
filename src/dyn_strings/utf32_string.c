#include "utf32_string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <fieldfusion.h>

struct utf32_str utf32_str_create(void) {
    struct utf32_str result = {.data = calloc(sizeof(char32_t), 2),
                               .length = 0,
                               .capacity = sizeof(char32_t) * 2};
    return result;
}

struct utf32_str utf32_str_clone(struct utf32_str* str32) {
    struct utf32_str result = {.data = malloc(str32->capacity),
                               .capacity = str32->capacity,
                               .length = str32->length};
    memcpy(result.data, str32->data, str32->capacity);
    return result;
}

void utf32_str_copy_utf8(struct utf32_str* s, const char* buffer,
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
        s->length = 0;
        if (s->capacity > previous_capacity) {
            s->capacity = previous_capacity;
            s->data = realloc(s->data, s->capacity);
        }
    } else {
        s->length = len;
    }
}

void utf32_str_read_file(struct utf32_str* s, const char* path) {
    FILE* file = fopen(path, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* file_contents = calloc(1, file_size + 1);
    fread(file_contents, 1, file_size, file);
    fclose(file);

    utf32_str_copy_utf8(s, file_contents, file_size);
    free(file_contents);
}

void utf32_str_destroy(struct utf32_str* s) {
    free(s->data);
    s->data = 0;
    s->length = 0;
    s->capacity = 0;
}

void utf32_str_copy(struct utf32_str* s, char32_t* buffer,
                    size_t len) {
    size_t required_capacity = sizeof(char32_t) * len;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    memcpy(s->data, buffer, len * sizeof(char32_t));
    s->length = len;
}

void utf32_str_delete(struct utf32_str* this, size_t pos,
                      size_t count) {
    if (!count || !this->length) return;
    assert(pos + count <= this->length);

    memmove(&this->data[pos], &this->data[pos + count],
            (this->length - (pos + count)) * sizeof(char32_t));
    this->length -= count;
}

void utf32_str_insert_buf(struct utf32_str* this, size_t pos,
                          char32_t* str, size_t len) {
    assert(pos <= this->length);
    size_t required_capacity =
        (this->length + len) * sizeof(char32_t);

    while (required_capacity > this->capacity) {
        this->capacity *= 2;
        this->data = realloc(this->data, this->capacity);
        assert(this->data);
    }

    memmove(&this->data[pos + len], &this->data[pos],
            (this->length - pos) * sizeof(char32_t));
    memcpy(&this->data[pos], str, len * sizeof(char32_t));
    this->length += len;
}

void utf32_str_clear(struct utf32_str* this) { this->length = 0; }

void utf32_str_insert_utf8_buf(struct utf32_str* s, size_t pos,
                               const char* str, const size_t len) {
    char32_t utf32_str[len];
    int failed = ff_utf8_to_utf32(utf32_str, str, len);
    assert(!failed);

    utf32_str_insert_buf(s, pos, utf32_str, len);
}

void utf32_str_insert_char(struct utf32_str* s, size_t pos,
                           char32_t chr) {
    utf32_str_insert_buf(s, pos, &chr, 1);
}

char32_t* str32str32(char32_t* pattern, size_t pattern_length,
                     char32_t* s2, size_t s2_length) {
    assert(pattern);
    assert(s2);
    assert(pattern_length);
    if (!s2_length) return 0;

    if (pattern_length > s2_length) return 0;

    for (size_t i = 0; i < s2_length; i += 1) {
        if (pattern[0] != s2[i]) continue;
        bool found = !memcmp(pattern, &s2[i],
                             pattern_length * sizeof(char32_t));
        if (found) return &s2[i];
    }

    return 0;
}
