
#include <raylib.h>

#include "buffer/buffer_handler.h"
#include "buffer/buffer_picker.h"
#include "commands.h"
#include "compile.h"
#include "config.h"
#include "cursor.h"
#include "error_link.h"
#include "fieldfusion.h"
#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "highlighter/highlighter.h"
#include "keyboard.h"
#include "pane_controller.h"
#include "prompt.h"
#include "resources/resources.h"
#include "tools.hh"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_MIN_WIDTH 800
#define WIN_MIN_HEIGHT 600
#define WIN_TITLE "themis"
#define WIN_TARGET_FPS 60

static void override_keyboard_callbacks(void) {
    GLFWwindow* win = (GLFWwindow*)GetWindowHandle();
    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
}

namespace Flags {
enum : int {
    COMPILE_OPEN = (1 << 0),
};
}

namespace Focus_Flag {
enum : int {
    NONE = 0,
    CAN_INTERACT = 1,
    CAN_SCROLL = 1 << 1,
    ALL = CAN_INTERACT | CAN_SCROLL
};
}

struct Layout {
    Rectangle pane;
    Rectangle compile;
};

struct Focus {
    enum : int {
        FLAG_NONE = 0,
        FLAG_CAN_INTERACT = 1,
        FLAG_CAN_SCROLL = 1 << 1,
        FLAG_ALL = FLAG_CAN_INTERACT | FLAG_CAN_SCROLL
    };

    enum {
        SLOT_PANE_CONTROLLER = 0,
        SLOT_FILE_PICKER,
        SLOT_BUFFER_PICKER,
        SLOT_SET_CMD_PROMPT,
        SLOT_COUNT
    };

    int data[SLOT_COUNT] = {FLAG_ALL};

    inline void reset() { memset(data, 0, sizeof(data)); }
    inline void focus_slot(int slot) {
        reset();
        data[slot] = FLAG_ALL;
    }
};

struct Program {
    Vec2<float> win_size;
    Layout layout;
    int flags;
    prompt_t prompt;
    file_picker_t file_picker;
    Focus focus{};

    void operate_on_command(int command) {
        switch (command) {
            case main_cmd_open_file_picker: {
                focus.focus_slot(Focus::SLOT_FILE_PICKER);
            }
                return;
            case main_cmd_open_buffer_picker: {
                focus.focus_slot(Focus::SLOT_BUFFER_PICKER);
            }
                return;
            case main_cmd_close: {
                focus.focus_slot(Focus::SLOT_PANE_CONTROLLER);
            }
                return;
            case main_cmd_compile: {
                flags |= Flags::COMPILE_OPEN;
                calculate_layout();
                compile_spawn();
                pane_controller_update_bounds(layout.pane);
            }
                return;
            case main_cmd_compile_close: {
                flags &= ~Flags::COMPILE_OPEN;
                calculate_layout();
                pane_controller_update_bounds(layout.pane);
            }
                return;
            case main_cmd_compile_goto_next_error: {
                compile_jmp_next_err();
            }
                return;
            case main_cmd_compile_goto_prev_error: {
                compile_jmp_prev_err();
            }
                return;
            case main_cmd_open_set_cmd_prompt: {
                focus.focus_slot(Focus::SLOT_SET_CMD_PROMPT);
                prompt_clear(&prompt);
                c32_t* label = (int*)L"compilation command:";
                size_t label_len = 19;
                prompt_set_label(&prompt, label, label_len,
                                 g_cfg.typo);
            }
                return;
        }
        assert(0 && "unhandled command");
    }

    void operate_file_picker(void) {
        const char* result =
            file_picker_ui(&file_picker, g_cfg.typo,
                           focus.data[(int)Focus::SLOT_FILE_PICKER]);
        if (!result) return;
        pane_controller_open_in_focused(result);
        focus.focus_slot(Focus::SLOT_PANE_CONTROLLER);
    }

