#include "line_editor.h"

#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <uchar.h>

#include "history.h"

static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

struct line_editor line_editor_create(void) {
    return (struct line_editor){
        .undo_stack = editor_history_stack_create(),
        .redo_stack = editor_history_stack_create(),
        .cursor = {0},
        .text = text_view_create()};
}

void line_editor_destroy(struct line_editor* this) {
    editor_history_stack_destroy(&this->undo_stack);
    editor_history_stack_destroy(&this->redo_stack);
    text_view_destroy(&this->text);
}

static void line_editor_mov_right(struct line_editor* this) {
    if (this->cursor.column + 1 > this->text.buffer.size) return;
    this->cursor.column += 1;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_mov_beg(struct line_editor* this) {
    this->cursor.column = 0;
    this->line_editor_flags |= line_editor_flag_cursor_moved;
}

static void line_editor_mov_end(struct line_editor* this) {
    this->cursor.column = this->text.buffer.size;
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
        this->text.lines.data[this->text.selection.from_line];
    struct line line_end =
        this->text.lines.data[this->text.selection.to_line];

    size_t beg_index = line_beg.start + this->text.selection.from_col;
    size_t end_index = line_end.start + this->text.selection.to_col;
    size_t count = end_index - beg_index;

    utf32_str_delete(&this->text.buffer, beg_index, count);
    line_editor_end_selection_mode(this);
    text_view_on_modified(&this->text);
}

static void line_editor_delete_char_at(struct line_editor* this,
                                       size_t index) {
    assert(index <= this->text.buffer.size);
    utf32_str_delete(&this->text.buffer, index, 1);
    text_view_on_modified(&this->text);
}

static void line_editor_backspace(struct line_editor* this) {
    editor_history_stack_push(
        &this->undo_stack,
        editor_history_create(&this->text.buffer, this->cursor));

    if (this->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(this);
        line_editor_end_selection_mode(this);
        return;
    }

    size_t cursor_position =
        this->text.lines.data[this->cursor.row].start +
        this->cursor.column;
    if (!cursor_position) return;

    line_editor_delete_char_at(this, cursor_position - 1);
    line_editor_mov_left(this);
}

static void line_editor_save_history(
    struct line_editor* this, struct editor_history_stack* stack) {
    editor_history_stack_push(
        stack,
        editor_history_create(&this->text.buffer, this->cursor));
}

static void line_editor_delete_rest_of_line(
    struct line_editor* this) {
    size_t delete_len = this->text.lines.data[this->cursor.row].end -
                        this->cursor.column;

    line_editor_save_history(this, &this->undo_stack);
    utf32_str_delete(&this->text.buffer,
                    this->text.lines.data[this->cursor.row].start +
                        this->cursor.column,
                    delete_len);
    text_view_on_modified(&this->text);
}

static void line_editor_delete(struct line_editor* this) {
    line_editor_save_history(this, &this->undo_stack);

    if (this->line_editor_mode & line_editor_mode_selection) {
        line_editor_delete_selection(this);
        return;
    }

    if (this->cursor.column ||
        this->cursor.column >= this->text.buffer.size)
        return;

    line_editor_delete_char_at(this, this->cursor.column + 1);
}

static void line_editor_movement_keys(struct line_editor* this) {
    if (is_key_sticky(KEY_F)) return line_editor_mov_right(this);
    if (is_key_sticky(KEY_B)) return line_editor_mov_left(this);
    if (is_key_sticky(KEY_H)) return line_editor_backspace(this);
    if (is_key_sticky(KEY_D)) return line_editor_delete(this);
    if (is_key_sticky(KEY_A)) return line_editor_mov_beg(this);
    if (is_key_sticky(KEY_E)) return line_editor_mov_end(this);
}

static void line_editor_undo(struct line_editor* this) {
    if (!this->undo_stack.size) return;
    line_editor_save_history(this, &this->redo_stack);

    struct editor_history* top =
        editor_history_stack_top(&this->undo_stack);
    utf32_str_copy(&this->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    this->cursor = top->cursor;

    editor_history_stack_pop(&this->undo_stack);
    text_view_on_modified(&this->text);
}

static void line_editor_redo(struct line_editor* this) {
    if (!this->redo_stack.size) return;
    line_editor_save_history(this, &this->undo_stack);

    struct editor_history* top =
        editor_history_stack_top(&this->redo_stack);
    utf32_str_copy(&this->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    this->cursor = top->cursor;

    editor_history_stack_pop(&this->redo_stack);
    text_view_on_modified(&this->text);
}

static void line_editor_insert_char(struct line_editor* this,
                                    char32_t char_) {
    line_editor_save_history(this, &this->undo_stack);

    utf32_str_insert_char(&this->text.buffer, this->cursor.column,
                         char_);
    text_view_on_modified(&this->text);
    line_editor_mov_right(this);
}

static void line_editor_normal_mode(struct line_editor* this) {
    if (is_key_sticky(KEY_BACKSPACE))
        return line_editor_backspace(this);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        line_editor_movement_keys(this);

        if (is_key_sticky(KEY_K))
            return line_editor_delete_rest_of_line(this);

        if (is_key_sticky(KEY_SLASH)) return line_editor_undo(this);
        if (is_key_sticky(KEY_U)) return line_editor_redo(this);
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
    text_view_clear(&e->text);
    e->cursor.row = 0;
    e->cursor.column = 0;
}
