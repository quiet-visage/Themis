#include "pane_controller.h"

#include <stdlib.h>

#include "field_fusion/fieldfusion.h"
#include "focus.h"
#include "raylib.h"
#include "text/file_editor.h"
#include "tile_layout.h"

#define FILE_EDITOR_CAP 32

struct file_editor g_file_editors[FILE_EDITOR_CAP] = {0};
size_t g_file_editors_count = 0;
static size_t g_focused_num = 0;

void pane_controller_update_bounds(Rectangle bounds) {
    tile_set_root(bounds);
    tile_perform();
}

void pane_controller_init(Rectangle bounds) {
    pane_controller_update_bounds(bounds);
    g_file_editors[g_file_editors_count++] = file_editor_create();
}

static void pane_controller_split(enum split_kind split_kind) {
    assert(g_focused_num < tile_get_count());
    bool success = tile_split(split_kind, g_focused_num);
    if (success)
        g_file_editors[g_file_editors_count++] = file_editor_create();
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

static void pane_controller_close_focused(void) {
    if (g_file_editors_count == 1) return;

    if (g_focused_num == g_file_editors_count - 1) {
        file_editor_destroy(&g_file_editors[g_focused_num]);
        g_file_editors_count -= 1;
        pane_controller_focus_prev();
        tile_remove(g_focused_num);
        return;
    }

    file_editor_destroy(&g_file_editors[g_focused_num]);
    memmove(&g_file_editors[g_focused_num],
            &g_file_editors[g_focused_num + 1],
            sizeof(struct file_editor) *
                (g_file_editors_count - g_focused_num));
    g_file_editors_count -= 1;

    if (tile_get_sorted_n(g_focused_num) == tile_get_count() - 1) {
        tile_remove(g_focused_num);
        pane_controller_focus_prev();
        g_focused_num += 1;
    } else {
        tile_remove(g_focused_num);
        pane_controller_focus_prev();
    }
}

void pane_controller_keybinds(void) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (IsKeyPressed(KEY_THREE)) {
            pane_controller_split(split_horizontal);
        } else if (IsKeyPressed(KEY_FOUR)) {
            pane_controller_split(split_vertical);
        } else if (IsKeyPressed(KEY_RIGHT)) {
            pane_controller_focus_next();
        } else if (IsKeyPressed(KEY_LEFT)) {
            pane_controller_focus_prev();
        } else if (IsKeyPressed(KEY_ZERO)) {
            pane_controller_close_focused();
        }
    }
}

void pane_controler_draw(struct ff_typography typo, int focus_flags) {
    if (focus_flags & focus_flag_can_interact) {
        pane_controller_keybinds();
    }

    assert(g_file_editors_count == tile_get_count());

    for (size_t i = 0; i < tile_get_count(); i += 1) {
        Rectangle rec = tile_get_rect(i);
        // DrawRectangleLinesEx(rec, 1.0f,
        //                      i == g_focused_num ? PINK : WHITE);
        // char name[32] = {0};
        // snprintf(name, 32, "%ld", i);
        // DrawText(name, rec.x + 4, rec.y + 4, 14, GREEN);
        // snprintf(name, 32, "%ld", tile_get_sorted_n(i));
        // DrawText(name, rec.x + 26, rec.y + 4, 14, MAGENTA);

        int flags = 0;

        if (focus_flags & focus_flag_can_interact) {
            flags |= i == g_focused_num ? focus_flag_can_interact : 0;
            flags |= focus_flag_can_scroll;
        }

        file_editor_draw(&g_file_editors[i], typo, rec, flags);
    }
}

void pane_controller_open_in_focused(const char* file_name) {
    assert(g_focused_num < g_file_editors_count);
    file_editor_open(&g_file_editors[g_focused_num], file_name);
}

void pane_controller_terminate(void) {
    for (size_t i = 0; i < g_file_editors_count; i += 1) {
        file_editor_destroy(&g_file_editors[i]);
        g_file_editors_count -= 1;
    }
}
