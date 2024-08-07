#pragma once
#include "../text_view.h"

/*
    only access this struct through it's interface
*/
struct File_Preview {
    buffer_t buffer;
    text_view_t text;
    long path_last_modified;

    void create(const char* path);
    void destroy();
};

namespace file_preview_interface {
void init();
void terminate();
File_Preview* get_preview(const char* path);
};  // namespace file_preview_interface
