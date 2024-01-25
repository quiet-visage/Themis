#include "file_editor.h"

#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <stdio.h>
#include <uchar.h>

#include "../config/config.h"
#include "../dyn_strings/utf32_string.h"
#include "../dyn_strings/utf8_string.h"
#include "../editor/editor.h"
#include "../focus.h"
#include "../highlighter/highlighter.h"
#include "../text/file_editor.h"

struct file_editor file_editor_create(void) {
    struct file_editor result = {.file_path = utf8_str_create(),
                                 .editor = editor_create(),
                                 .status_line_text = text_view_create()};
    static const char* placeholder = "<unnamed>";
    utf32_str_copy_utf8(&result.status_line_text.buffer, placeholder,
                        strlen(placeholder));
    text_view_on_modified(&result.status_line_text);
    return result;
}

void file_editor_destroy(struct file_editor* this) {
    editor_destroy(&this->editor);
    text_view_destroy(&this->status_line_text);
    utf8_str_destroy(&this->file_path);
}

static void file_editor_update_status_line_text(
    struct file_editor* this) {
    size_t second_to_last_slash_index = (size_t)-1;
    size_t slash_counter = 0;

    for (size_t i = this->file_path.length; i-- > 0;) {
        if (this->file_path.data[i] != '/') continue;
        slash_counter += 1;
        second_to_last_slash_index = i;
        if (slash_counter == 2) break;
    }

    if (!second_to_last_slash_index) {
        utf32_str_copy_utf8(&this->status_line_text.buffer,
                            this->file_path.data,
                            this->file_path.length);
    }

    char* short_str_ptr =
        &this->file_path.data[second_to_last_slash_index];
    size_t short_str_len = strlen(short_str_ptr);

    static const char* ellipsis = "[..]";
    size_t ellipsis_len = strlen(ellipsis);

    size_t final_name_len = ellipsis_len + short_str_len;
    char final_name[final_name_len + 1];
    final_name[final_name_len] = 0;
    memcpy(final_name, ellipsis, ellipsis_len);
    memcpy(&final_name[ellipsis_len], short_str_ptr, short_str_len);

    utf32_str_copy_utf8(&this->status_line_text.buffer, final_name,
                        final_name_len);

    text_view_on_modified(&this->status_line_text);
}

void file_editor_open(struct file_editor* this,
                      const char* file_path) {
    assert(IsPathFile(file_path));
    utf8_str_copy(&this->file_path, file_path, strlen(file_path));
    utf32_str_read_file(&this->editor.text.buffer, file_path);

    const char* file_extension = GetFileExtension(file_path);
    if (file_extension) {
        enum language file_language =
            hlr_get_extension_language(file_extension);
        text_view_set_syntax_language(&this->editor.text, file_language);
    }

    text_view_on_modified(&this->editor.text);
    editor_save_history(&this->editor, &this->editor.undo_stack);

    this->editor.cursor.row = 0;
    this->editor.cursor.column = 0;

    file_editor_update_status_line_text(this);
}

void file_editor_save(struct file_editor* this) {
    if (!this->file_path.length) return;
    FILE* file = fopen(this->file_path.data, "w");
    assert(file != NULL);

    char new_contents[this->editor.text.buffer.size];
    memset(new_contents, 0, this->editor.text.buffer.size);
    ff_utf32_to_utf8(new_contents, this->editor.text.buffer.data,
                     this->editor.text.buffer.size);

    fwrite(new_contents, 1, this->editor.text.buffer.size, file);
    fclose(file);
}

void file_editor_draw(struct file_editor* this,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags) {
    if (focus_flags & focus_flag_can_interact) {
        if (IsKeyPressed(KEY_F2)) {
            file_editor_save(this);
        }
    }

    Rectangle editor_bounds = bounds;
    if (this->status_line_text.buffer.size) {
        Rectangle status_line_bounds = bounds;
        status_line_bounds.height =
            typo.size + g_cfg.layout.padding * 2;
        DrawRectangleRec(status_line_bounds,
                         GetColor(g_cfg.color_scheme.surface0_bg));

        status_line_bounds.y = status_line_bounds.y +
                               status_line_bounds.height * .5f -
                               typo.size * .5f;
        {
            float text_width =
                ff_measure(typo.font,
                           this->status_line_text.buffer.data,
                           this->status_line_text.buffer.size,
                           typo.size, true)
                    .width;

            status_line_bounds.x = status_line_bounds.x +
                                   status_line_bounds.width * .5f -
                                   text_width * .5f;
        }

        struct ff_typography status_line_typo = typo;
        status_line_typo.color = focus_flags & focus_flag_can_interact
                                     ? g_cfg.color_scheme.highlight_fg
                                     : g_cfg.color_scheme.fg;
        text_view_draw(&this->status_line_text, status_line_typo,
                  status_line_bounds, 0);
        editor_bounds.height -= status_line_bounds.height;
        editor_bounds.y +=
            status_line_bounds.height + g_cfg.layout.padding;
    }

    editor_draw(&this->editor, typo, editor_bounds, focus_flags);

    float fade_out_height = 32;
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
