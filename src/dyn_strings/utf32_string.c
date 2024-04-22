#include "utf32_string.h"

#include <assert.h>
#include <fieldfusion.h>
#include <magic.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

utf32_str_t utf32_str_create(void) {
    utf32_str_t result = {.data = calloc(sizeof(c32_t), 2),
                          .length = 0,
                          .capacity = sizeof(c32_t) * 2};
    return result;
}

utf32_str_t utf32_str_clone(utf32_str_t* str32) {
    utf32_str_t result = {.data = malloc(str32->capacity),
                          .capacity = str32->capacity,
                          .length = str32->length};
    memcpy(result.data, str32->data, str32->capacity);
    return result;
}

void utf32_str_copy_utf8(utf32_str_t* s, const char* buffer,
                         size_t len) {
    size_t required_capacity = sizeof(c32_t) * len;
    size_t previous_capacity = s->capacity;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    size_t new_len = ff_utf8_to_utf32(s->data, buffer, len);

    if (new_len == (size_t)-1) {
        s->length = 0;
        if (s->capacity > previous_capacity) {
            s->capacity = previous_capacity;
            s->data = realloc(s->data, s->capacity);
        }
    } else {
        s->length = new_len;
    }
}

void utf32_str_append_utf8(utf32_str_t* m, const char* buffer,
                           size_t len) {
    size_t required_capacity =
        m->length * sizeof(c32_t) + len * sizeof(c32_t);
    size_t previous_capacity = m->capacity;

    while (required_capacity > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }

    size_t new_len =
        ff_utf8_to_utf32(&m->data[m->length], buffer, len);
    bool failed = new_len == (size_t)-1;

    if (failed) {
        if (m->capacity > previous_capacity) {
            m->capacity = previous_capacity;
            m->data = realloc(m->data, m->capacity);
        }
    } else {
        m->length += new_len;
    }
}

void utf32_str_read_file(utf32_str_t* s, const char* path) {
    magic_t m = magic_open(MAGIC_MIME);
    if (!m) printf("init fail\n");
    int d = magic_load(m, 0);
    if (d) printf("load fail \n");
    const char* mf = magic_file(m, path);
    size_t mf_len = strlen(mf);
    printf("%s\n", mf);
    if ((strncmp(mf, "text", fminl(4, mf_len)))) {
        printf("skipping non text detected\n");
        magic_close(m);
        return;
    }
    magic_close(m);

    FILE* file = fopen(path, "r");
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

void utf32_str_destroy(utf32_str_t* s) {
    free(s->data);
    s->data = 0;
    s->length = 0;
    s->capacity = 0;
}

void utf32_str_copy(utf32_str_t* s, c32_t* buffer, size_t len) {
    size_t required_capacity = sizeof(c32_t) * len;

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    memcpy(s->data, buffer, len * sizeof(c32_t));
    s->length = len;
}

void utf32_str_delete(utf32_str_t* this, size_t pos, size_t count) {
    if (!count || !this->length) return;
    assert(pos + count <= this->length);

    memmove(&this->data[pos], &this->data[pos + count],
            (this->length - (pos + count)) * sizeof(c32_t));
    this->length -= count;
}

void utf32_str_insert_buf(utf32_str_t* this, size_t pos, c32_t* str,
                          size_t len) {
    assert(pos <= this->length);
    size_t required_capacity = (this->length + len) * sizeof(c32_t);

    while (required_capacity > this->capacity) {
        this->capacity *= 2;
        this->data = realloc(this->data, this->capacity);
        assert(this->data);
    }

    memmove(&this->data[pos + len], &this->data[pos],
            (this->length - pos) * sizeof(c32_t));
    memcpy(&this->data[pos], str, len * sizeof(c32_t));
    this->length += len;
}

void utf32_str_clear(utf32_str_t* this) { this->length = 0; }

void utf32_str_insert_utf8_buf(utf32_str_t* s, size_t pos,
                               const char* str, const size_t len) {
    c32_t utf32_str[len];
    int failed = ff_utf8_to_utf32(utf32_str, str, len);
    assert(!failed);

    utf32_str_insert_buf(s, pos, utf32_str, len);
}

void utf32_str_insert_char(utf32_str_t* s, size_t pos, c32_t chr) {
    utf32_str_insert_buf(s, pos, &chr, 1);
}

c32_t* str32str32(const c32_t* substr, size_t substr_len,
                  const c32_t* str, size_t str_len) {
    assert(substr);
    assert(str);
    assert(substr_len);
    if (!str_len) return 0;

    if (substr_len > str_len) return 0;

    for (size_t i = 0; i < str_len; i += 1) {
        if (substr[0] != str[i]) continue;
        bool found =
            !memcmp(substr, &str[i], substr_len * sizeof(c32_t));
        if (found) return &str[i];
    }

    return 0;
}
