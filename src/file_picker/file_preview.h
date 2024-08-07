#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../text_view.h"

typedef struct {
    buffer_t buffer;
    text_view_t text;
    long path_last_modified;
} file_preview_t;

void preview_init(void);
void preview_terminate(void);
file_preview_t* get_preview(const char* path);

#ifdef __cplusplus
}
#endif
