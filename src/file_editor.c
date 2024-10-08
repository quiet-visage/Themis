#include "file_editor.h"

#include <fieldfusion.h>
#include <raylib.h>
#include <stdio.h>

#include "buffer/buffer_handler.h"
#include "commands.h"
#include "config.h"
#include "editor/editor.h"
#include "file_editor.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "rlgl.h"

void file_editor_destroy(file_editor_t* m) {
    editor_destroy(&m->editor);
    utf8_str_destroy(&m->status_line_str);
    ff_glyph_vec_destroy(&m->status_line_glyphs);
    utf8_str_destroy(&m->file_path);
}

static void file_editor_update_status_line_text(file_editor_t* m) {
    if (m->file_path.length < 32) {
        utf8_str_copy(&m->status_line_str, m->file_path.data,
                      m->file_path.length);
        return;
    }

    size_t second_to_last_slash_index = 0;
    size_t slash_counter = 0;

    for (size_t i = m->file_path.length; i-- > 0;) {
        if (m->file_path.data[i] != '/') continue;
        slash_counter += 1;
        second_to_last_slash_index = i;
        if (slash_counter == 2) break;
    }

    if (!second_to_last_slash_index) {
        utf8_str_copy(&m->status_line_str, m->file_path.data,
                      m->file_path.length);
    }

    char* short_str_ptr =
        &m->file_path.data[second_to_last_slash_index];
    size_t short_str_len = strlen(short_str_ptr);

    static const char* ellipsis = "[..]";
    size_t ellipsis_len = strlen(ellipsis);

    size_t final_name_len = ellipsis_len + short_str_len;
    char final_name[final_name_len + 1];
    final_name[final_name_len] = 0;
    memcpy(final_name, ellipsis, ellipsis_len);
    memcpy(&final_name[ellipsis_len], short_str_ptr, short_str_len);

    utf8_str_copy(&m->status_line_str, final_name, final_name_len);
}

static Rectangle file_editor_get_status_line_bounds(
    file_editor_t* m, ff_typo_t typo, Rectangle bounds) {
    Rectangle result = bounds;
    result.height = typo.size + g_cfg.layout.padding * 2;
    return result;
}

static void file_editor_update_status_line_glyphs(file_editor_t* m,
                                                  Rectangle bounds,
                                                  ff_typo_t typo,
                                                  int focus_flags) {
    typo.color = focus_flags & focus_flag_can_interact
                     ? g_cfg.color_scheme.highlight_fg
                     : g_cfg.color_scheme.fg;

    Rectangle status_line_bounds =
        file_editor_get_status_line_bounds(m, typo, bounds);

    ff_dimensions_t text_dimensions = ff_measure_utf8(
        m->status_line_str.data, m->status_line_str.length, typo.font,
        typo.size, 0);

    float text_x = status_line_bounds.x +
                   status_line_bounds.width * .5f -
                   text_dimensions.width * .5f;
    float text_y = status_line_bounds.y +
                   status_line_bounds.height * .5f -
                   text_dimensions.height * .5;

    ff_glyph_vec_clear(&m->status_line_glyphs);

    ff_print_utf8_vec(&m->status_line_glyphs, m->status_line_str.data,
                      m->status_line_str.length, typo, text_x, text_y,
                      ff_flag_default, 0);
}

void file_editor_create(file_editor_t* m) {
    memset(m, 0, sizeof(file_editor_t));
    m->file_path = utf8_str_create();
    editor_create(&m->editor);
    m->status_line_str = utf8_str_create();
    m->status_line_glyphs = ff_glyph_vec_create();
    file_editor_set_path(m, "[scratch]");
}

