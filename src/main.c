#include <raylib.h>
#include <string.h>
#include <uchar.h>

#include "buffer/buffer_handler.h"
#include "buffer/buffer_picker.h"
#include "commands.h"
#include "config.h"
#include "dyn_strings/utf32_string.h"
#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"
#include "key_seq/sequence_association.h"
#include "keyboard.h"
#include "pane_controller.h"
#include "resources/resources.h"
#include "file_editor.h"
#include "tile_layout.h"

struct focus {
    int picker_flags;
    int pane_flags;
    int buffer_picker_flags;
};

static void focus_file_picker(struct focus* focus) {
    focus->picker_flags =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->pane_flags = 0;
}

static void focus_buffer_picker(struct focus* focus) {
    buffer_picker_update();
    focus->picker_flags = 0;
    focus->buffer_picker_flags =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->pane_flags = 0;
}

static void focus_pane_controller(struct focus* focus) {
    focus->pane_flags =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->picker_flags = 0;
    focus->buffer_picker_flags = 0;
}

void (*g_command_table[])(struct focus*) = {
    [main_cmd_open_file_picker] = focus_file_picker,
    [main_cmd_open_buffer_picker] = focus_buffer_picker,
    [main_cmd_close] = focus_pane_controller,
};

void override_keyboard_callbacks(void) {
    GLFWwindow* win = GetWindowHandle();
    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
}

int main(void) {
    InitWindow(800, 800, "Themis");
    SetTargetFPS(60);
    SetWindowMinSize(800, 800);
    override_keyboard_callbacks();

    ff_initialize("430");
    resources_init();
    hlr_init();
    cursor_initialize();
    preview_init();
    buffer_handler_init();
    buffer_picker_init();
    buffer_picker_update();
    pane_controller_init(
        (Rectangle){.x = 100,
                    .y = 10,
                    .width = GetScreenWidth() - 200,
                    .height = GetScreenHeight() - 20});
    config_init_default_keybinds();

    struct file_picker file_picker = file_picker_create();

    struct ff_typography typo = {
        .font = 0, .size = 14.f, .color = 0xffffffff};

    struct focus focus = {0};

    focus_pane_controller(&focus);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});
        key_seq_handler_begin_frame();

        pane_controler_draw(typo, focus.pane_flags);

        if (focus.picker_flags & focus_flag_can_interact) {
            const char* result = file_picker_perform(
                &file_picker, typo, focus.picker_flags);
            if (IsKeyPressed(KEY_ESCAPE)) {
                focus_pane_controller(&focus);
            }
            if (result) {
                pane_controller_open_in_focused(result);
                focus_pane_controller(&focus);
            }
        }

        if (focus.buffer_picker_flags & focus_flag_can_interact) {
            struct buffer* buffer_picked = buffer_picker_perform(
                typo, focus.buffer_picker_flags);

            if (IsKeyPressed(KEY_ESCAPE)) {
                focus_pane_controller(&focus);
            }

            if (buffer_picked) {
                pane_controller_set_focused_buffer(buffer_picked);
                focus_pane_controller(&focus);
            }
        }

        if (IsWindowResized()) {
            pane_controller_update_bounds(
                (Rectangle){.x = 100,
                            .y = 10,
                            .width = GetScreenWidth() - 200,
                            .height = GetScreenHeight() - 20});
        }

        int cmd = key_seq_handler_get_command(g_cfg.keybinds.main);
        if (cmd != -1) {
            g_command_table[cmd](&focus);
        }

        kb_end_frame();
        key_seq_handler_end_frame();
        EndDrawing();
    }

    file_picker_destroy(&file_picker);
    pane_controller_terminate();
    cursor_terminate();
    ff_terminate();
    hlr_terminate();
    preview_terminate();
    buffer_handler_terminate();
    buffer_picker_terminate();
    key_seq_handler_terminate();
    CloseWindow();

    return 0;
}
