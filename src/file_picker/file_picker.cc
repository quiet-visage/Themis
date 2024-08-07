#include "file_picker.hh"

#include <raylib.h>

#include "../config.h"
#include "fieldfusion.h"
#include "file_preview.hh"

#define FILE_PICKER_FILE_PREVIEW_HEIGHT_PERC 0.9f
#define FILE_PICKER_FILE_PREVIEW_WIDTH_PERC 0.75f

void File_Picker::reload_options() {
    FilePathList files = LoadDirectoryFiles(dir);
    if (files.paths == NULL) return;
    fuzzy_menu_reset(&menu);

    for (size_t i = 0; i < files.count; i += 1) {
        const char* file_name = GetFileName(files.paths[i]);
        size_t name_len = strlen(file_name);
        c32_t* option = (c32_t*)alloca(name_len * sizeof(c32_t));
        (void)ff_utf8_to_utf32(option, file_name, name_len);

        char file_path[256] = {0};
        size_t dir_len = strlen(dir);
        memcpy(file_path, dir, dir_len);
        memcpy(&file_path[dir_len], "/", 1);
        memcpy(&file_path[dir_len + 1], file_name, strlen(file_name));

        if (IsPathFile(file_path)) {
            fuzzy_menu_push_option_with_icon(&menu, option, name_len,
                                             icon_file_t);
        } else if (DirectoryExists(file_path)) {
            fuzzy_menu_push_option_with_icon(&menu, option, name_len,
                                             icon_folder_t);
        } else {
            fuzzy_menu_push_option(&menu, option, name_len);
        }
    }

    UnloadDirectoryFiles(files);
}

File_Picker::Dimensions File_Picker::get_dimensions(ff_typo_t typo) {
    const Vector2 window_size = {.x = (float)GetScreenWidth(),
                                 .y = (float)GetScreenHeight()};
    File_Picker::Dimensions result = {};
    result.menu_dimensions =
        fuzzy_menu_get_dimensions(&menu, typo, window_size);

    result.preview_bg_height =
        window_size.y * FILE_PICKER_FILE_PREVIEW_HEIGHT_PERC;
    result.preview_bounds_height =
        result.preview_bg_height - g_cfg.layout.padding * 2;

    result.preview_bg_y =
        window_size.y * 0.5f - result.preview_bg_height * 0.5f;
    result.preview_bounds_y =
        result.preview_bg_y + g_cfg.layout.padding;
    result.menu_dimensions.editor_bg_y = result.preview_bg_y;
    result.menu_dimensions.editor_y = result.preview_bounds_y;
    result.menu_dimensions.editor_y = result.preview_bounds_y;
    result.menu_dimensions.options_bg_y =
        result.menu_dimensions.editor_bg_y +
        result.menu_dimensions.editor_bg_height;
    result.menu_dimensions.options_y =
        result.menu_dimensions.options_bg_y + g_cfg.layout.padding;

    result.preview_bg_width =
        (window_size.x - result.menu_dimensions.bg_width) *
        FILE_PICKER_FILE_PREVIEW_WIDTH_PERC;
    result.preview_bounds_width =
        result.preview_bg_width - g_cfg.layout.padding * 2;

    result.menu_dimensions.editor_width =
        result.menu_dimensions.bg_width - g_cfg.layout.padding * 2;
    result.menu_dimensions.bg_x =
        window_size.x * 0.5f -
        (result.preview_bg_width + result.menu_dimensions.bg_width) *
            0.5f;
    result.preview_bg_x =
        result.menu_dimensions.bg_x + result.menu_dimensions.bg_width;
    result.preview_bounds_x =
        result.preview_bg_x + g_cfg.layout.padding;

    result.menu_dimensions.options_bg_height =
        result.preview_bg_height -
        result.menu_dimensions.editor_bg_height;

    result.menu_dimensions.options_y =
        result.menu_dimensions.options_bg_y + g_cfg.layout.padding;
    result.menu_dimensions.bounds_x =
        result.menu_dimensions.bg_x + g_cfg.layout.padding;
    result.menu_dimensions.options_x =
        result.menu_dimensions.bounds_x + 24;  // NOTE
    result.menu_dimensions.options_height =
        result.menu_dimensions.options_bg_height -
        g_cfg.layout.padding * 2;

    return result;
}

