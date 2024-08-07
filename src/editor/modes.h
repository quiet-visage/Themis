#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum editor_mode {
    editor_mode_normal,
    editor_mode_selection,
    editor_mode_search,
};

enum line_editor_mode {
    line_editor_mode_normal,
    line_editor_mode_selection,
};

#ifdef __cplusplus
}
#endif
