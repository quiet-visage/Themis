#include <field_fusion/fieldfusion.h>
#include "highlighter/highlighter.h"
#include <resources/resources.h>
#include "menu/fuzzy_menu.h"
#include <raylib.h>
#include <string.h>
#include <text/editor.h>
#include <text/file_editor.h>
#include <text/text.h>
#include <uchar.h>

typedef struct {
    fuzzy_menu_t menu;
    char dir[256];
    char result[256];
} file_picker_t;

void file_picker_reload_options(file_picker_t* fp) {
    FilePathList files = LoadDirectoryFiles(fp->dir);
    if (files.paths == NULL) return;
    fuzzy_menu_reset(&fp->menu);
    for (size_t i = 0; i < files.count; i += 1) {
        const char* file_name = GetFileName(files.paths[i]);
        size_t name_len = strlen(file_name);
        char32_t option[name_len];
        ff_utf8_to_utf32(option, file_name, name_len);
        fuzzy_menu_push_option(&fp->menu, option, name_len);
    }
    UnloadDirectoryFiles(files);
}

file_picker_t file_picker_create() {
    file_picker_t result = {.menu = fuzzy_menu_create(), .dir = {0}};
    const char* cwd = GetWorkingDirectory();
    size_t cwd_count = strlen(cwd);
    memcpy(result.dir, cwd, cwd_count);
    file_picker_reload_options(&result);
    return result;
}

static size_t char32_str_len(const char32_t* str) {
    const char32_t* ptr = str;
    size_t result = 0;
    while (*ptr != 0) {
        result += 1;
        ptr += 1;
    }
    return result;
}

void file_picker_up_dir(file_picker_t* fp) {
    size_t last_slash_i = 0;
    size_t current_dir_path_len = strlen(fp->dir);
    for (size_t i = 0; i < current_dir_path_len; i += 1)
        if (fp->dir[i] == '/') last_slash_i = i;
    if (last_slash_i < 2) return;
    fp->dir[last_slash_i] = 0;
    printf("%s\n", fp->dir);
    file_picker_reload_options(fp);
}

const char* file_picker_perform(file_picker_t* fp,
                                ff_typography_t typo, bool focused) {
    if (IsKeyDown(KEY_LEFT_ALT) && IsKeyReleased(KEY_COMMA)) {
        file_picker_up_dir(fp);
    }
    const char32_t* result =
        fuzzy_menu_perform(&fp->menu, typo, focused);
    if (result != NULL) {
        const char* dir_path = fp->dir;
        size_t dir_path_len = strlen(dir_path);

        size_t result_len = char32_str_len(result);
        assert(dir_path_len + 1 + result_len < 256);
        memcpy(fp->result, dir_path, dir_path_len);
        memcpy(fp->result + dir_path_len, "/", 1);

        char utf8_result[result_len];
        ff_utf32_to_utf8(utf8_result, result, result_len);
        memcpy(fp->result + dir_path_len + 1, utf8_result,
               result_len);
        fp->result[dir_path_len + 1 + result_len] = 0;

        return fp->result;
    }
    return NULL;
}

void file_picker_destroy(file_picker_t* fp) {
    fuzzy_menu_destroy(&fp->menu);
}

int main(void) {
    InitWindow(800, 800, "Themis");
    resources_init();
    hlr_init();
    SetTargetFPS(120);

    ff_initialize("430");
    int maple_mono =
        ff_new_font("/usr/share/fonts/TTF/mplus-2m-regular.ttf",
                    ff_default_font_config());

    file_editor_t fe = file_editor_create();
    file_editor_open(&fe, __FILE__);

    SetExitKey(0);

    file_picker_t fp = file_picker_create();
    ff_typography_t typo = {
        .font = maple_mono, .size = 12.f, .color = 0xffffffff};
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});

        editor_draw(&fe.editor, typo, (Rectangle){20, 40, 760, 720},
                    false);
        DrawRectangleLinesEx((Rectangle){20, 40, 760, 720}, 1.0f,
                             WHITE);

        const char* result = file_picker_perform(&fp, typo, true);
        if (result != NULL) {
            printf("%s\n", result);
        }

        EndDrawing();
    }

    file_picker_destroy(&fp);
    file_editor_destroy(&fe);

    ff_terminate();
    hlr_terminate();
    return 0;
}
