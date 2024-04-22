#include "buffer.h"

#include <assert.h>
#include <string.h>
#include <threads.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"
#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"

#define BUFFER_ON_MODIFIED(buf)                           \
    do {                                                  \
        buffer_lines_update(&buf->lines, buf->str.data,   \
                            buf->str.length);             \
        buffer_syntax_update(&buf->syntax, buf->str.data, \
                             buf->str.length);            \
    } while (0)

void buffer_create(buffer_t* m, utf32_str_t data) {
    m->str = data;
    m->undo_history = buffer_history_create();
    m->redo_history = buffer_history_create();
    m->lines = buffer_lines_create();
    m->syntax = buffer_syntax_create();
    buffer_lines_update(&m->lines, m->str.data, m->str.length);
}

void buffer_set_name(buffer_t* m, const char* name) {
    size_t buffer_name_len = strlen(name);

    if (buffer_name_len > BUFFER_NAME_CAP) {
        memcpy(m->buffer_name,
               &name[buffer_name_len - BUFFER_NAME_CAP],
               BUFFER_NAME_CAP);
        m->buffer_name[BUFFER_NAME_CAP - 1] = 0;
        return;
    }

    memcpy(m->buffer_name, name, buffer_name_len);
    m->buffer_name[buffer_name_len] = 0;
}

void buffer_save_undo(buffer_t* m, text_pos_t cursor) {
    buffer_history_copy_and_push(&m->undo_history, &m->str, cursor);
}

static void buffer_save_redo(buffer_t* m, text_pos_t cursor) {
    buffer_history_copy_and_push(&m->redo_history, &m->str, cursor);
}

text_pos_t buffer_undo(buffer_t* m, text_pos_t cursor) {
    assert(m->undo_history.length >= 1 &&
           "the original buffer must always be stored");

    bool is_original_buffer = m->undo_history.length == 1;
    buffer_history_item_t* undo_item =
        buffer_history_top(&m->undo_history);

    if (is_original_buffer) {
        utf32_str_copy(&m->str, undo_item->str.data,
                       undo_item->str.length);
        BUFFER_ON_MODIFIED(m);
        return undo_item->cursor;
    };

    buffer_save_redo(m, cursor);

    utf32_str_copy(&m->str, undo_item->str.data,
                   undo_item->str.length);
    buffer_history_pop(&m->undo_history);

    BUFFER_ON_MODIFIED(m);
    return undo_item->cursor;
}

text_pos_t buffer_redo(buffer_t* m, text_pos_t cursor) {
    // cursor position row is set to (size_t)-1 if there is nothing
    // in the redo buffer history
    text_pos_t result = {
        .row = (size_t)-1,
    };

    if (!m->redo_history.length) return result;

    buffer_history_item_t* redo_item =
        buffer_history_top(&m->redo_history);
    buffer_save_undo(m, cursor);
    result = redo_item->cursor;

    utf32_str_copy(&m->str, redo_item->str.data,
                   redo_item->str.length);
    buffer_history_pop(&m->redo_history);

    BUFFER_ON_MODIFIED(m);
    return result;
}

void buffer_destroy(buffer_t* m) {
    utf32_str_destroy(&m->str);
    buffer_history_destroy(&m->undo_history);
    buffer_history_destroy(&m->redo_history);
    buffer_lines_destroy(&m->lines);
    buffer_syntax_destroy(&m->syntax);
}

void buffer_clear(buffer_t* m) {
    m->str.length = 0;
    BUFFER_ON_MODIFIED(m);
}

void buffer_insert_char(buffer_t* m, const size_t pos,
                        const c32_t chr) {
    utf32_str_insert_char(&m->str, pos, chr);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_insert_utf8_buf(buffer_t* m, size_t pos, char* str,
                            size_t len) {
    utf32_str_insert_utf8_buf(&m->str, pos, str, len);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_insert_buf(buffer_t* m, size_t pos, c32_t* str,
                       size_t len) {
    utf32_str_insert_buf(&m->str, pos, str, len);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_delete(buffer_t* m, size_t pos, size_t count) {
    utf32_str_delete(&m->str, pos, count);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_copy(buffer_t* m, c32_t* buffer, size_t len) {
    utf32_str_copy(&m->str, buffer, len);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_read_file(buffer_t* m, const char* path) {
    utf32_str_read_file(&m->str, path);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_copy_utf8(buffer_t* m, const char* buffer, size_t len) {
    utf32_str_copy_utf8(&m->str, buffer, len);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_append_utf8(buffer_t* m, const char* buffer, size_t len) {
    utf32_str_append_utf8(&m->str, buffer, len);
    BUFFER_ON_MODIFIED(m);
    buffer_history_clear(&m->redo_history);
}
