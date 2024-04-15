#include "editor.h"

#include "../commands.h"
#include "../config.h"
#include "../focus.h"
#include "../key_seq/key_seq.h"
#include "../keyboard.h"
#include "line_editor.h"

struct editor_action_param {
    struct editor* m;
    struct ff_typography typo;
    Rectangle bounds;
};

static void editor_move_char_right(struct editor_action_param* param);
static void editor_move_line_down(struct editor_action_param* param);
static void editor_move_char_left(struct editor_action_param* param);
static void editor_move_end_of_line(
    struct editor_action_param* param);
static void editor_end_mode_selection(
    struct editor_action_param* param);

void editor_save_undo(struct editor* i) {
    buffer_save_undo(i->text.buffer, i->cursor);
}

static void editor_undo(struct editor_action_param* param) {
    if (!param->m->text.buffer->undo_history.length) return;
    struct text_position undo_cursor_pos =
        buffer_undo(param->m->text.buffer, param->m->cursor);
    if (undo_cursor_pos.row == (size_t)-1) return;
    param->m->cursor = undo_cursor_pos;
    param->m->editor_flags |= editor_flag_cursor_moved;
}

static void editor_redo(struct editor_action_param* param) {
    if (!param->m->text.buffer->redo_history.length) return;
    struct text_position redo_cursor_pos =
        buffer_redo(param->m->text.buffer, param->m->cursor);
    if (redo_cursor_pos.row == (size_t)-1) return;
    param->m->cursor = redo_cursor_pos;
    param->m->editor_flags |= editor_flag_cursor_moved;
}

static void editor_ensure_cursor_idx_within_str(struct editor* m) {
    if (m->cursor.row >= m->text.buffer->lines.length) {
        m->cursor.row = m->text.buffer->lines.length - 1;
        m->editor_flags |= editor_flag_cursor_moved;
        m->editor_flags |= editor_flag_cursor_moved_manually;
    }

    struct line* cursor_line =
        &m->text.buffer->lines.data[m->cursor.row];
    size_t cursor_line_len = line_len(cursor_line);

    if (m->cursor.column > cursor_line_len) {
        m->cursor.column = cursor_line_len;
        m->editor_flags |= editor_flag_cursor_moved;
        m->editor_flags |= editor_flag_cursor_moved_manually;
    }
}

void editor_create(struct editor* m) {
    m->text = text_view_create();
    search_mod_create(&m->search_mod);
}

static void editor_copy(struct editor_action_param* param) {
    assert(param->m->editor_mode == editor_mode_selection);

    size_t index_begin = param->m->text.buffer->lines
                             .data[param->m->text.selection.from_line]
                             .start +
                         param->m->text.selection.from_col;
    size_t index_end = param->m->text.buffer->lines
                           .data[param->m->text.selection.to_line]
                           .start +
                       param->m->text.selection.to_col;

    size_t count = index_end - index_begin;
    char utf8_selection_str[count + 1];
    utf8_selection_str[count] = 0;
    ff_utf32_to_utf8(utf8_selection_str,
                     &param->m->text.buffer->str.data[index_begin],
                     count);

    SetClipboardText(utf8_selection_str);
    editor_end_mode_selection(param);
}

static void editor_paste(struct editor_action_param* param) {
    const char* clipboard_utf8 = GetClipboardText();
    if (!clipboard_utf8) return;

    size_t clipboard_utf8_len = strlen(clipboard_utf8);
    c32_t clipboard_utf32[clipboard_utf8_len + 1];
    clipboard_utf32[clipboard_utf8_len] = 0;

    size_t ret = ff_utf8_to_utf32(clipboard_utf32, clipboard_utf8,
                                  clipboard_utf8_len);
    if (ret == (size_t)-1) return;

    editor_save_undo(param->m);
    size_t cursor_index =
        param->m->text.buffer->lines.data[param->m->cursor.row]
            .start +
        param->m->cursor.column;
    buffer_insert_buf(param->m->text.buffer, cursor_index,
                      clipboard_utf32, clipboard_utf8_len);

    for (size_t i = 0; i < clipboard_utf8_len; i += 1) {
        editor_move_char_right(param);
    }
}

static void editor_delete_selection(
    struct editor_action_param* param) {
    if (!(param->m->text.text_flags & text_flag_has_selection)) {
        editor_end_mode_selection(param);
    }

    editor_save_undo(param->m);

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
    editor_end_mode_selection(param);
}

static void editor_cut(struct editor_action_param* param) {
    assert(param->m->editor_mode == editor_mode_selection);
    editor_copy(param);
    editor_delete_selection(param);
    editor_end_mode_selection(param);
}

void editor_destroy(struct editor* m) {
    text_view_destroy(&m->text);
    search_mod_destroy(&m->search_mod);
}