void file_editor_open(file_editor_t* m, const char* file_path) {
    assert(IsPathFile(file_path));

    buffer_t* stored_buffer = buffer_handler_get(file_path);
    utf8_str_copy(&m->file_path, file_path, strlen(file_path));
    editor_reset_mode(&m->editor);
    if (stored_buffer) {
        m->editor.text.buffer = stored_buffer;
    } else {
        buffer_t* new_buffer =
            buffer_handler_create_buffer(file_path);

        m->editor.text.buffer = new_buffer;

        const char* file_extension = GetFileExtension(file_path);
        if (file_extension) {
            enum language file_language =
                hlr_get_extension_language(file_extension);
            buffer_syntax_set_language(&m->editor.text.buffer->syntax,
                                       file_language);
        }

        buffer_read_file(m->editor.text.buffer, file_path);
    }

    file_editor_update_status_line_text(m);
    buffer_save_undo(m->editor.text.buffer, (text_pos_t){0});
}

void file_editor_save(file_editor_t* m) {
    if (!m->file_path.length) return;
    FILE* file = fopen(m->file_path.data, "w");
    assert(file != NULL);

    char new_contents[m->editor.text.buffer->str.length];
    memset(new_contents, 0, m->editor.text.buffer->str.length);
    ff_utf32_to_utf8(new_contents, m->editor.text.buffer->str.data,
                     m->editor.text.buffer->str.length);

    fwrite(new_contents, 1, m->editor.text.buffer->str.length, file);
    fclose(file);
}

void file_editor_set_path(file_editor_t* o, const char* path) {
    utf8_str_copy(&o->file_path, path, strlen(path));
    file_editor_update_status_line_text(o);
}

void file_editor_draw_status_line(file_editor_t* m, ff_typo_t typo,
                                  Rectangle bounds, int focus_flags) {
    file_editor_update_status_line_glyphs(m, bounds, typo,
                                          focus_flags);

    Rectangle bar_rec =
        file_editor_get_status_line_bounds(m, typo, bounds);
    DrawRectangleRec(bar_rec,
                     GetColor(g_cfg.color_scheme.surface0_bg));
    rlDrawRenderBatchActive();

    float projection[4][4];
    ff_get_ortho_projection(0, GetScreenWidth(), GetScreenHeight(), 0,
                            -1.0f, 1.0f, projection);
    ff_draw(typo.font, m->status_line_glyphs.data,
            m->status_line_glyphs.len, (float*)projection);
}

void file_editor_draw_fade(Rectangle editor_bounds) {
    float fade_out_height = 42;
    Rectangle fade_out_bounds = {
        .x = editor_bounds.x,
        .y = editor_bounds.y + editor_bounds.height - fade_out_height,
        .width = editor_bounds.width,
        .height = fade_out_height};

    Color bg = GetColor(g_cfg.color_scheme.bg);
    Color transparent_bg = bg;
    transparent_bg.a = 0;
    DrawRectangleGradientV(
        fade_out_bounds.x, fade_out_bounds.y, fade_out_bounds.width,
        fade_out_bounds.height, transparent_bg, bg);
}

void file_editor_handle_commands(file_editor_t* m, int focus_flags) {
    if (!(focus_flags & focus_flag_can_interact)) return;

    int cmd = key_seq_handler_get_command(g_cfg.keybinds.file_editor);
    if (cmd != -1 && cmd == file_editor_cmd_save) file_editor_save(m);
}

void file_editor_draw(file_editor_t* m, ff_typo_t typo,
                      Rectangle bounds, int focus_flags) {
    file_editor_handle_commands(m, focus_flags);

    if (m->file_path.length != m->status_line_str.length)
        file_editor_update_status_line_text(m);

    file_editor_draw_status_line(m, typo, bounds, focus_flags);

    Rectangle editor_bounds = bounds;
    float status_line_height =
        file_editor_get_status_line_bounds(m, typo, bounds).height;
    editor_bounds.y += status_line_height;
    editor_bounds.height -= status_line_height;
    editor_draw(&m->editor, typo, editor_bounds, focus_flags);

    file_editor_draw_fade(editor_bounds);
}
