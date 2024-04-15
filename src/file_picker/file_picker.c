#include "file_picker.h"

#include <fieldfusion.h>

#include "../commands.h"
#include "../config.h"
#include "../focus.h"
#include "file_preview.h"
#include "raylib.h"

static size_t c32_str_len(const c32_t* str) {
    const c32_t* ptr = str;
    size_t result = 0;
    while (*ptr != 0) {
        result += 1;
        ptr += 1;
    }
    return result;
}

static void file_picker_reload_options(struct file_picker* fp) {
    FilePathList files = LoadDirectoryFiles(fp->dir);
    if (files.paths == NULL) return;
    fuzzy_menu_reset(&fp->menu);

    for (size_t i = 0; i < files.count; i += 1) {
        const char* file_name = GetFileName(files.paths[i]);
        size_t name_len = strlen(file_name);
        c32_t option[name_len];
        (void)ff_utf8_to_utf32(option, file_name, name_len);

        char file_path[256] = {0};
        size_t dir_len = strlen(fp->dir);
        memcpy(file_path, fp->dir, dir_len);
        memcpy(&file_path[dir_len], "/", 1);
        memcpy(&file_path[dir_len + 1], file_name, strlen(file_name));

        if (IsPathFile(file_path)) {
            fuzzy_menu_push_option_with_icon(&fp->menu, option,
                                             name_len, icon_file_t);
        } else if (DirectoryExists(file_path)) {
            fuzzy_menu_push_option_with_icon(&fp->menu, option,
                                             name_len, icon_folder_t);
        } else {
            fuzzy_menu_push_option(&fp->menu, option, name_len);
        }
    }

    UnloadDirectoryFiles(files);
}

struct file_picker file_picker_create() {
    struct file_picker result = {0};
    fuzzy_menu_create(&result.menu);

    const char* cwd = GetWorkingDirectory();
    size_t cwd_len = strlen(cwd);

    memcpy(result.dir, cwd, cwd_len);
    file_picker_reload_options(&result);

    return result;
}

static void file_picker_up_dir(struct file_picker* fp) {
    size_t last_slash_i = 0;
    size_t current_dir_path_len = strlen(fp->dir);
    for (size_t i = 0; i < current_dir_path_len; i += 1)
        if (fp->dir[i] == '/') last_slash_i = i;
    if (last_slash_i < 2) return;
    fp->dir[last_slash_i] = 0;
    file_picker_reload_options(fp);
}

struct file_picker_dimensions {
    struct fuzzy_menu_dimensions fz_menu_dimensions;

    float file_preview_bg_x;
    float file_preview_bounds_x;

    float file_preview_bg_y;
    float file_preview_bounds_y;

    float file_preview_bg_width;
    float file_preview_bounds_width;

    float file_preview_bg_height;
    float file_preview_bounds_height;
};

#define FILE_PICKER_FILE_PREVIEW_HEIGHT_PERC 0.9f
#define FILE_PICKER_FILE_PREVIEW_WIDTH_PERC 0.75f

struct file_picker_dimensions file_picker_get_dimensions(
    struct file_picker* fp, struct ff_typography typo) {
    const Vector2 window_size = {.x = GetScreenWidth(),
                                 .y = GetScreenHeight()};
    struct file_picker_dimensions result = {0};
    result.fz_menu_dimensions =
        fuzzy_menu_get_dimensions(&fp->menu, typo, window_size);

    result.file_preview_bg_height =
        window_size.y * FILE_PICKER_FILE_PREVIEW_HEIGHT_PERC;
    result.file_preview_bounds_height =
        result.file_preview_bg_height - g_cfg.layout.padding * 2;

    result.file_preview_bg_y =
        window_size.y * 0.5f - result.file_preview_bg_height * 0.5f;
    result.file_preview_bounds_y =
        result.file_preview_bg_y + g_cfg.layout.padding;
    result.fz_menu_dimensions.editor_bg_y = result.file_preview_bg_y;
    result.fz_menu_dimensions.editor_y = result.file_preview_bounds_y;
    result.fz_menu_dimensions.editor_y = result.file_preview_bounds_y;
    result.fz_menu_dimensions.options_bg_y =
        result.fz_menu_dimensions.editor_bg_y +
        result.fz_menu_dimensions.editor_bg_height;
    result.fz_menu_dimensions.options_y =
        result.fz_menu_dimensions.options_bg_y + g_cfg.layout.padding;

    result.file_preview_bg_width =
        (window_size.x - result.fz_menu_dimensions.bg_width) *
        FILE_PICKER_FILE_PREVIEW_WIDTH_PERC;
    result.file_preview_bounds_width =
        result.file_preview_bg_width - g_cfg.layout.padding * 2;

    result.fz_menu_dimensions.editor_width =
        result.fz_menu_dimensions.bg_width - g_cfg.layout.padding * 2;
    result.fz_menu_dimensions.bg_x =
        window_size.x * 0.5f - (result.file_preview_bg_width +
                                result.fz_menu_dimensions.bg_width) *
                                   0.5f;
    result.file_preview_bg_x = result.fz_menu_dimensions.bg_x +
                               result.fz_menu_dimensions.bg_width;
    result.file_preview_bounds_x =
        result.file_preview_bg_x + g_cfg.layout.padding;

    result.fz_menu_dimensions.options_bg_height =
        result.file_preview_bg_height -
        result.fz_menu_dimensions.editor_bg_height;

    result.fz_menu_dimensions.options_y =
        result.fz_menu_dimensions.options_bg_y + g_cfg.layout.padding;
    result.fz_menu_dimensions.bounds_x =
        result.fz_menu_dimensions.bg_x + g_cfg.layout.padding;
    result.fz_menu_dimensions.options_x =
        result.fz_menu_dimensions.bounds_x + 24;  // NOTE
    result.fz_menu_dimensions.options_height =
        result.fz_menu_dimensions.options_bg_height -
        g_cfg.layout.padding * 2;

    return result;
}

