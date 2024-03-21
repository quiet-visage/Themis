#include "buffer.h"

#include <assert.h>
#include <string.h>

#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"
#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

void buffer_create(struct buffer* m, struct utf32_str data) {
    m->str = data;
    m->undo_history = buffer_history_create();
    m->redo_history = buffer_history_create();
    m->lines = buffer_lines_create();
    m->syntax = buffer_syntax_create();
    buffer_lines_update(&m->lines, m->str.data, m->str.length);
}

void buffer_set_name(struct buffer* m, const char* name) {
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

void buffer_save_undo(struct buffer* m, struct text_position cursor) {
    buffer_history_copy_and_push(&m->undo_history, &m->str, cursor);
}

static void buffer_save_redo(struct buffer* m,
                             struct text_position cursor) {
    buffer_history_copy_and_push(&m->redo_history, &m->str, cursor);
}

static void buffer_on_modified(struct buffer* m) {
    buffer_lines_update(&m->lines, m->str.data, m->str.length);
    buffer_syntax_update(&m->syntax, m->str.data, m->str.length);
}

struct text_position buffer_undo(struct buffer* m,
                                 struct text_position cursor) {
    assert(m->undo_history.length >= 1 &&
           "the original buffer must always be stored");

    bool is_original_buffer = m->undo_history.length == 1;
    struct buffer_history_item* undo_item =
        buffer_history_top(&m->undo_history);

    if (is_original_buffer) {
        utf32_str_copy(&m->str, undo_item->str.data, undo_item->str.length);
        buffer_on_modified(m);
        return undo_item->cursor;
    };

    buffer_save_redo(m, cursor);

    utf32_str_copy(&m->str, undo_item->str.data, undo_item->str.length);
    buffer_history_pop(&m->undo_history);

    buffer_on_modified(m);

    return undo_item->cursor;
}

struct text_position buffer_redo(struct buffer* m,
                                 struct text_position cursor) {
    // cursor position row is set to (size_t)-1 if there is nothing
    // in the redo buffer history
    struct text_position result = {
        .row = (size_t)-1,
    };

    if (!m->redo_history.length) return result;

    struct buffer_history_item* redo_item =
        buffer_history_top(&m->redo_history);
    buffer_save_undo(m, cursor);
    result = redo_item->cursor;

    utf32_str_copy(&m->str, redo_item->str.data, redo_item->str.length);
    buffer_history_pop(&m->redo_history);

    buffer_on_modified(m);
    return result;
}

void buffer_destroy(struct buffer* m) {
    utf32_str_destroy(&m->str);
    buffer_history_destroy(&m->undo_history);
    buffer_history_destroy(&m->redo_history);
    buffer_lines_destroy(&m->lines);
    buffer_syntax_destroy(&m->syntax);
}

void buffer_clear(struct buffer* m) {
    m->str.length = 0;
    buffer_on_modified(m);
}

void buffer_insert_char(struct buffer* m, const size_t pos,
                        const char32_t chr) {
    utf32_str_insert_char(&m->str, pos, chr);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_insert_utf8_buf(struct buffer* m, size_t pos, char* str,
                            size_t len) {
    utf32_str_insert_utf8_buf(&m->str, pos, str, len);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_insert_buf(struct buffer* m, size_t pos, char32_t* str,
                       size_t len) {
    utf32_str_insert_buf(&m->str, pos, str, len);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_delete(struct buffer* m, size_t pos, size_t count) {
    utf32_str_delete(&m->str, pos, count);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_copy(struct buffer* m, char32_t* buffer, size_t len) {
    utf32_str_copy(&m->str, buffer, len);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_read_file(struct buffer* m, const char* path) {
    utf32_str_read_file(&m->str, path);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}

void buffer_copy_utf8(struct buffer* m, const char* buffer,
                      size_t len) {
    utf32_str_copy_utf8(&m->str, buffer, len);
    buffer_on_modified(m);
    buffer_history_clear(&m->redo_history);
}
