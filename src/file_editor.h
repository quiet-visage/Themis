#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <fieldfusion.h>
#include <raylib.h>

#include "dyn_strings/utf8_string.h"
#include "editor/editor.h"

typedef struct {
    utf8_str_t file_path;
    editor_t editor;
    utf8_str_t status_line_str;
    ff_glyph_vec_t status_line_glyphs;
} file_editor_t;

void file_editor_create(file_editor_t* m);
void file_editor_destroy(file_editor_t* m);
void file_editor_open(file_editor_t* m, const char* file_path);
void file_editor_set_path(file_editor_t* m, const char* path);
void file_editor_draw(file_editor_t* m, ff_typo_t typo,
                      Rectangle bounds, int focus_flags);
void file_editor_save(file_editor_t* m);

#ifdef __cplusplus
}
#endif

