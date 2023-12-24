#pragma once
#include <text/editor.h>

typedef struct {
    const char* file_path;
    editor_t editor;
} file_editor_t;

file_editor_t file_editor_create(void);
void file_editor_destroy(file_editor_t* fe);
void file_editor_open(file_editor_t* fe, const char* file_path);
void file_editor_save(file_editor_t* fe);
