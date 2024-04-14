#include <assert.h>
#include <raylib.h>

#include "buffer/buffer_handler.h"
#include "buffer/buffer_picker.h"
#include "commands.h"
#include "compile.h"
#include "config.h"
#include "error_link.h"
#include "fieldfusion.h"
#include "file_picker/file_picker.h"
#include "file_picker/file_preview.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"
#include "keyboard.h"
#include "pane_controller.h"
#include "resources/resources.h"

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720
#define WIN_MIN_WIDTH 800
#define WIN_MIN_HEIGHT 600
#define WIN_TITLE "themis"
#define WIN_TARGET_FPS 60

struct focus {
    int file_picker;
    int pane_controller;
    int buffer_picker;
};

struct area {
    Rectangle pane;
    Rectangle compile;
};

static bool g_compile_open = {0};
static struct area g_area = {0};
struct focus g_focus = {0};
static struct file_picker g_file_picker = {0};

static void calculate_areas(void) {
    float win_width = GetScreenWidth();
    float win_height = GetScreenHeight();
    if (g_compile_open) {
        g_area.pane.x = 40;
        g_area.pane.y = 20;
        g_area.pane.width = win_width * 0.65f - 80;
        g_area.pane.height = win_height - 40;

        g_area.compile.x = g_area.pane.width + g_area.pane.x + 80;
        g_area.compile.y = g_area.pane.y;
        g_area.compile.width = win_width * .35f - 80;
        g_area.compile.height = g_area.pane.height;
        return;
    }
    g_area.pane.x = 40;
    g_area.pane.y = 20;
    g_area.pane.width = win_width - 80;
    g_area.pane.height = win_height - 40;
}

static void focus_file_picker(struct focus* focus) {
    focus->file_picker =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->pane_controller = 0;
}

static void focus_buffer_picker(struct focus* focus) {
    buffer_picker_update();
    focus->file_picker = 0;
    focus->buffer_picker =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->pane_controller = 0;
}

static void focus_pane_controller(struct focus* focus) {
    focus->pane_controller =
        focus_flag_can_interact | focus_flag_can_scroll;
    focus->file_picker = 0;
    focus->buffer_picker = 0;
}

static void perform_build(struct focus* focus) {
    g_compile_open = 1;
    calculate_areas();
    compile_spawn();
    pane_controller_update_bounds(g_area.pane);
}

static void close_compile(struct focus*) {
    g_compile_open = 0;
    calculate_areas();
    pane_controller_update_bounds(g_area.pane);
}

void (*g_command_table[])(struct focus*) = {
    [main_cmd_open_file_picker] = focus_file_picker,
    [main_cmd_open_buffer_picker] = focus_buffer_picker,
    [main_cmd_close] = focus_pane_controller,
    [main_cmd_compile] = perform_build,
    [main_cmd_compile_close] = close_compile,
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
    pane_controller_init(g_area.pane);
    compile_init();
    config_init();

    g_file_picker = file_picker_create();
}

static void main_terminate(void) {
    compile_terminate();
    file_picker_destroy(&g_file_picker);
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
    const char* result = file_picker_perform(
        &g_file_picker, g_cfg.typo, g_focus.file_picker);
    if (!result) return;
    pane_controller_open_in_focused(result);
    focus_pane_controller(&g_focus);
}

static void handle_buffer_picker(void) {
    struct buffer* buffer_picked =
        buffer_picker_perform(g_cfg.typo, g_focus.buffer_picker);

    if (!buffer_picked) return;
    pane_controller_set_focused_buffer(buffer_picked);
    focus_pane_controller(&g_focus);
}

static void handle_open_file_link(struct cmd_arg *cmd) {
    struct file_link* fl = cmd->arg;
    char fname[fl->path_len + 1];
    fname[fl->path_len] = 0;
    ff_utf32_to_utf8(fname, fl->path, fl->path_len);
    pane_controller_open_in_focused(fname);
    struct file_editor* ed = pane_controller_get_focused();
    assert(ed);
    ed->editor.cursor.column = fl->column;
    assert(fl->row >= 0);
    ed->editor.cursor.row = fl->row-1;
}

int main(void) {
    static_assert(WIN_WIDTH > WIN_MIN_WIDTH,
                  "window width must be greater or equal to the "
                  "window minimum width");
    static_assert(WIN_HEIGHT > WIN_MIN_HEIGHT,
                  "window height must be greater or equal to the "
                  "window minimum height");

    main_init();

    focus_pane_controller(&g_focus);
    compile_set_cmd("make", 4);

    while (!WindowShouldClose()) {
        main_begin_frame();

        pane_controler_draw(g_cfg.typo, g_focus.pane_controller);
        if (g_compile_open)
            compile_draw(
                g_cfg.typo, g_area.compile,
                focus_flag_can_scroll | focus_flag_can_interact);

        if (g_focus.file_picker & focus_flag_can_interact)
            handle_file_picker();

        if (g_focus.buffer_picker & focus_flag_can_interact)
            handle_buffer_picker();

        if (IsWindowResized()) {
            calculate_areas();
            pane_controller_update_bounds(g_area.pane);
        }

        int cmd = key_seq_handler_get_command(g_cfg.keybinds.main);
        if (cmd != -1) g_command_table[cmd](&g_focus);

        struct cmd_arg cmd_arg = cmd_arg_get(command_group_main);
        if (cmd_arg.cmd == main_cmd_open_file_link) {
            handle_open_file_link(&cmd_arg);
        }
        cmd_arg_destroy(&cmd_arg);

        main_end_frame();
    }

    main_terminate();
    return 0;
}
