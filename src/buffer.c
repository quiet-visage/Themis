#include "buffer.h"

#include <string.h>
#include <assert.h>

#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"
#include "dyn_strings/utf32_string.h"

void buffer_create(struct buffer* o, struct utf32_str data) {
    o->str = data;
    o->undo_history = buffer_history_create();
    o->redo_history = buffer_history_create();
    o->lines = buffer_lines_create();
    o->syntax = buffer_syntax_create();
    buffer_lines_update(&o->lines, o->str.data, o->str.size);
}

void buffer_set_name(struct buffer* o, const char* name) {
    size_t buffer_name_len = strlen(name);

    if (buffer_name_len > BUFFER_NAME_CAP) {
        memcpy(o->buffer_name,
               &name[buffer_name_len - BUFFER_NAME_CAP],
               BUFFER_NAME_CAP);
        o->buffer_name[BUFFER_NAME_CAP - 1] = 0;
        return;
    }

    memcpy(o->buffer_name, name, buffer_name_len);
    o->buffer_name[buffer_name_len] = 0;
}

void buffer_save_undo(struct buffer* o) {
    buffer_history_copy_and_push(&o->undo_history, &o->str);
}

static void buffer_save_redo(struct buffer* o) {
    buffer_history_copy_and_push(&o->redo_history, &o->str);
}

static void buffer_on_modified(struct buffer* o) {
    buffer_lines_update(&o->lines, o->str.data, o->str.size);
    buffer_syntax_update(&o->syntax, o->str.data, o->str.size);
}

void buffer_undo(struct buffer* o) {
    if (!o->undo_history.length) return;

    buffer_save_redo(o);
    struct utf32_str* undo_str = buffer_history_top(&o->undo_history);

    utf32_str_copy(&o->str, undo_str->data, undo_str->size);
    buffer_history_pop(&o->undo_history);

    buffer_on_modified(o);
}

void buffer_redo(struct buffer* o) {
    if (!o->redo_history.length) return;

    buffer_save_undo(o);
    struct utf32_str* redo_str = buffer_history_top(&o->redo_history);

    utf32_str_copy(&o->str, redo_str->data, redo_str->size);
    buffer_history_pop(&o->redo_history);

    buffer_on_modified(o);
}

void buffer_destroy(struct buffer* o) {
    utf32_str_destroy(&o->str);
    buffer_history_destroy(&o->undo_history);
    buffer_history_destroy(&o->redo_history);
    buffer_lines_destroy(&o->lines);
    buffer_syntax_destroy(&o->syntax);
}

void buffer_clear(struct buffer* o) {
    o->str.size = 0;
    buffer_on_modified(o);
}

void buffer_insert_char(struct buffer* o, const size_t pos,
                        const char32_t chr) {
    utf32_str_insert_char(&o->str, pos, chr);
    buffer_on_modified(o);
}

void buffer_insert_utf8_buf(struct buffer* o, size_t pos, char* str,
                            size_t len) {
    utf32_str_insert_utf8_buf(&o->str, pos, str, len);
    buffer_on_modified(o);
}

void buffer_insert_buf(struct buffer* o, size_t pos, char32_t* str,
                       size_t len) {
    utf32_str_insert_buf(&o->str, pos, str, len);
    buffer_on_modified(o);
}

void buffer_delete(struct buffer* o, size_t pos, size_t count) {
    utf32_str_delete(&o->str, pos, count);
    buffer_on_modified(o);
}

void buffer_copy(struct buffer* o, char32_t* buffer, size_t len) {
    utf32_str_copy(&o->str, buffer, len);
    buffer_on_modified(o);
}

void buffer_read_file(struct buffer* o, const char* path) {
    utf32_str_read_file(&o->str, path);
    buffer_on_modified(o);
}


void buffer_copy_utf8(struct buffer* o, const char* buffer,
                      size_t len){
    utf32_str_copy_utf8(&o->str, buffer, len);
    buffer_on_modified(o);
}
