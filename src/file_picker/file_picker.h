#pragma once
#include "../fuzzy_menu.h"

struct file_picker {
    struct fuzzy_menu menu;
    char dir[256];
    char result[256];
};

struct file_picker file_picker_create();
const char* file_picker_perform(struct file_picker* fp,
                                struct ff_typography typo,
                                int focus_flags);
void file_picker_destroy(struct file_picker* fp);
