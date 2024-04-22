#include <assert.h>
#include <raylib.h>

#include "buffer/buffer_handler.h"
#include "buffer/buffer_picker.h"
#include "commands.h"
#include "compile.h"
#include "config.h"
#include "editor/editor.h"
#include "error_link.h"
#include "fieldfusion.h"
#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"
#include "keyboard.h"
#include "pane_controller.h"
#include "prompt.h"
#include "resources/resources.h"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_MIN_WIDTH 800
#define WIN_MIN_HEIGHT 600
#define WIN_TITLE "themis"
#define WIN_TARGET_FPS 60

#define FOCUS_ALL_OPTS focus_flag_can_interact | focus_flag_can_scroll

enum focus_slot {
    e_pane_controller,
    e_file_picker,
    e_buffer_picker,
    e_set_cmd_prompt,
};

typedef struct {
    Rectangle pane;
    Rectangle compile;
} window_partitions_t;

static int g_focus[] = {
    [e_pane_controller] = FOCUS_ALL_OPTS,
    [e_file_picker] = 0,
    [e_buffer_picker] = 0,
    [e_set_cmd_prompt] = 0,
};
static bool g_compile_open = {0};
static window_partitions_t g_partitions = {0};
static file_picker_t g_file_picker = {0};
static prompt_t g_prompt = {0};

static void calculate_areas(void) {
    float win_width = GetScreenWidth();
    float win_height = GetScreenHeight();
    if (g_compile_open) {
        g_partitions.pane.x = 40;
        g_partitions.pane.y = 20;
        g_partitions.pane.width = win_width * 0.65f - 80;
        g_partitions.pane.height = win_height - 40;

        g_partitions.compile.x =
            g_partitions.pane.width + g_partitions.pane.x + 80;
        g_partitions.compile.y = g_partitions.pane.y;
        g_partitions.compile.width = win_width * .35f - 80;
        g_partitions.compile.height = g_partitions.pane.height;
        return;
    }
    g_partitions.pane.x = 40;
    g_partitions.pane.y = 20;
    g_partitions.pane.width = win_width - 80;
    g_partitions.pane.height = win_height - 40;
}

static void focus_reset(void) { memset(g_focus, 0, sizeof(g_focus)); }

static void focus_file_picker(void) {
    focus_reset();
    g_focus[e_file_picker] = FOCUS_ALL_OPTS;
}

static void focus_buffer_picker(void) {
    buffer_picker_update_options();
    focus_reset();
    g_focus[e_buffer_picker] = FOCUS_ALL_OPTS;
}

static void focus_pane_controller(void) {
    focus_reset();
    g_focus[e_pane_controller] = FOCUS_ALL_OPTS;
}

static void perform_build(void) {
    g_compile_open = 1;
    calculate_areas();
    compile_spawn();
    pane_controller_update_bounds(g_partitions.pane);
}

static void close_compile(void) {
    g_compile_open = 0;
    calculate_areas();
    pane_controller_update_bounds(g_partitions.pane);
}

static void focus_prompt(void) {
    focus_reset();
    prompt_clear(&g_prompt);
    static const c32_t label[] = L"compilation command:";
    size_t label_len = 19;
    prompt_set_label(&g_prompt, label, label_len, g_cfg.typo);
    g_focus[e_set_cmd_prompt] = FOCUS_ALL_OPTS;
}

void (*g_command_table[])(void) = {
    [main_cmd_open_file_picker] = focus_file_picker,
    [main_cmd_open_buffer_picker] = focus_buffer_picker,
    [main_cmd_close] = focus_pane_controller,
    [main_cmd_compile] = perform_build,
    [main_cmd_compile_close] = close_compile,
    [main_cmd_compile_goto_next_error] = compile_jmp_next_err,
    [main_cmd_compile_goto_prev_error] = compile_jmp_prev_err,
    [main_cmd_open_set_cmd_prompt] = focus_prompt,
};

void override_keyboard_callbacks(void) {
    GLFWwindow* win = GetWindowHandle();
    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
}

