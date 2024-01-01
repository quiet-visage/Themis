#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <string.h>
#include <text/file_editor.h>
#include <text/text.h>
#include <uchar.h>

#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "highlighter/highlighter.h"
#include "menu/fuzzy_menu.h"
#include "resources/resources.h"
#include "text/editor.h"

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
    SetTargetFPS(60);

    ff_initialize("430");
    int maple_mono =
        ff_new_font("/usr/share/fonts/TTF/mplus-2m-regular.ttf",
                    ff_default_font_config());

    struct file_editor fe = file_editor_create();
    file_editor_open(&fe, __FILE__);

    SetExitKey(0);

    struct file_picker fp = file_picker_create();
    struct ff_typography typo = {
        .font = maple_mono, .size = 14.f, .color = 0xffffffff};

    struct focus focus = {0};
    open_file_picker(&focus);

    preview_init();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});

        file_editor_draw(&fe, typo, (Rectangle){GetScreenWidth() * 0.5f - (GetScreenWidth() * 0.7f) * 0.5f, 0, GetScreenWidth()*0.7, GetScreenHeight()},
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

        EndDrawing();
    }

    file_picker_destroy(&fp);
    file_editor_destroy(&fe);

    ff_terminate();
    hlr_terminate();

    preview_terminate();
    return 0;
}
