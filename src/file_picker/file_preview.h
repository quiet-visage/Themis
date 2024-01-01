#pragma once

#include <text/text.h>

struct file_preview {
    const char* path;
    struct text text;
    size_t path_last_modified;
};

void preview_init(void);
void preview_terminate(void);
struct file_preview* get_preview(const char* path);
