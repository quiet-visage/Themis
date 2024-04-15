#include "line_editor.h"

#include <fieldfusion.h>
#include <raylib.h>
#include <string.h>

#include "../buffer/buffer.h"
#include "../commands.h"
#include "../config.h"
#include "../focus.h"
#include "../key_seq/key_seq.h"
#include "../keyboard.h"

struct line_editor_action_param {
    struct line_editor* m;
    struct ff_typography typo;
    Rectangle bounds;
};

static void line_editor_save_undo(struct line_editor* m) {
    buffer_save_undo(m->text.buffer, m->cursor);
}

static void line_editor_undo(struct line_editor_action_param* param) {
    if (!param->m->text.buffer->undo_history.length) return;
    struct text_position undo_cursor_pos =
        buffer_undo(param->m->text.buffer, param->m->cursor);
    if (undo_cursor_pos.row == (size_t)-1) return;
    param->m->cursor = undo_cursor_pos;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_redo(struct line_editor_action_param* param) {
    if (!param->m->text.buffer->redo_history.length) return;
    struct text_position redo_cursor_pos =
        buffer_redo(param->m->text.buffer, param->m->cursor);
    if (redo_cursor_pos.row == (size_t)-1) return;
    param->m->cursor = redo_cursor_pos;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_move_cursor_on_click(
    struct line_editor_action_param* param) {
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, param->bounds)) return;

    ulong hover_line = text_view_get_mouse_hover_line(
        &param->m->text, param->typo.size, mouse, param->bounds.y);
    ulong hover_col = text_view_get_mouse_hover_col(
        &param->m->text, param->typo, mouse, param->bounds.x,
        hover_line);

    param->m->cursor.row = hover_line;
    param->m->cursor.column = hover_col;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

void line_editor_create(struct line_editor* m) {
    memset(m, 0, sizeof(struct line_editor));
    m->text = text_view_create();
    m->line_editor_mode = line_editor_mode_normal;
}

void line_editor_destroy(struct line_editor* o) {
    text_view_destroy(&o->text);
}

static void line_editor_move_char_right(
    struct line_editor_action_param* param) {
    assert(param->m->text.buffer);
    if (param->m->cursor.column + 1 >
        param->m->text.buffer->str.length)
        return;
    param->m->cursor.column += 1;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void line_editor_move_beg_of_line(
    struct line_editor_action_param* param) {
    param->m->cursor.column = 0;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void line_editor_move_end_of_line(
    struct line_editor_action_param* param) {
    assert(param->m->text.buffer);
    param->m->cursor.column = param->m->text.buffer->str.length;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void line_editor_move_char_left(
    struct line_editor_action_param* param) {
    if (param->m->cursor.column > 0) param->m->cursor.column -= 1;
    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void line_editor_begin_mode_selection(
    struct line_editor_action_param* param) {
    param->m->line_editor_mode = line_editor_mode_selection;
    param->m->selection_begin = param->m->cursor.column;
}

static void line_editor_end_mode_selection(
    struct line_editor_action_param* param) {
    param->m->line_editor_mode = line_editor_mode_normal;
    text_view_clear_selection(&param->m->text);
}

static void line_editor_delete_selection(
    struct line_editor_action_param* param) {
    assert(param->m->line_editor_mode == line_editor_mode_selection);
    if (!(param->m->text.text_flags & text_flag_has_selection)) {
        line_editor_end_mode_selection(param);
    }

    line_editor_save_undo(param->m);

    param->m->cursor.row = param->m->text.selection.from_line;
    param->m->cursor.column = param->m->text.selection.from_col;

    struct line line_beg =
        param->m->text.buffer->lines
            .data[param->m->text.selection.from_line];
    struct line line_end =
        param->m->text.buffer->lines
            .data[param->m->text.selection.to_line];

    size_t beg_index =
        line_beg.start + param->m->text.selection.from_col;
    size_t end_index =
        line_end.start + param->m->text.selection.to_col;
    size_t count = end_index - beg_index;

    buffer_delete(param->m->text.buffer, beg_index, count);
    line_editor_end_mode_selection(param);
}

static void line_editor_delete_char_at(struct line_editor* m,
                                       size_t index) {
    assert(index <= m->text.buffer->str.length);
    buffer_delete(m->text.buffer, index, 1);
}

static void line_editor_backspace(
    struct line_editor_action_param* param) {
    assert(param->m->text.buffer);
    if (param->m->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(param);
        line_editor_end_mode_selection(param);
        return;
    }

    size_t cursor_position =
        param->m->text.buffer->lines.data[param->m->cursor.row]
            .start +
        param->m->cursor.column;
    if (!cursor_position) return;

    line_editor_delete_char_at(param->m, cursor_position - 1);
    line_editor_move_char_left(param);
}

static void line_editor_delete_rest_of_line(
    struct line_editor_action_param* param) {
    assert(param->m->text.buffer);
    line_editor_save_undo(param->m);

    if (param->m->text.text_flags & text_flag_has_selection)
        return line_editor_delete_selection(param);

    struct line line =
        param->m->text.buffer->lines.data[param->m->cursor.row];
    size_t start_index = line.start + param->m->cursor.column;
    size_t delete_len = line.end - start_index;

    buffer_delete(param->m->text.buffer, start_index, delete_len);
}

static void line_editor_delete(
    struct line_editor_action_param* param) {
    assert(param->m->text.buffer);

    if (param->m->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(param);
        return;
    }

    if (param->m->cursor.column ||
        param->m->cursor.column >= param->m->text.buffer->str.length)
        return;

    line_editor_delete_char_at(param->m, param->m->cursor.column);
}

static void line_editor_copy(struct line_editor_action_param* param) {
    assert(param->m->line_editor_mode == line_editor_mode_selection);
    size_t index_begin = param->m->text.selection.from_col;
    size_t index_end = param->m->text.selection.to_col;

    size_t count = index_end - index_begin;
    char utf8_selection_str[count + 1];
    utf8_selection_str[count] = 0;
    ff_utf32_to_utf8(utf8_selection_str,
                     &param->m->text.buffer->str.data[index_begin],
                     count);
    SetClipboardText(utf8_selection_str);
}

static void line_editor_paste(
    struct line_editor_action_param* param) {
    const char* clipboard_utf8 = GetClipboardText();
    if (!clipboard_utf8) return;

    size_t clipboard_utf8_len = strlen(clipboard_utf8);

    size_t chars_until_new_line = 0;
    for (size_t i = 0; i < clipboard_utf8_len; i += 1) {
        if (clipboard_utf8[i] == '\n') break;
        chars_until_new_line += 1;
    }
    clipboard_utf8_len = chars_until_new_line;

    c32_t clipboard_utf32[clipboard_utf8_len + 1];
    clipboard_utf32[clipboard_utf8_len] = 0;

    int err = ff_utf8_to_utf32(clipboard_utf32, clipboard_utf8,
                               clipboard_utf8_len);
    if (err) return;

    line_editor_save_undo(param->m);
    size_t cursor_index = param->m->cursor.column;
    buffer_insert_buf(param->m->text.buffer, cursor_index,
                      clipboard_utf32, clipboard_utf8_len);

    for (size_t i = 0; i < clipboard_utf8_len; i += 1) {
        line_editor_move_char_right(param);
    }
}

static void line_editor_cut(struct line_editor_action_param* param) {
    assert(param->m->line_editor_mode == line_editor_mode_selection);
    line_editor_copy(param);
    line_editor_delete_selection(param);
    line_editor_end_mode_selection(param);
}

static void line_editor_insert_char(
    struct line_editor_action_param* param, c32_t chr) {
    assert(param->m->text.buffer);
    if (chr == U' ' || chr == U'_' || chr == U'\n' ||
        param->m->line_editor_flags &
            line_editor_flag_cursor_moved_manually) {
        line_editor_save_undo(param->m);
    }

    buffer_insert_char(param->m->text.buffer, param->m->cursor.column,
                       chr);
    line_editor_move_char_right(param);
    param->m->line_editor_flags &=
        ~line_editor_flag_cursor_moved_manually;
}

static void line_editor_move_word_right(
    struct line_editor_action_param* param) {
    size_t idx = param->m->cursor.column;

    c32_t current_char = param->m->text.buffer->str.data[idx];
    for (c32_t current_char = param->m->text.buffer->str.data[idx];
         idx && current_char == U' '; idx += 1,
               current_char = param->m->text.buffer->str.data[idx])
        ;

    while (idx < param->m->text.buffer->str.length &&
           param->m->text.buffer->str.data[idx] != U' ') {
        idx += 1;
    }

    param->m->cursor.column = idx;

    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void line_editor_move_word_left(
    struct line_editor_action_param* param) {
    size_t idx = param->m->cursor.column;

    if (idx) idx -= 1;

    c32_t current_char = param->m->text.buffer->str.data[idx];
    for (c32_t current_char = param->m->text.buffer->str.data[idx];
         idx && current_char == U' '; idx -= 1,
               current_char = param->m->text.buffer->str.data[idx])
        ;

    while (idx && param->m->text.buffer->str.data[idx] != U' ') {
        idx -= 1;
    }

    if (param->m->text.buffer->str.data[idx] == U' ') idx += 1;

    param->m->cursor.column = idx;

    param->m->line_editor_flags |= line_editor_flag_cursor_moved;
    param->m->line_editor_flags |=
        line_editor_flag_cursor_moved_manually;
}

static void (*g_line_editor_cmd_table[line_editor_cmd_count])(
    struct line_editor_action_param*) = {
    [line_editor_cmd_move_char_left] = line_editor_move_char_left,
    [line_editor_cmd_move_char_right] = line_editor_move_char_right,
    [line_editor_cmd_move_end_of_line] = line_editor_move_end_of_line,
    [line_editor_cmd_move_beg_of_line] = line_editor_move_beg_of_line,
    [line_editor_cmd_copy] = line_editor_copy,
    [line_editor_cmd_paste] = line_editor_paste,
    [line_editor_cmd_cut] = line_editor_cut,
    [line_editor_cmd_begin_mode_selection] =
        line_editor_begin_mode_selection,
    [line_editor_cmd_end_mode_selection] =
        line_editor_end_mode_selection,
    [line_editor_cmd_delete] = line_editor_delete,
    [line_editor_cmd_backspace] = line_editor_backspace,
    [line_editor_cmd_undo] = line_editor_undo,
    [line_editor_cmd_redo] = line_editor_redo,
    [line_editor_cmd_delete_rest_of_line] =
        line_editor_delete_rest_of_line,
    [line_editor_cmd_move_word_left] = line_editor_move_word_left,
    [line_editor_cmd_move_word_right] = line_editor_move_word_right,
};

static void line_editor_handle_mode_normal(
    struct line_editor_action_param* param) {
    line_editor_move_cursor_on_click(param);

    int cmd = key_seq_handler_get_command(
        g_cfg.keybinds.line_editor.mode_normal);
    if (cmd != -1) g_line_editor_cmd_table[cmd](param);

    int char_pressed = get_char();
    if (!key_seq_handler_key_seq_active() && char_pressed)
        line_editor_insert_char(param, char_pressed);
}

static void line_editor_handle_mode_selection(
    struct line_editor_action_param* param) {
    line_editor_move_cursor_on_click(param);

    int command = key_seq_handler_get_command(
        g_cfg.keybinds.line_editor.mode_normal);
    if (command != -1) {
        void (*action_function)(struct line_editor_action_param*) =
            g_line_editor_cmd_table[command];
        if (action_function) action_function(param);
    }

    if (param->m->line_editor_flags & line_editor_flag_cursor_moved) {
        text_view_select(&param->m->text,
                         (struct selection){
                             .from_line = 0,
                             .from_col = param->m->selection_begin,
                             .to_line = 0,
                             .to_col = param->m->cursor.column,
                         });

        if (text_view_selection_is_invalid(&param->m->text)) {
            text_view_clear_selection(&param->m->text);
            line_editor_end_mode_selection(param);
            return;
        }
    }

    int char_pressed = get_char();
    if (!char_pressed) return;
    line_editor_delete_selection(param);
    line_editor_insert_char(param, char_pressed);
    line_editor_end_mode_selection(param);
}

void line_editor_handle_user_input(
    struct line_editor_action_param* param) {
    switch (param->m->line_editor_mode) {
        case line_editor_mode_normal:
            line_editor_handle_mode_normal(param);
            break;
        case line_editor_mode_selection:
            line_editor_handle_mode_selection(param);
            break;
    }
}

void line_editor_draw(struct line_editor* m,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags) {
    if (focus_flags & focus_flag_can_interact) {
        struct line_editor_action_param param = {m, typo, bounds};
        line_editor_handle_user_input(&param);
    }

    focus_flags &= ~focus_flag_can_scroll;
    text_view_draw_with_cursor(
        &m->text, typo, bounds, m->cursor,
        m->line_editor_flags & line_editor_flag_cursor_moved,
        focus_flags, 0, 0);

    m->line_editor_flags &= ~line_editor_flag_cursor_moved;
}

void line_editor_clear(struct line_editor* m) {
    buffer_clear(m->text.buffer);
    m->cursor.row = 0;
    m->cursor.column = 0;
}
