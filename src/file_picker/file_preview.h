#pragma once

#include "../text/text_view.h"

struct file_preview {
    const char* path;
    struct text_view text;
    long path_last_modified;
};

void preview_init(void);
void preview_terminate(void);
struct file_preview* get_preview(const char* path);
