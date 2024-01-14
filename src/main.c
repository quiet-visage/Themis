#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <string.h>
#include <uchar.h>

#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "highlighter/highlighter.h"
#include "menu/fuzzy_menu.h"
#include "resources/resources.h"
#include "text/cursor.h"
#include "text/file_editor.h"
#include "text/text.h"
#include "tile_layout.h"

struct focus {
    int picker_flags;
    int editor_flags;
};

void open_file_picker(struct focus* focus) {
    focus->picker_flags |=
        focus_flag_can_scroll | focus_flag_can_interact;
    focus->editor_flags = 0;
}
void close_file_picker(struct focus* focus) {
    focus->picker_flags = 0;
    focus->editor_flags |=
        focus_flag_can_scroll | focus_flag_can_interact;
}

int main(void) {
    InitWindow(800, 800, "Themis");
    resources_init();
    hlr_init();
    // win_layout_init();
    tile_set_root((Rectangle){.x = 200,
                              .y = 200,
                              .width = GetScreenWidth() - 400,
                              .height = GetScreenHeight() - 400});
    tile_perform();
    cursor_initialize();
    SetTargetFPS(60);

    ff_initialize("430");
    int maple_mono =
        ff_new_font("/usr/share/fonts/TTF/mplus-2m-regular.ttf",
                    ff_default_font_config());

    struct file_editor fe = file_editor_create();

    SetExitKey(0);

    struct file_picker fp = file_picker_create();
    struct ff_typography typo = {
        .font = maple_mono, .size = 14.f, .color = 0xffffffff};

    struct focus focus = {0};
    open_file_picker(&focus);

    preview_init();

    short win_count = 1;
    short win_focused = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});

        file_editor_draw(
            &fe, typo,
            (Rectangle){GetScreenWidth() * 0.5f -
                            (GetScreenWidth() * 0.7f) * 0.5f,
                        0, GetScreenWidth() * 0.7, GetScreenHeight()},
            focus.editor_flags);

        if (IsKeyPressed(KEY_F1)) {
            open_file_picker(&focus);
        }

        if (focus.picker_flags & focus_flag_can_interact &&
            IsKeyPressed(KEY_ESCAPE)) {
            close_file_picker(&focus);
        }

        if (focus.picker_flags & focus_flag_can_interact) {
            const char* result =
                file_picker_perform(&fp, typo, focus.picker_flags);

            if (result) {
                file_editor_open(&fe, result);
                close_file_picker(&focus);
            }
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) &&
            IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_EQUAL))
            typo.size += 1;

        if (IsKeyDown(KEY_LEFT_CONTROL) &&
            IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_MINUS))
            typo.size -= 1;

        if (IsWindowResized()) {
            tile_set_root((Rectangle){.x = 200,
                              .y = 200,
                              .width = GetScreenWidth() - 400,
                              .height = GetScreenHeight() - 400});
            tile_perform();
        }

        for (short i = 0; i < win_count; i += 1) {
            Rectangle rec = tile_get_rect(i);
            char name[32] = {0};
            snprintf(name, 32, "%d", i);
            DrawText(name, rec.x, rec.y, 24, WHITE);
            DrawRectangleLinesEx(rec, 2.0f,
                                 i == win_focused ? PINK : BLUE);
        }

        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (IsKeyPressed(KEY_THREE)) {
                tile_split(split_horizontal, win_focused);
                win_count = tile_get_count();
            }
            if (IsKeyPressed(KEY_TWO)) {
                tile_split(split_vertical, win_focused);
                win_count = tile_get_count();
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                win_focused += 1;
                if (win_focused > win_count - 1)
                    win_focused = win_count - 1;
            }
            if (IsKeyPressed(KEY_LEFT)) {
                if (win_focused > 0) win_focused -= 1;
            }
            if (IsKeyPressed(KEY_FOUR)) {
                if (tile_get_count()> 1) {
                    
                tile_remove(win_focused);
                win_count = tile_get_count();
                }
            }
        }

        EndDrawing();
    }

    file_picker_destroy(&fp);
    file_editor_destroy(&fe);

    cursor_terminate();
    ff_terminate();
    hlr_terminate();
    preview_terminate();
    return 0;
}

