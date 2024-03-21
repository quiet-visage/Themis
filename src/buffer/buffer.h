#pragma once
#include <uchar.h>

#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"
#include "buffer_history.h"
#include "buffer_lines.h"
#include "buffer_syntax.h"

#define BUFFER_NAME_CAP 0x100

struct buffer {
    char buffer_name[BUFFER_NAME_CAP];
    struct utf32_str str;
    struct buffer_history undo_history;
    struct buffer_history redo_history;
    struct buffer_lines lines;
    struct buffer_syntax syntax;
};

void buffer_create(struct buffer* m, struct utf32_str data);
void buffer_destroy(struct buffer* m);
void buffer_clear(struct buffer* m);
void buffer_set_name(struct buffer* m, const char* name);
void buffer_save_undo(struct buffer* m, struct text_position cursor);
struct text_position buffer_undo(struct buffer* m,
                                 struct text_position cursor);
struct text_position buffer_redo(struct buffer* m,
                                 struct text_position cursor);
void buffer_insert_char(struct buffer* m, const size_t pos,
                        const char32_t chr);
void buffer_insert_utf8_buf(struct buffer* m, size_t pos, char* str,
                            size_t len);
void buffer_insert_buf(struct buffer* m, size_t pos, char32_t* str,
                       size_t len);
void buffer_delete(struct buffer* m, size_t pos, size_t count);
void buffer_copy(struct buffer* m, char32_t* buffer, size_t len);
void buffer_copy_utf8(struct buffer* m, const char* buffer,
                      size_t len);
void buffer_read_file(struct buffer* m, const char* path);
