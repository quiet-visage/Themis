#include "file_preview.hh"

#include <unordered_map>

#include "raylib.h"

namespace file_preview_map {
/*
NOTE: calling clear on this, will result in a memory leak
because File_Preview needs to be deallocated by calling
File_Preview::destroy();
NOTE: might change this to shared pointer
*/
std::unordered_map<const char*, File_Preview> g_map;

void init() { return; }

void terminate() {
    for (auto& elem : g_map) {
        bool empty = !elem.second.path_last_modified;
        assert(!empty);
        elem.second.destroy();
    }
}

File_Preview* get_preview(const char* path) {
    if (!IsPathFile(path)) return 0;
    File_Preview* preview = &g_map[path];
    bool empty = !preview->path_last_modified;

    if (empty) {
        preview->create(path);
    } else {
        size_t file_last_modified = GetFileModTime(path);
        if (file_last_modified > preview->path_last_modified)
            buffer_read_file(&preview->buffer, path);
    }

    return preview;
}
}  // namespace file_preview_map

void File_Preview::create(const char* path) {
    assert(IsPathFile(path));
    path_last_modified = GetFileModTime(path);
    buffer_create(&buffer, utf32_str_create());
    text = text_view_create();
    text.buffer = &buffer;

    buffer_read_file(text.buffer, path);
    const char* file_extension = GetFileExtension(path);
    if (file_extension) {
        enum language file_language =
            hlr_get_extension_language(file_extension);
        buffer_syntax_set_language(&text.buffer->syntax,
                                   file_language);
        buffer_syntax_update(&text.buffer->syntax,
                             text.buffer->str.data,
                             text.buffer->str.length);
    }
}

void File_Preview::destroy() {
    text_view_destroy(&text);
    buffer_destroy(&buffer);
}