static void main_init(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE);
    SetTargetFPS(WIN_TARGET_FPS);
    SetWindowMinSize(WIN_MIN_WIDTH, WIN_MIN_HEIGHT);
    override_keyboard_callbacks();

    ff_initialize("430");
    resources_init();
    hlr_init();
    cursor_initialize();
    preview_init();
    buffer_handler_init();
    buffer_picker_init();
    calculate_areas();
    pane_controller_init(g_partitions.pane);
    compile_init();
    config_init();

    prompt_create(&g_prompt);
    g_file_picker = file_picker_create();
}

static void main_terminate(void) {
    compile_terminate();
    file_picker_destroy(&g_file_picker);
    prompt_destroy(&g_prompt);
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

static void main_begin_frame(void) {
    BeginDrawing();
    ClearBackground((Color){0x1e, 0x1e, 0x2e, 0xff});
    key_seq_handler_begin_frame();
}

static void main_end_frame(void) {
    kb_end_frame();
    key_seq_handler_end_frame();
    EndDrawing();
}

static void handle_file_picker(void) {
    const char* result = file_picker_ui(&g_file_picker, g_cfg.typo,
                                        g_focus[e_file_picker]);
    if (!result) return;
    pane_controller_open_in_focused(result);
    focus_pane_controller();
}

static void handle_buffer_picker(void) {
    buffer_t* buffer_picked =
        buffer_picker_ui(g_cfg.typo, g_focus[e_buffer_picker]);

    if (!buffer_picked) return;
    pane_controller_set_focused_buffer(buffer_picked);
    focus_pane_controller();
}

static void handle_open_file_link(cmd_arg_t* cmd) {
    file_link_t* fl = cmd->arg;
    char fname[fl->path_len + 1];
    fname[fl->path_len] = 0;
    ff_utf32_to_utf8(fname, fl->path, fl->path_len);
    pane_controller_open_in_focused(fname);
    file_editor_t* f_ed = pane_controller_get_focused();
    assert(f_ed);
    assert(fl->row >= 0);
    editor_move_cursor(&f_ed->editor, fl->row - 1, fl->column);
}

int main(void) {
    static_assert(WIN_WIDTH > WIN_MIN_WIDTH,
                  "window width must be greater or equal to the "
                  "window minimum width");
    static_assert(WIN_HEIGHT > WIN_MIN_HEIGHT,
                  "window height must be greater or equal to the "
                  "window minimum height");

    main_init();

    focus_pane_controller();
    compile_set_cmd("make", 4);

    while (!WindowShouldClose()) {
        main_begin_frame();
        ff_get_ortho_projection(0, GetScreenWidth(),
                                GetScreenHeight(), 0.f, -1.f, 1.f,
                                g_cfg.scr_proj);

        pane_controler_draw(g_cfg.typo, g_focus[e_pane_controller]);
        if (g_compile_open)
            compile_draw(
                g_cfg.typo, g_partitions.compile,
                focus_flag_can_scroll | focus_flag_can_interact);

        if (g_focus[e_file_picker] & focus_flag_can_interact)
            handle_file_picker();

        if (g_focus[e_buffer_picker] & focus_flag_can_interact)
            handle_buffer_picker();

        if (g_focus[e_set_cmd_prompt] & focus_flag_can_interact) {
            prompt_result_t ret = prompt_render(&g_prompt);

            if (ret.str) {
                char res_str[ret.len + 1];
                res_str[ret.len] = 0;
                ff_utf32_to_utf8(res_str, ret.str, ret.len);
                compile_set_cmd(res_str, strlen(res_str));
            }
        }

        if (IsWindowResized()) {
            calculate_areas();
            pane_controller_update_bounds(g_partitions.pane);
        }

        int cmd = key_seq_handler_get_command(g_cfg.keybinds.main);
        if (cmd != -1) g_command_table[cmd]();

        cmd_arg_t cmd_arg = cmd_arg_get(command_group_main);
        if (cmd_arg.cmd == main_cmd_open_file_link) {
            handle_open_file_link(&cmd_arg);
        }
        cmd_arg_destroy(&cmd_arg);

        main_end_frame();
    }

    main_terminate();
    return 0;
}
