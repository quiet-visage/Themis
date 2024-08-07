#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "editor/line_editor.h"
#include "fieldfusion.h"

typedef struct {
    line_editor_t editor;
    ff_glyph_vec_t glyphs;
} prompt_t;

typedef struct {
    c32_t* str;
    size_t len;
} prompt_result_t;

void prompt_create(prompt_t* m);
void prompt_clear(prompt_t* m);
void prompt_set_label(prompt_t* m, const c32_t* str, size_t len,
                      ff_typo_t typo);
prompt_result_t prompt_render(prompt_t* m);
void prompt_destroy(prompt_t* m);

#ifdef __cplusplus
}
#endif
