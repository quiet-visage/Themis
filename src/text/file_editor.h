#pragma once
#include <text/editor.h>

struct file_editor {
    const char* file_path;
    struct editor editor;
};

struct file_editor file_editor_create(void);
void file_editor_destroy(struct file_editor* fe);
void file_editor_open(struct file_editor* fe, const char* file_path);
void file_editor_save(struct file_editor* fe);
