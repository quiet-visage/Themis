#pragma once
#include <uchar.h>

#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"
#include "dyn_strings/utf32_string.h"
#include "highlighter/highlighter.h"

#define BUFFER_NAME_CAP 0x100

struct buffer {
    char buffer_name[BUFFER_NAME_CAP];
    struct utf32_str str;
    struct buffer_history undo_history;
    struct buffer_history redo_history;
    struct buffer_lines lines;
    struct buffer_syntax syntax;
};

void buffer_create(struct buffer* o, struct utf32_str data);
void buffer_destroy(struct buffer* o);
void buffer_clear(struct buffer* o);
void buffer_set_name(struct buffer* o, const char* name);
void buffer_save_undo(struct buffer* o);
void buffer_undo(struct buffer* o);
void buffer_redo(struct buffer* o);
void buffer_insert_char(struct buffer* o, const size_t pos,
                        const char32_t chr);
void buffer_insert_utf8_buf(struct buffer* o, size_t pos, char* str,
                            size_t len);
void buffer_insert_buf(struct buffer* o, size_t pos, char32_t* str,
                       size_t len);
void buffer_delete(struct buffer* o, size_t pos, size_t count);
void buffer_copy(struct buffer* o, char32_t* buffer, size_t len);
void buffer_copy_utf8(struct buffer* o, const char* buffer,
                      size_t len);
void buffer_read_file(struct buffer* o, const char* path);