static void editor_backspace(struct editor_action_param* param) {
    if (param->m->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(param);
    if (param->m->editor_mode & editor_mode_selection)
        editor_end_mode_selection(param);

    size_t cursor_position =
        param->m->text.buffer->lines.data[param->m->cursor.row]
            .start +
        param->m->cursor.column;
    if (cursor_position == 0) return;

    buffer_delete(param->m->text.buffer, cursor_position - 1, 1);
    editor_move_char_left(param);
}

static void editor_delete(struct editor_action_param* param) {
    editor_save_undo(param->m);

    if (param->m->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(param);
    size_t cursor_position =
        param->m->text.buffer->lines.data[param->m->cursor.row]
            .start +
        param->m->cursor.column;
    if (cursor_position >= param->m->text.buffer->str.length) return;
    buffer_delete(param->m->text.buffer, cursor_position, 1);
}

static void editor_insert_char(struct editor_action_param* param,
                               c32_t chr) {
    if (chr == U' ' || chr == U'_' || chr == U'\n' ||
        param->m->editor_flags & editor_flag_cursor_moved_manually) {
        editor_save_undo(param->m);
    }
    buffer_insert_char(
        param->m->text.buffer,
        param->m->text.buffer->lines.data[param->m->cursor.row]
                .start +
            param->m->cursor.column,
        chr);
    editor_move_char_right(param);
    param->m->editor_flags &= ~editor_flag_cursor_moved_manually;
}

static void editor_insert_tab(struct editor_action_param* param,
                              uint8_t tab_size) {
    editor_save_undo(param->m);

    c32_t tab[tab_size];
    for (size_t i = 0; i < tab_size; i += 1) tab[i] = U' ';
    buffer_insert_buf(
        param->m->text.buffer,
        param->m->text.buffer->lines.data[param->m->cursor.row]
                .start +
            param->m->cursor.column,
        tab, tab_size);
    for (size_t i = 0; i < tab_size; i += 1)
        editor_move_char_right(param);
}

static void editor_move_char_right(
    struct editor_action_param* param) {
    struct line line =
        param->m->text.buffer->lines.data[param->m->cursor.row];
    if (param->m->cursor.column >= line_len(&line))
        return editor_move_line_down(param);
    param->m->cursor.column += 1;
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_line_up(struct editor_action_param* param) {
    if (param->m->cursor.row == 0)
        return editor_move_char_left(param);
    param->m->cursor.column = 0;
    param->m->cursor.row -= 1;
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_char_left(struct editor_action_param* param) {
    if (param->m->cursor.column == 0) {
        if (param->m->cursor.row == 0) return;
        editor_move_line_up(param);
        return editor_move_end_of_line(param);
    }
    param->m->cursor.column -= 1;
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_beg_of_line(
    struct editor_action_param* param) {
    param->m->cursor.column = 0;
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_end_of_line(
    struct editor_action_param* param) {
    struct line line =
        param->m->text.buffer->lines.data[param->m->cursor.row];
    param->m->cursor.column = line_len(&line);
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_line_down(struct editor_action_param* param) {
    if (param->m->cursor.row >=
        param->m->text.buffer->lines.length - 1)
        return;
    param->m->cursor.column = 0;
    param->m->cursor.row += 1;
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_begin_mode_selection(
    struct editor_action_param* param) {
    param->m->editor_mode = editor_mode_selection;
    param->m->selection_begin = param->m->cursor;
}

static void editor_end_mode_selection(
    struct editor_action_param* param) {
    param->m->editor_mode = editor_mode_normal;
    text_view_clear_selection(&param->m->text);
}

static void editor_end_mode_search(
    struct editor_action_param* param) {
    param->m->editor_mode = editor_mode_normal;
}

static void editor_begin_mode_search(
    struct editor_action_param* param) {
    param->m->editor_mode = editor_mode_search;
}

static void editor_select_first_search_match(
    struct editor_action_param* param) {
    if (search_mod_is_empty(&param->m->search_mod)) return;

    struct selection* selected_match =
        search_mod_get_selected_match(&param->m->search_mod);
    if (!selected_match) return;
    param->m->cursor.row = selected_match->from_line;
    param->m->cursor.column = selected_match->from_col;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_select_next_search_match(
    struct editor_action_param* param) {
    search_mod_select_next(&param->m->search_mod);

    struct selection* selected_match =
        search_mod_get_selected_match(&param->m->search_mod);
    if (!selected_match) return;
    param->m->cursor.row = selected_match->from_line;
    param->m->cursor.column = selected_match->from_col;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_select_previous_search_match(
    struct editor_action_param* param) {
    search_mod_select_prev(&param->m->search_mod);

    struct selection* selected_match =
        search_mod_get_selected_match(&param->m->search_mod);
    if (!selected_match) return;
    param->m->cursor.row = selected_match->from_line;
    param->m->cursor.column = selected_match->from_col;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_delete_rest_of_line(
    struct editor_action_param* param) {
    editor_save_undo(param->m);
    if (param->m->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(param);

    struct line line =
        param->m->text.buffer->lines.data[param->m->cursor.row];
    size_t start_index = line.start + param->m->cursor.column;
    size_t delete_len = line.end - start_index;

    buffer_delete(param->m->text.buffer, start_index, delete_len);
}

static void editor_move_cursor_on_click(
    struct editor_action_param* param) {
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
    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static size_t editor_get_cursor_idx(struct editor* m) {
    assert(m->cursor.row < m->text.buffer->lines.length);

    const struct line* line =
        &m->text.buffer->lines.data[m->cursor.row];
    assert(m->cursor.column <= line->start - line->end);

    return line->start + m->cursor.column;
}

static inline bool is_word_separator(c32_t chr) {
    return chr == U' ' || chr == U'\n';
}

static void editor_move_word_right(
    struct editor_action_param* param) {
    size_t idx = editor_get_cursor_idx(param->m);

    c32_t current_char = param->m->text.buffer->str.data[idx];
    for (c32_t current_char = param->m->text.buffer->str.data[idx];
         idx && is_word_separator(current_char); idx += 1,
               current_char = param->m->text.buffer->str.data[idx])
        ;

    for (char current_char = param->m->text.buffer->str.data[idx];
         idx < param->m->text.buffer->str.length &&
         !is_word_separator(current_char);
         idx += 1,
              current_char = param->m->text.buffer->str.data[idx])
        ;

    param->m->cursor.row = buffer_lines_get_line_num_from_idx(
        &param->m->text.buffer->lines, idx);
    assert(idx >=
           param->m->text.buffer->lines.data[param->m->cursor.row]
               .start);

    param->m->cursor.column =
        idx -
        param->m->text.buffer->lines.data[param->m->cursor.row].start;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_word_left(struct editor_action_param* param) {
    size_t idx = editor_get_cursor_idx(param->m);

    if (idx) idx -= 1;

    c32_t current_char = param->m->text.buffer->str.data[idx];
    for (c32_t current_char = param->m->text.buffer->str.data[idx];
         idx && is_word_separator(current_char); idx -= 1,
               current_char = param->m->text.buffer->str.data[idx])
        ;

    for (char current_char = param->m->text.buffer->str.data[idx];
         idx < param->m->text.buffer->str.length &&
         !is_word_separator(current_char);
         idx -= 1,
              current_char = param->m->text.buffer->str.data[idx])
        ;

    if (param->m->text.buffer->str.data[idx] == U' ' ||
        param->m->text.buffer->str.data[idx] == U'\n')
        idx += 1;

    param->m->cursor.row = buffer_lines_get_line_num_from_idx(
        &param->m->text.buffer->lines, idx);
    assert(idx >=
           param->m->text.buffer->lines.data[param->m->cursor.row]
               .start);

    param->m->cursor.column =
        idx -
        param->m->text.buffer->lines.data[param->m->cursor.row].start;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_buffer_begin(
    struct editor_action_param* param) {
    param->m->cursor.row = 0;
    param->m->cursor.column = 0;

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_move_buffer_end(
    struct editor_action_param* param) {
    param->m->cursor.row = param->m->text.buffer->lines.length - 1;
    param->m->cursor.column = line_len(
        &param->m->text.buffer->lines.data[param->m->cursor.row]);

    param->m->editor_flags |= editor_flag_cursor_moved;
    param->m->editor_flags |= editor_flag_cursor_moved_manually;
}

static void (*g_editor_cmd_table[editor_cmd_count])(
    struct editor_action_param*) = {
    [editor_cmd_move_char_left] = editor_move_char_left,
    [editor_cmd_move_char_right] = editor_move_char_right,
    [editor_cmd_move_line_up] = editor_move_line_up,
    [editor_cmd_move_line_down] = editor_move_line_down,
    [editor_cmd_move_end_of_line] = editor_move_end_of_line,
    [editor_cmd_move_beg_of_line] = editor_move_beg_of_line,
    [editor_cmd_copy] = editor_copy,
    [editor_cmd_paste] = editor_paste,
    [editor_cmd_cut] = editor_cut,
    [editor_cmd_begin_mode_selection] = editor_begin_mode_selection,
    [editor_cmd_end_mode_selection] = editor_end_mode_selection,
    [editor_cmd_begin_mode_search] = editor_begin_mode_search,
    [editor_cmd_end_mode_search] = editor_end_mode_search,
    [editor_cmd_delete] = editor_delete,
    [editor_cmd_backspace] = editor_backspace,
    [editor_cmd_next_search_match] = editor_select_next_search_match,
    [editor_cmd_previous_search_match] =
        editor_select_previous_search_match,
    [editor_cmd_undo] = editor_undo,
    [editor_cmd_redo] = editor_redo,
    [editor_cmd_delete_rest_of_line] = editor_delete_rest_of_line,
    [editor_cmd_move_word_right] = editor_move_word_right,
    [editor_cmd_move_word_left] = editor_move_word_left,
    [editor_cmd_move_buffer_end] = editor_move_buffer_end,
    [editor_cmd_move_buffer_begin] = editor_move_buffer_begin,
};

bool editor_handle_char_input(struct editor_action_param* param) {
    bool inserted_something = 0;

    int char_pressed = get_char();
    if (!key_seq_handler_key_seq_active() && char_pressed) {
        editor_insert_char(param, char_pressed);
        inserted_something = 1;
    }

    if (is_key_sticky(KEY_ENTER)) {
        editor_insert_char(param, U'\n');
        inserted_something = 1;
    }

    if (is_key_sticky(KEY_TAB)) {
        editor_insert_tab(param, 4);
        inserted_something = 1;
    }

    return inserted_something;
}

void editor_handle_mode_normal(struct editor_action_param* param) {
    editor_move_cursor_on_click(param);
    editor_handle_char_input(param);

    int command = key_seq_handler_get_command(
        g_cfg.keybinds.editor.mode_normal);
    if (command != -1) g_editor_cmd_table[command](param);
}

static void editor_handle_mode_selection(
    struct editor_action_param* param) {
    int command = key_seq_handler_get_command(
        g_cfg.keybinds.editor.mode_selection);
    if (command != -1) g_editor_cmd_table[command](param);

    editor_move_cursor_on_click(param);

    if (param->m->editor_flags & editor_flag_cursor_moved) {
        text_view_select(
            &param->m->text,
            (struct selection){
                .from_line = param->m->selection_begin.row,
                .from_col = param->m->selection_begin.column,
                .to_line = param->m->cursor.row,
                .to_col = param->m->cursor.column,
            });

        if (text_view_selection_is_invalid(&param->m->text)) {
            text_view_clear_selection(&param->m->text);
            editor_end_mode_selection(param);
            return;
        }
    }

    if (editor_handle_char_input(param)) {
        editor_delete_selection(param);
        editor_end_mode_selection(param);
        editor_move_char_right(param);
    }
}

static void editor_handle_mode_search(
    struct editor_action_param* param) {
    editor_move_cursor_on_click(param);

    int cmd = key_seq_handler_get_command(
        g_cfg.keybinds.editor.mode_search);
    if (cmd != -1) g_editor_cmd_table[cmd](param);
}

static void editor_handle_user_input(
    struct editor_action_param* param) {
    editor_move_cursor_on_click(param);
    switch (param->m->editor_mode) {
        case editor_mode_normal:
            return editor_handle_mode_normal(param);
        case editor_mode_selection:
            return editor_handle_mode_selection(param);
        case editor_mode_search:
            return editor_handle_mode_search(param);
    }
}

void editor_draw(struct editor* m, struct ff_typography typo,
                 Rectangle bounds, int focus_flags) {
    editor_ensure_cursor_idx_within_str(m);
    struct editor_action_param param = {
        .m = m, .typo = typo, .bounds = bounds};
    if (focus_flags & focus_flag_can_interact) {
        editor_handle_user_input(&param);
    }
    editor_ensure_cursor_idx_within_str(m);

    if (m->editor_mode == editor_mode_search &&
        search_mod_input_changed(&m->search_mod)) {
        search_mod_find(&m->search_mod, m->text.buffer);
        editor_select_first_search_match(&param);
    }

    if (m->editor_mode == editor_mode_search) {
        struct decoration decor = {
            .kind = decoration_selection,
            .selections = m->search_mod.search_matches.data,
            .selections_len = m->search_mod.search_matches.length};
        text_view_draw_with_cursor(
            &m->text, typo, bounds, m->cursor,
            m->editor_flags & editor_flag_cursor_moved, focus_flags,
            &decor, 1);
    } else {
        text_view_draw_with_cursor(
            &m->text, typo, bounds, m->cursor,
            m->editor_flags & editor_flag_cursor_moved, focus_flags,
            0, 0);
    }

    if (m->editor_mode == editor_mode_search) {
        search_mod_draw(&m->search_mod, typo, bounds, focus_flags);
    }

    m->editor_flags &= ~editor_flag_cursor_moved;
}

void editor_reset_mode(struct editor* m) {
    m->editor_mode = editor_mode_normal;
    search_mod_clear_matches(&m->search_mod);
    line_editor_clear(&m->search_mod.search_editor);
}

void editor_clear(struct editor* m) {
    buffer_clear(m->text.buffer);
    m->cursor.row = 0;
    m->cursor.column = 0;
}
