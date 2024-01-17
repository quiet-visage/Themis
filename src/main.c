#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <string.h>
#include <uchar.h>

#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "menu/fuzzy_menu.h"
#include "pane_controller.h"
#include "resources/resources.h"
#include "text/cursor.h"
#include "text/file_editor.h"
#include "text/text.h"
#include "tile_layout.h"

struct focus {
    int picker_flags;
    int pane_flags;
};

static void focus_file_picker(struct focus* focus) {
    focus->picker_flags =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->pane_flags = 0;
}

static void focus_pane_controller(struct focus* focus) {
    focus->pane_flags =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->picker_flags = 0;
}

int main(void) {
    InitWindow(800, 800, "Themis");
    SetTargetFPS(60);
    SetExitKey(0);

    ff_initialize("430");
    resources_init();
    hlr_init();
    cursor_initialize();
    preview_init();
    pane_controller_init(
        (Rectangle){.x = 100,
                    .y = 100,
                    .width = GetScreenWidth() - 200,
                    .height = GetScreenHeight() - 200});

    struct file_picker file_picker = file_picker_create();

    int maple_mono =
        ff_new_font("/usr/share/fonts/TTF/mplus-2m-regular.ttf",
                    ff_default_font_config());
    struct ff_typography typo = {
        .font = maple_mono, .size = 14.f, .color = 0xffffffff};

    struct focus focus = {0};

    focus_pane_controller(&focus);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});

        pane_controler_draw(typo, focus.pane_flags);

        if (focus.picker_flags & focus_flag_can_interact) {
            const char* result = file_picker_perform(&file_picker, typo,
                                focus.picker_flags);
            if (result) {
                pane_controller_open_in_focused(result);
                focus_pane_controller(&focus);
            }
        }

        if (IsWindowResized()) {
            pane_controller_update_bounds(
                (Rectangle){.x = 100,
                            .y = 100,
                            .width = GetScreenWidth() - 200,
                            .height = GetScreenHeight() - 200});
        }

        if (IsKeyPressed(KEY_F1)) {
            focus_file_picker(&focus);
        }

        EndDrawing();
    }

    file_picker_destroy(&file_picker);
    pane_controller_terminate();
    cursor_terminate();
    ff_terminate();
    hlr_terminate();
    preview_terminate();
    return 0;
}
