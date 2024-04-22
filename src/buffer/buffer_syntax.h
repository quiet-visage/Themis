#pragma once

#include <fieldfusion.h>

#include "../highlighter/highlighter.h"

typedef struct {
    highlighter_t highlighter;
    tokens_t tokens;
} buffer_syntax_t;

buffer_syntax_t buffer_syntax_create(void);
void buffer_syntax_destroy(buffer_syntax_t* m);
void buffer_syntax_set_language(buffer_syntax_t* m,
                                enum language language);
void buffer_syntax_update(buffer_syntax_t* m, c32_t* buffer,
                          size_t buffer_len);
