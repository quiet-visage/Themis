#pragma once

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"
#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"

#define BUFFER_NAME_CAP 0x100

typedef struct {
    char buffer_name[BUFFER_NAME_CAP];
    utf32_str_t str;
    buffer_history_t undo_history;
    buffer_history_t redo_history;
    buffer_lines_t lines;
    buffer_syntax_t syntax;
    size_t str_last_checked_size;
} buffer_t;

void buffer_create(buffer_t* m, utf32_str_t data);
void buffer_destroy(buffer_t* m);
void buffer_clear(buffer_t* m);
void buffer_set_name(buffer_t* m, const char* name);
void buffer_save_undo(buffer_t* m, text_pos_t cursor);
text_pos_t buffer_undo(buffer_t* m, text_pos_t cursor);
text_pos_t buffer_redo(buffer_t* m, text_pos_t cursor);
void buffer_insert_char(buffer_t* m, const size_t pos,
                        const c32_t chr);
void buffer_insert_utf8_buf(buffer_t* m, size_t pos, char* str,
                            size_t len);
void buffer_insert_buf(buffer_t* m, size_t pos, c32_t* str,
                       size_t len);
void buffer_delete(buffer_t* m, size_t pos, size_t count);
void buffer_copy(buffer_t* m, c32_t* buffer, size_t len);
void buffer_copy_utf8(buffer_t* m, const char* buffer, size_t len);
void buffer_append_utf8(buffer_t* m, const char* buffer, size_t len);
void buffer_read_file(buffer_t* m, const char* path);