void File_Picker::init() {
    fuzzy_menu_create(&menu);
    const char* cwd = GetWorkingDirectory();
    size_t cwd_len = strlen(cwd);
    memcpy(dir, cwd, cwd_len);
    reload_options();
}

void File_Picker::draw_preview(ff_typo_t typo,
                               File_Picker::Dimensions dimensions,
                               int focus_flags) {
    const c32_t* selected = menu.options[menu.selected].name;

    size_t selected_len = menu.options[menu.selected].name_len;
    size_t dir_path_len = strlen(dir);
    size_t selected_file_path_len = dir_path_len + 1 + selected_len;

    char* selected_file_path =
        (char*)alloca(selected_file_path_len + 1);
    selected_file_path[selected_file_path_len] = 0;

    assert(dir_path_len + 1 + selected_len < 256);
    memcpy(selected_file_path, dir, dir_path_len);
    memcpy(&selected_file_path[dir_path_len], "/", 1);

    char* selected_utf8 = (char*)alloca(selected_len + 1);
    selected_utf8[selected_len] = 0;
    (void)ff_utf32_to_utf8(selected_utf8, selected, selected_len);
    memcpy(&selected_file_path[dir_path_len + 1], selected_utf8,
           selected_len);

    Rectangle preview_bg = {.x = dimensions.preview_bg_x,
                            .y = dimensions.preview_bg_y,
                            .width = dimensions.preview_bg_width,
                            .height = dimensions.preview_bg_height};

    DrawRectangleRec(preview_bg,
                     GetColor(g_cfg.color_scheme.surface1_bg));
    if (IsPathFile(selected_file_path)) {
        File_Preview* preview =
            file_preview_interface::get_preview(selected_file_path);
        if (!preview) return;

        Rectangle file_preview_bounds = {
            .x = dimensions.preview_bounds_x,
            .y = dimensions.preview_bounds_y,
            .width = dimensions.preview_bounds_width,
            .height = dimensions.preview_bounds_height};

        BeginScissorMode(file_preview_bounds.x, file_preview_bounds.y,
                         file_preview_bounds.width,
                         file_preview_bounds.height);
        text_view_draw(&preview->text, typo, file_preview_bounds,
                       focus_flags, 0, 0, 0, 0);
        EndScissorMode();
    }
}

char* File_Picker::ui_view(ff_typo_t typo, int focus_flags) {
    File_Picker::Dimensions dimensions = get_dimensions(typo);
    fuzzy_menu_draw_editor(&menu, typo, focus_flags,
                           dimensions.menu_dimensions);
    fuzzy_menu_draw_options(&menu, typo, dimensions.menu_dimensions);

    if (fuzzy_menu_buffer_changed(&menu))
        fuzzy_menu_on_buffer_change(&menu);

    draw_preview(typo, dimensions, focus_flags);

    const c32_t* file_picked = NULL;

    if (focus_flags & focus_flag_can_interact) {
        int cmd =
            key_seq_handler_get_command(g_cfg.keybinds.file_picker);
        if (cmd != -1) {
            switch (cmd) {
                case file_picker_cmd_previous_dir:
                    file_picker_up_dir(fp);
                    break;
            }
        }
        file_picked = fuzzy_menu_handle_user_input(&menu);
    }

    if (file_picked) {
        const char* dir_path = dir;
        size_t dir_path_len = strlen(dir_path);

        size_t result_len = c32_str_len(file_picked);
        assert(dir_path_len + 1 + result_len < 256);
        memcpy(result, dir_path, dir_path_len);
        memcpy(result + dir_path_len, "/", 1);

        char* utf8_result = (char*)alloca(result_len);
        ff_utf32_to_utf8(utf8_result, file_picked, result_len);
        memcpy(result + dir_path_len + 1, utf8_result, result_len);
        result[dir_path_len + 1 + result_len] = 0;

        if (DirectoryExists(result)) {
            memset(dir, 0, 256);
            memcpy(dir, result, strlen(result));
            file_picker_reload_options(fp);
            return NULL;
        }

        return result;
    }

    return NULL;
}

void File_Picker::destroy() { fuzzy_menu_destroy(& > menu); }
