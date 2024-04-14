#include "pane_controller.h"

#include <fieldfusion.h>
#include <string.h>

#include "buffer/buffer_handler.h"
#include "buffer/buffer_syntax.h"
#include "commands.h"
#include "config.h"
#include "editor/editor.h"
#include "file_editor.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "key_seq/key_seq.h"
#include "raylib.h"
#include "text_view.h"
#include "tile_layout.h"

#define FILE_EDITOR_CAP 32

struct file_editor g_file_editors[FILE_EDITOR_CAP] = {0};
size_t g_file_editors_count = 0;
static size_t g_focused_num = 0;

struct file_editor* pane_controller_get_focused(void) {
    return &g_file_editors[g_focused_num];
}

void pane_controller_update_bounds(Rectangle bounds) {
    tile_set_root(bounds);
    tile_perform();
}

void pane_controller_init(Rectangle bounds) {
    pane_controller_update_bounds(bounds);
    file_editor_create(&g_file_editors[g_file_editors_count++]);
    struct buffer* scratch_buffer = buffer_handler_get("[scratch]");
    assert(scratch_buffer &&
           "scratch buffer should be created on buffer_handler_init");
    g_file_editors[g_file_editors_count - 1].editor.text.buffer =
        scratch_buffer;
}

static void pane_controller_split(enum split_kind split_kind) {
    assert(g_focused_num < tile_get_count());
    bool success = tile_split(split_kind, g_focused_num);
    if (!success) return;
    struct file_editor* old_file_editor =
        &g_file_editors[g_file_editors_count - 1];
    struct file_editor* new_file_editor =
        &g_file_editors[g_file_editors_count++];
    file_editor_create(new_file_editor);

    new_file_editor->editor.text.buffer =
        old_file_editor->editor.text.buffer;
    buffer_syntax_set_language(
        &new_file_editor->editor.text.buffer->syntax,
        old_file_editor->editor.text.buffer->syntax.highlighter
            .language);
    file_editor_set_path(new_file_editor,
                         old_file_editor->file_path.data);
}

static inline void pane_controller_focus_next(void) {
    size_t next_sorted_n = tile_get_sorted_n(g_focused_num) + 1;
    if (next_sorted_n >= g_file_editors_count) return;
    g_focused_num = tile_get_non_sorted_n(next_sorted_n);
}

static inline void pane_controller_focus_prev(void) {
    if (!g_focused_num) return;
    size_t prev_sorted_n = tile_get_sorted_n(g_focused_num) - 1;
    g_focused_num = tile_get_non_sorted_n(prev_sorted_n);
}

static void pane_controller_focus_right(void) {
    size_t right_n = tile_get_right_of(g_focused_num);
    if (right_n != (size_t)-1) g_focused_num = right_n;
}

static void pane_controller_focus_left(void) {
    size_t left_n = tile_get_left_of(g_focused_num);
    if (left_n != (size_t)-1) g_focused_num = left_n;
}

static void pane_controller_focus_up(void) {
    size_t up_n = tile_get_up_of(g_focused_num);
    if (up_n != (size_t)-1) g_focused_num = up_n;
}

static void pane_controller_focus_down(void) {
    size_t down_n = tile_get_down_of(g_focused_num);
    if (down_n != (size_t)-1) g_focused_num = down_n;
}

static void pane_controller_close_focused(void) {
    if (g_file_editors_count < 2) return;

    file_editor_destroy(&g_file_editors[g_focused_num]);

    memmove(&g_file_editors[g_focused_num],
            &g_file_editors[g_focused_num + 1],
            (g_file_editors_count - (g_focused_num + 1)) *
                sizeof(struct file_editor));

    pane_controller_focus_prev();
    tile_remove(g_focused_num);

    g_file_editors_count -= 1;
}

static void pane_controller_split_vertical(void) {
    pane_controller_split(split_vertical);
}

static void pane_controller_split_horizontal(void) {
    pane_controller_split(split_horizontal);
}

void (*g_pane_cmd_table[])(void) = {
    [pane_cmd_split_vertical] = pane_controller_split_vertical,
    [pane_cmd_split_horizontal] = pane_controller_split_horizontal,
    [pane_cmd_close_pane] = pane_controller_close_focused,
    [pane_cmd_focus_next] = pane_controller_focus_next,
    [pane_cmd_focus_prev] = pane_controller_focus_prev,
    [pane_cmd_focus_up] = pane_controller_focus_up,
    [pane_cmd_focus_down] = pane_controller_focus_down,
    [pane_cmd_focus_left] = pane_controller_focus_left,
    [pane_cmd_focus_right] = pane_controller_focus_right,
};

void pane_controler_draw(struct ff_typography typo, int focus_flags) {
    assert(g_file_editors_count == tile_get_count());

    if (focus_flags & focus_flag_can_interact) {
        int cmd = key_seq_handler_get_command(g_cfg.keybinds.pane);
        if (cmd != -1) g_pane_cmd_table[cmd]();
    }

    size_t rects_count = tile_get_count();
    Rectangle rects[rects_count];

    for (size_t i = 0; i < rects_count; i += 1) {
        rects[i] = tile_get_rect(i);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), rects[i])) {
            g_focused_num = i;
        }
    }

    for (size_t i = 0; i < rects_count; i += 1) {
        int flags = 0;

        if (focus_flags & focus_flag_can_interact) {
            flags |= i == g_focused_num ? focus_flag_can_interact : 0;
            flags |= focus_flag_can_scroll;
        }

        file_editor_draw(&g_file_editors[i], typo, rects[i], flags);
    }
}

void pane_controller_open_in_focused(const char* file_name) {
    assert(g_focused_num < g_file_editors_count);
    struct file_editor* focused = &g_file_editors[g_focused_num];
    editor_reset_mode(&focused->editor);
    file_editor_open(focused, file_name);
}

void pane_controller_terminate(void) {
    for (size_t i = 0; i < g_file_editors_count; i += 1) {
        file_editor_destroy(&g_file_editors[i]);
        g_file_editors_count -= 1;
    }
}

void pane_controller_set_focused_buffer(struct buffer* buffer) {
    assert(buffer);
    struct file_editor* focused_file_editor =
        pane_controller_get_focused();
    focused_file_editor->editor.text.buffer = buffer;

    memset(&focused_file_editor->editor.cursor, 0,
           sizeof(struct text_position));
    file_editor_set_path(focused_file_editor, buffer->buffer_name);
}
