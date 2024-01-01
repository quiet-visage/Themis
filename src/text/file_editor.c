#include "file_editor.h"

#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <stdio.h>
#include <text/editor.h>
#include <text/file_editor.h>
#include <text/text.h>
#include <text/unicode_string.h>
#include "editor.h"
#include "raylib.h"
#include <focus.h>

struct file_editor file_editor_create(void) {
    struct file_editor result = {.file_path = 0,
                                 .editor = editor_create()};
    return result;
}

void file_editor_destroy(struct file_editor* fe) {
    editor_destroy(&fe->editor);
}

void file_editor_open(struct file_editor* fe, const char* file_path) {
    assert(IsPathFile(file_path));
    fe->file_path = file_path;
    string32_copy_file(&fe->editor.text.buffer, file_path);

    const char* file_extension = GetFileExtension(file_path);
    if (file_extension) {
        enum language file_language = hlr_get_extension_language(file_extension);
            text_set_syntax_language(&fe->editor.text, file_language);
    }

    text_on_modified(&fe->editor.text);
    editor_save_undo(&fe->editor);
    fe->editor.cursor.row = 0;
    fe->editor.cursor.column = 0;
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

void file_editor_draw(struct file_editor* fe,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags){
    if (focus_flags & focus_flag_can_interact) {
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
            file_editor_save(fe);
        }
    }
    editor_draw(&fe->editor, typo, bounds, focus_flags);
}
