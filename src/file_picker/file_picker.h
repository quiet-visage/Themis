#pragma once
#include "../fuzzy_menu.h"

typedef struct {
    fuzzy_menu_t menu;
    char dir[256];
    char result[256];
} file_picker_t;

file_picker_t file_picker_create();
const char* file_picker_ui(file_picker_t* fp, ff_typo_t typo,
                           int focus_flags);
void file_picker_destroy(file_picker_t* fp);
