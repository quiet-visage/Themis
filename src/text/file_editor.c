#include "file_editor.h"

#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <stdio.h>

#include "../editor/editor.h"
#include "../focus.h"
#include "../highlighter/highlighter.h"
#include "../text/file_editor.h"
#include "../text/text.h"
#include "../text/unicode_string.h"
#include "../config/config.h"

struct file_editor file_editor_create(void) {
    struct file_editor result = {.file_path = 0,
                                 .editor = editor_create(),
                                 .status_line_text = text_create()};
    return result;
}

void file_editor_destroy(struct file_editor* fe) {
    editor_destroy(&fe->editor);
    text_destroy(&fe->status_line_text);
}

void file_editor_open(struct file_editor* this, const char* file_path) {
    assert(IsPathFile(file_path));
    this->file_path = file_path;
    string32_copy_file(&this->editor.text.buffer, file_path);

    const char* file_extension = GetFileExtension(file_path);
    if (file_extension) {
        enum language file_language =
            hlr_get_extension_language(file_extension);
        text_set_syntax_language(&this->editor.text, file_language);
    }

    text_on_modified(&this->editor.text);
    editor_save_history(&this->editor, &this->editor.undo_stack);

    this->editor.cursor.row = 0;
    this->editor.cursor.column = 0;

    string32_copy_utf8(&this->status_line_text.buffer, this->file_path, strlen(this->file_path));
    text_on_modified(&this->status_line_text);
}

void file_editor_save(struct file_editor* fe) {
    if (!fe->file_path) return;
    FILE* file = fopen(fe->file_path, "w");
    assert(file != NULL);

    char new_contents[fe->editor.text.buffer.size];
    memset(new_contents, 0, fe->editor.text.buffer.size);
    ff_utf32_to_utf8(new_contents, fe->editor.text.buffer.data,
                     fe->editor.text.buffer.size);

    fwrite(new_contents, 1, fe->editor.text.buffer.size, file);
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

    Rectangle status_line_bounds = bounds;
    status_line_bounds.height = typo.size + g_layout.padding;
    text_draw(&this->status_line_text, typo, status_line_bounds, 0);

    Rectangle editor_bounds = bounds;
    editor_bounds.height -= status_line_bounds.height;
    editor_bounds.y += status_line_bounds.height;

    editor_draw(&this->editor, typo, editor_bounds, focus_flags);
}