    void calculate_layout() {
        win_size = {(float)GetScreenWidth(),
                    (float)GetScreenHeight()};

        if (flags & Flags::COMPILE_OPEN) {
            layout.pane.x = 40;
            layout.pane.y = 20;
            layout.pane.width = win_size.x * .65 - 80.;
            layout.pane.height = win_size.y - 40.;
            layout.compile.x = layout.pane.width + layout.pane.x + 80;
            layout.compile.y = layout.pane.y;
            layout.compile.width = win_size.x * .35f - 80;
            layout.compile.height = layout.pane.height;
        }

        layout.pane.x = 40;
        layout.pane.y = 20;
        layout.pane.width = win_size.x - 80;
        layout.pane.height = win_size.y - 40;
    }

    void init() {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE);
        SetTargetFPS(WIN_TARGET_FPS);
        SetWindowMinSize(WIN_MIN_WIDTH, WIN_MIN_HEIGHT);

        override_keyboard_callbacks();
        ff_initialize("330");
        resources_init();
        hlr_init();
        cursor_initialize();
        preview_init();
        buffer_handler_init();
        buffer_picker_init();
        calculate_layout();
        pane_controller_init(layout.pane);
        config_init();
        prompt_create(&prompt);
        file_picker = file_picker_create();
        compile_init();
    }

    void operate_open_file_link(cmd_arg_t* cmd) {
        file_link_t* fl = (file_link_t*)cmd->arg;
        char* fname = (char*)alloca(fl->path_len + 1);
        fname[fl->path_len] = 0;
        ff_utf32_to_utf8(fname, fl->path, fl->path_len);
        pane_controller_open_in_focused(fname);
        file_editor_t* f_ed = pane_controller_get_focused();
        assert(f_ed);
        assert(fl->row >= 0);
        editor_move_cursor(&f_ed->editor, fl->row - 1, fl->column);
    }

    void begin_frame() {
        BeginDrawing();
        ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});
        key_seq_handler_begin_frame();
    }

    void end_frame(void) {
        kb_end_frame();
        key_seq_handler_end_frame();
        EndDrawing();
    }

    void loop() {
        while (!WindowShouldClose()) {
            begin_frame();
            ff_get_ortho_projection(0, GetScreenWidth(),
                                    GetScreenHeight(), 0.f, -1.f, 1.f,
                                    g_cfg.scr_proj);

            pane_controler_draw(
                g_cfg.typo, focus.data[Focus::SLOT_PANE_CONTROLLER]);
            if (flags & Flags::COMPILE_OPEN)
                compile_draw(g_cfg.typo, layout.compile,
                             Focus_Flag::CAN_SCROLL |
                                 Focus_Flag::CAN_INTERACT);

            if (focus.data[Focus::SLOT_FILE_PICKER] &
                Focus::FLAG_CAN_INTERACT)
                operate_file_picker();

            if (focus.data[Focus::SLOT_BUFFER_PICKER] &
                Focus::FLAG_CAN_INTERACT)
                operate_file_picker();

            if (focus.data[Focus::SLOT_SET_CMD_PROMPT] &
                Focus::FLAG_CAN_INTERACT) {
                prompt_result_t ret = prompt_render(&prompt);

                if (ret.str) {
                    char* res_str = (char*)alloca(ret.len + 1);
                    res_str[ret.len] = 0;
                    ff_utf32_to_utf8(res_str, ret.str, ret.len);
                    compile_set_cmd(res_str, strlen(res_str));
                }
            }

            if (IsWindowResized()) {
                calculate_layout();
                pane_controller_update_bounds(layout.pane);
            }

            int cmd =
                key_seq_handler_get_command(g_cfg.keybinds.main);
            if (cmd != -1) operate_on_command(cmd);

            cmd_arg_t cmd_arg = cmd_arg_get(command_group_main);
            if (cmd_arg.cmd == main_cmd_open_file_link) {
                operate_open_file_link(&cmd_arg);
            }
            cmd_arg_destroy(&cmd_arg);

            end_frame();
        }
    }

    void terminate() {
        compile_terminate();
        file_picker_destroy(&file_picker);
        prompt_destroy(&prompt);
        pane_controller_terminate();
        cursor_terminate();
        ff_terminate();
        hlr_terminate();
        preview_terminate();
        buffer_handler_terminate();
        buffer_picker_terminate();
        key_seq_handler_terminate();
        CloseWindow();
    }
};

int main(void) {
    static Program prog{};

    prog.init();
    prog.loop();
    prog.terminate();

    return 0;
}
