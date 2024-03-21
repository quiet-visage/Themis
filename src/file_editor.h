#pragma once
#include <fieldfusion.h>
#include <raylib.h>

#include "dyn_strings/utf32_string.h"
#include "dyn_strings/utf8_string.h"
#include "editor/editor.h"

struct file_editor {
    struct utf8_str file_path;
    struct editor editor;
    struct utf8_str status_line_str;
    struct ff_glyphs_vector status_line_glyphs;
};

void file_editor_create(struct file_editor* m);
void file_editor_destroy(struct file_editor* m);
void file_editor_open(struct file_editor* m, const char* file_path);
void file_editor_set_path(struct file_editor* m, const char* path);
void file_editor_draw(struct file_editor* m,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags);
void file_editor_save(struct file_editor* m);
