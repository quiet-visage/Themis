#pragma once

#include <uchar.h>

#include "highlighter/highlighter.h"

struct buffer_syntax {
    struct highlighter highlighter;
    struct tokens tokens;
};

struct buffer_syntax buffer_syntax_create(void);
void buffer_syntax_destroy(struct buffer_syntax* o);
void buffer_syntax_set_language(struct buffer_syntax* o,
                                enum language language);
void buffer_syntax_update(struct buffer_syntax* o, char32_t* buffer,
                          size_t buffer_len);
