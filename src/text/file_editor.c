#include <text/file_editor.h>

#include <stdio.h>

#include <text/editor.h>
#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <text/text.h>
#include <text/unicode_string.h>
#include "file_editor.h"

struct file_editor file_editor_create(void) {
    struct file_editor result = {.file_path = 0, .editor = editor_create()};
    return result;
}

void file_editor_destroy(struct file_editor* fe) { editor_destroy(&fe->editor); }

void file_editor_open(struct file_editor* fe, const char* file_path) {
    FILE* file = fopen(file_path, "r");
    assert(file != NULL);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* file_contents = calloc(1, file_size + 1);
    fread(file_contents, 1, file_size, file);

    string32_copy_utf8(&fe->editor.text.buffer, file_contents,
                       file_size);
    free(file_contents);

    const char* file_extension = GetFileExtension(file_path);
    if (file_extension) {
        if (strcmp(".c", file_extension) == 0) {
            text_set_syntax_language(&fe->editor.text, language_c_t);
        }else {
            text_set_syntax_language(&fe->editor.text, language_none_t);
        }
    }

    text_on_modified(&fe->editor.text);
    fe->editor.cursor.row = 0;
    fe->editor.cursor.column = 0;

    fclose(file);
}

void file_editor_save(struct file_editor* fe) {
    assert(fe->file_path != NULL);
    FILE* file = fopen(fe->file_path, "r");
    assert(file != NULL);

    char new_contents[fe->editor.text.buffer.size];
    memset(new_contents, 0, fe->editor.text.buffer.size);
    ff_utf32_to_utf8(new_contents, fe->editor.text.buffer.data,
                     fe->editor.text.buffer.size);

    fwrite(new_contents, 1, fe->editor.text.buffer.size, file);
    fclose(file);
}
