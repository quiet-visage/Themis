#pragma once

#include "../dyn_strings/utf32_string.h"
#include "../text/text_view.h"

struct file_preview {
    struct buffer buffer;
    struct text_view text;
    long path_last_modified;
};

void preview_init(void);
void preview_terminate(void);
struct file_preview* get_preview(const char* path);
