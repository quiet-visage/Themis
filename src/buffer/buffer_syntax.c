#include "buffer_syntax.h"

#include <fieldfusion.h>

#include "../highlighter/highlighter.h"

buffer_syntax_t buffer_syntax_create(void) {
    return (buffer_syntax_t){
        .tokens = hlr_tokens_create(),
        .highlighter = hlr_highlighter_create(language_none_t, 0, 0)};
}

void buffer_syntax_destroy(buffer_syntax_t* m) {
    hlr_tokens_destroy(&m->tokens);
    hlr_highlighter_destroy(&m->highlighter);
}

void buffer_syntax_set_language(buffer_syntax_t* m,
                                enum language language) {
    m->highlighter.language = language;
}

void buffer_syntax_update(buffer_syntax_t* m, c32_t* buffer,
                          size_t buffer_len) {
    assert(buffer);
    if (m->highlighter.language == language_none_t) return;

    char utf8_buffer[buffer_len + 1];
    utf8_buffer[buffer_len] = 0;

    int conv_failed =
        ff_utf32_to_utf8(utf8_buffer, buffer, buffer_len);
    assert(conv_failed != (size_t)-1);

    hlr_highlighter_update(&m->highlighter, utf8_buffer, buffer_len);
    hlr_tokens_update(&m->highlighter, &m->tokens);
}