static void file_picker_draw_preview(
    struct file_picker* fp, struct ff_typography typo,
    struct file_picker_dimensions dimensions, int focus_flags) {
    const c32_t* selected = fp->menu.options[fp->menu.selected].name;

    size_t selected_len =
        fp->menu.options[fp->menu.selected].name_len;
    size_t dir_path_len = strlen(fp->dir);
    size_t selected_file_path_len = dir_path_len + 1 + selected_len;

    char selected_file_path[selected_file_path_len + 1];
    selected_file_path[selected_file_path_len] = 0;

    assert(dir_path_len + 1 + selected_len < 256);
    memcpy(selected_file_path, fp->dir, dir_path_len);
    memcpy(&selected_file_path[dir_path_len], "/", 1);

    char selected_utf8[selected_len + 1];
    selected_utf8[selected_len] = 0;
    (void)ff_utf32_to_utf8(selected_utf8, selected, selected_len);
    memcpy(&selected_file_path[dir_path_len + 1], selected_utf8,
           selected_len);

    Rectangle file_preview_bg = {
        .x = dimensions.file_preview_bg_x,
        .y = dimensions.file_preview_bg_y,
        .width = dimensions.file_preview_bg_width,
        .height = dimensions.file_preview_bg_height};

    DrawRectangleRec(file_preview_bg,
                     GetColor(g_cfg.color_scheme.surface1_bg));
    if (IsPathFile(selected_file_path)) {
        struct file_preview* preview =
            get_preview(selected_file_path);
        if (!preview) return;

        Rectangle file_preview_bounds = {
            .x = dimensions.file_preview_bounds_x,
            .y = dimensions.file_preview_bounds_y,
            .width = dimensions.file_preview_bounds_width,
            .height = dimensions.file_preview_bounds_height};

        BeginScissorMode(file_preview_bounds.x, file_preview_bounds.y,
                         file_preview_bounds.width,
                         file_preview_bounds.height);
        text_view_draw(&preview->text, typo, file_preview_bounds,
                       focus_flags, 0, 0, 0, 0);
        EndScissorMode();
    }
}

const char* file_picker_ui(struct file_picker* fp,
                                struct ff_typography typo,
                                int focus_flags) {
    struct file_picker_dimensions dimensions =
        file_picker_get_dimensions(fp, typo);
    fuzzy_menu_draw_editor(&fp->menu, typo, focus_flags,
                           dimensions.fz_menu_dimensions);
    fuzzy_menu_draw_options(&fp->menu, typo,
                            dimensions.fz_menu_dimensions);

    if (fuzzy_menu_buffer_changed(&fp->menu))
        fuzzy_menu_on_buffer_change(&fp->menu);

    file_picker_draw_preview(fp, typo, dimensions, focus_flags);

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
        file_picked = fuzzy_menu_handle_user_input(&fp->menu);
    }

    if (file_picked) {
        const char* dir_path = fp->dir;
        size_t dir_path_len = strlen(dir_path);

        size_t result_len = c32_str_len(file_picked);
        assert(dir_path_len + 1 + result_len < 256);
        memcpy(fp->result, dir_path, dir_path_len);
        memcpy(fp->result + dir_path_len, "/", 1);

        char utf8_result[result_len];
        ff_utf32_to_utf8(utf8_result, file_picked, result_len);
        memcpy(fp->result + dir_path_len + 1, utf8_result,
               result_len);
        fp->result[dir_path_len + 1 + result_len] = 0;

        if (DirectoryExists(fp->result)) {
            memset(fp->dir, 0, 256);
            memcpy(fp->dir, fp->result, strlen(fp->result));
            file_picker_reload_options(fp);
            return NULL;
        }

        return fp->result;
    }

    return NULL;
}

void file_picker_destroy(struct file_picker* fp) {
    fuzzy_menu_destroy(&fp->menu);
}
