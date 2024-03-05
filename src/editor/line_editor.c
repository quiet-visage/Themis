#include "line_editor.h"

#include <fieldfusion.h>
#include <raylib.h>
#include <uchar.h>

#include "../buffer.h"

static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

struct line_editor line_editor_create(void) {
    return (struct line_editor){
        .selection_begin = {0},
        .cursor = {0},
        .text = text_view_create(),
        .line_editor_flags = 0,
        .line_editor_mode = line_editor_mode_normal};
}

void line_editor_destroy(struct line_editor* this) {
    text_view_destroy(&this->text);
}

static void line_editor_mov_right(struct line_editor* this) {
    assert(this->text.buffer);
    if (this->cursor.column + 1 > this->text.buffer->str.size) return;
    this->cursor.column += 1;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_mov_beg(struct line_editor* this) {
    this->cursor.column = 0;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_mov_end(struct line_editor* this) {
    assert(this->text.buffer);
    this->cursor.column = this->text.buffer->str.size;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_mov_left(struct line_editor* this) {
    if (this->cursor.column > 0) this->cursor.column -= 1;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_begin_selection_mode(
    struct line_editor* this) {
    this->line_editor_mode = line_editor_mode_selection;
    this->selection_begin = this->cursor;
}

static void line_editor_end_selection_mode(struct line_editor* this) {
    this->line_editor_mode = line_editor_mode_normal;
    text_view_clear_selection(&this->text);
}

static void line_editor_delete_selection(struct line_editor* this) {
    assert(this->line_editor_mode == line_editor_mode_selection);

    this->cursor.row = this->text.selection.from_line;
    this->cursor.column = this->text.selection.from_col;

    struct line line_beg =
        this->text.buffer->lines.data[this->text.selection.from_line];
    struct line line_end =
        this->text.buffer->lines.data[this->text.selection.to_line];

    size_t beg_index = line_beg.start + this->text.selection.from_col;
    size_t end_index = line_end.start + this->text.selection.to_col;
    size_t count = end_index - beg_index;

    buffer_delete(this->text.buffer, beg_index, count);
    line_editor_end_selection_mode(this);
}

static void line_editor_delete_char_at(struct line_editor* this,
                                       size_t index) {
    assert(index <= this->text.buffer->str.size);
    buffer_delete(this->text.buffer, index, 1);
}

static void line_editor_backspace(struct line_editor* this) {
    assert(this->text.buffer);
    buffer_save_undo(this->text.buffer);

    if (this->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(this);
        line_editor_end_selection_mode(this);
        return;
    }

    size_t cursor_position =
        this->text.buffer->lines.data[this->cursor.row].start +
        this->cursor.column;
    if (!cursor_position) return;

    line_editor_delete_char_at(this, cursor_position - 1);
    line_editor_mov_left(this);
}

static void line_editor_delete_rest_of_line(
    struct line_editor* this) {
    assert(this->text.buffer);
    buffer_save_undo(this->text.buffer);

    if (this->text.text_flags & text_flag_has_selection)
        return line_editor_delete_selection(this);

    struct line line = this->text.buffer->lines.data[this->cursor.row];
    size_t start_index = line.start + this->cursor.column;
    size_t delete_len = line.end - start_index;

    buffer_delete(this->text.buffer, start_index,
                     delete_len);
}

static void line_editor_delete(struct line_editor* this) {
    assert(this->text.buffer);
    buffer_save_undo(this->text.buffer);

    if (this->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(this);
        return;
    }

    if (this->cursor.column ||
        this->cursor.column >= this->text.buffer->str.size)
        return;

    line_editor_delete_char_at(this, this->cursor.column);
}

static void line_editor_movement_keys(struct line_editor* this) {
    if (is_key_sticky(KEY_F)) return line_editor_mov_right(this);
    if (is_key_sticky(KEY_B)) return line_editor_mov_left(this);
    if (is_key_sticky(KEY_H)) return line_editor_backspace(this);
    if (is_key_sticky(KEY_D)) return line_editor_delete(this);
    if (is_key_sticky(KEY_A)) return line_editor_mov_beg(this);
    if (is_key_sticky(KEY_E)) return line_editor_mov_end(this);
}

static void line_editor_insert_char(struct line_editor* this,
                                    char32_t char_) {
    assert(this->text.buffer);
    buffer_save_undo(this->text.buffer);

    buffer_insert_char(this->text.buffer,
                          this->cursor.column, char_);
    line_editor_mov_right(this);
}

static void line_editor_normal_mode(struct line_editor* this) {
    if (is_key_sticky(KEY_BACKSPACE))
        return line_editor_backspace(this);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        line_editor_movement_keys(this);

        if (is_key_sticky(KEY_K))
            return line_editor_delete_rest_of_line(this);

        if (is_key_sticky(KEY_SLASH))
            return buffer_undo(this->text.buffer);
        if (is_key_sticky(KEY_U))
            return buffer_redo(this->text.buffer);
        if (IsKeyPressed(KEY_SPACE))
            return line_editor_begin_selection_mode(this);
        if (IsKeyPressed(KEY_G))
            return line_editor_end_selection_mode(this);
    }

    char32_t char_pressed = GetCharPressed();
    if (char_pressed) {
        line_editor_insert_char(this, char_pressed);
        this->line_editor_flags |= line_editor_flag_cursor_moved;
    }
}

static void line_editor_selection_mode(struct line_editor* this) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        line_editor_movement_keys(this);
    }

    if (this->line_editor_flags & line_editor_flag_cursor_moved) {
        text_view_select(&this->text,
                         (struct selection){
                             .from_line = this->selection_begin.row,
                             .from_col = this->selection_begin.column,
                             .to_line = this->cursor.row,
                             .to_col = this->cursor.column,
                         });
    }
}

void line_editor_handle_interaction(struct line_editor* this) {
    switch (this->line_editor_mode) {
        case line_editor_mode_normal:
            line_editor_normal_mode(this);
            break;
        case line_editor_mode_selection:
            line_editor_selection_mode(this);
            break;
        default: break;
    }
}

void line_editor_draw(struct line_editor* this,
                      struct ff_typography typo, Rectangle bounds,
                      int focus_flags) {
    if (focus_flags & focus_flag_can_interact) {
        line_editor_handle_interaction(this);
    }
    focus_flags &= ~focus_flag_can_scroll;
    text_view_draw_with_cursor(
        &this->text, typo, bounds, this->cursor,
        this->line_editor_flags & line_editor_flag_cursor_moved,
        focus_flags, 0);

    this->line_editor_flags &= ~line_editor_flag_cursor_moved;
}

void line_editor_clear(struct line_editor* e) {
    buffer_clear(e->text.buffer);
    e->cursor.row = 0;
    e->cursor.column = 0;
}
