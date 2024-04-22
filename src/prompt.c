#include "prompt.h"

#include <math.h>
#include <rlgl.h>

#include "buffer/buffer.h"
#include "config.h"
#include "dyn_strings/utf32_string.h"
#include "editor/line_editor.h"
#include "fieldfusion.h"
#include "raylib.h"

#define PROMPT_WIDTH 400

void prompt_create(prompt_t* m) {
    memset(m, 0, sizeof(*m));
    line_editor_create(&m->editor);
    m->editor.text.buffer = malloc(sizeof(buffer_t));
    buffer_create(m->editor.text.buffer, utf32_str_create());
    m->glyphs = ff_glyph_vec_create();
}

void prompt_clear(prompt_t* m) { line_editor_clear(&m->editor); }

inline void prompt_set_label(prompt_t* m, const c32_t* str,
                             size_t len, ff_typo_t typo) {
    ff_print_utf32_vec(&m->glyphs, str, len, typo, 0, 0,
                       ff_flag_default, 0);
}

prompt_result_t prompt_render(prompt_t* m) {
    float label_w =
        ff_measure_glyphs(m->glyphs.data, m->glyphs.len,
                          g_cfg.typo.font, g_cfg.typo.size, 0)
            .width;
    float w = fmaxf(PROMPT_WIDTH, label_w) + g_cfg.layout.padding * 2;
    float h = g_cfg.layout.padding * 4 + g_cfg.typo.size * 2;
    float label_bg_x = GetScreenWidth() * .5f - w * .5f;
    float label_bg_y = GetScreenHeight() * .5f - h * .5f;
    DrawRectangle(label_bg_x, label_bg_y, w, h, GRAY);
    rlDrawRenderBatchActive();
    float label_fg_x = label_bg_x + g_cfg.layout.padding;
    float label_fg_y = label_bg_y + g_cfg.layout.padding;
    ff_set_glyphs_pos(m->glyphs.data, m->glyphs.len, label_fg_x,
                      label_fg_y, g_cfg.typo.font, 0);
    ff_draw(g_cfg.typo.font, m->glyphs.data, m->glyphs.len,
            (float*)g_cfg.scr_proj);

    // float y = GetScreenHeight() * .5f - h * .5f;
    // float x = GetScreenWidth() * .5f - w * .5f;
    // DrawRectangle(x, y, w, h, WHITE);
    return (prompt_result_t){0};
}

void prompt_destroy(prompt_t* m) {
    buffer_destroy(m->editor.text.buffer);
    free(m->editor.text.buffer);
    line_editor_destroy(&m->editor);
    ff_glyph_vec_destroy(&m->glyphs);
}
