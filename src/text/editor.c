#include "editor.h"

#include <assert.h>
#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <raylib.h>
#include <stdint.h>
#include <text/editor.h>
#include <text/text.h>
#include <text/unicode_string.h>
#include <uchar.h>

#include "text.h"
#include "unicode_string.h"

static struct editor_history editor_history_create(struct editor* e) {
    struct editor_history result = {
        .text_buffer = string32_clone(&e->text.buffer),
        .cursor = e->cursor};
    return result;
}

static struct editor_history editor_history_clone(
    struct editor_history* eh) {
    struct editor_history result = {
        .text_buffer = string32_clone(&eh->text_buffer),
        .cursor = eh->cursor};
    return result;
}

static void editor_history_destroy(struct editor_history* h) {
    string32_destroy(&h->text_buffer);
}

static struct editor_history_stack editor_history_stack_create(void) {
    ulong prealloc = 2;
    struct editor_history_stack result = {
        .data = calloc(sizeof(struct editor_history), prealloc),
        .size = 0,
        .capacity = sizeof(struct editor_history) * prealloc};
    return result;
}

static void editor_history_stack_free(
    struct editor_history_stack* s) {
    for (ulong i = 0; i < s->size; i += 1) {
        editor_history_destroy(&s->data[i]);
    }
    free(s->data);
    s->data = NULL;
    s->capacity = 0;
}

static void editor_history_stack_pop(struct editor_history_stack* s) {
    assert(s->size > 0);
    editor_history_destroy(&s->data[s->size - 1]);
    s->size -= 1;
}

static struct editor_history*
editor_history_stack_push_current_history(
    struct editor_history_stack* s, struct editor* editor) {
    size_t required_capacity =
        (s->size + 1) * sizeof(struct editor_history);

    while (required_capacity > s->capacity) {
        s->capacity *= 2;
        s->data = realloc(s->data, s->capacity);
        assert(s->data);
    }

    s->data[s->size++] = editor_history_create(editor);
}
static struct editor_history* editor_history_stack_top(
    struct editor_history_stack* s) {
    assert(s->size > 0);
    return &s->data[s->size - 1];
}

void editor_save_undo(struct editor* e);
struct editor editor_create(void) {
    struct editor result = {0};
    result.redo_stack = editor_history_stack_create();
    result.undo_stack = editor_history_stack_create();
    result.text = text_create();
    return result;
}

void editor_undo(struct editor* e) {
    if (e->undo_stack.size == 0) return;
    editor_history_stack_push_current_history(&e->redo_stack, e);
    struct editor_history* top =
        editor_history_stack_top(&e->undo_stack);
    e->cursor_moved = true;

    e->cursor = top->cursor;
    string32_copy(&e->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    text_on_modified(&e->text);

    editor_history_stack_pop(&e->undo_stack);
}

void editor_redo(struct editor* e) {
    if (e->redo_stack.size == 0) return;
    editor_history_stack_push_current_history(&e->undo_stack, e);
    struct editor_history* top =
        editor_history_stack_top(&e->redo_stack);

    e->cursor = top->cursor;
    string32_copy(&e->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    text_on_modified(&e->text);

    editor_history_stack_pop(&e->redo_stack);
}

void editor_save_undo(struct editor* e) {
    editor_history_stack_push_current_history(&e->undo_stack, e);
}

static void editor_end_selection_mode(struct editor* e);
static void editor_delete_selection(struct editor* e) {
    assert(e->text.has_selection);

    e->cursor.row = e->text.selection.from_line;
    e->cursor.column = e->text.selection.from_col;

    struct line line_beg = e->text.lines.data[e->text.selection.from_line];
    struct line line_end = e->text.lines.data[e->text.selection.to_line];

    size_t beg_index = line_beg.start + e->text.selection.from_col;
    size_t end_index = line_end.start + e->text.selection.to_col;
    size_t count = end_index - beg_index;

    string32_delete(&e->text.buffer, beg_index, count);
    editor_end_selection_mode(e);
    text_on_modified(&e->text);
}

void editor_destroy(struct editor* e) {
    editor_history_stack_free(&e->redo_stack);
    editor_history_stack_free(&e->undo_stack);
    text_destroy(&e->text);
}

static void editor_mov_char_left(struct editor* e);

static void editor_backspace(struct editor* e) {
    editor_save_undo(e);

    if (e->text.has_selection) return editor_delete_selection(e);
    if (e->editor_mode & editor_mode_selection) editor_end_selection_mode(e);

    size_t cursor_position =
        e->text.lines.data[e->cursor.row].start + e->cursor.column;
    if (cursor_position == 0) return;

    string32_delete(&e->text.buffer, cursor_position - 1, 1);
    editor_mov_char_left(e);
    text_on_modified(&e->text);
}

static void editor_delete(struct editor* e) {
    editor_save_undo(e);
    if (e->text.has_selection) return editor_delete_selection(e);
    size_t cursor_position =
        e->text.lines.data[e->cursor.row].start + e->cursor.column;
    if (cursor_position >= e->text.buffer.size) return;
    string32_delete(&e->text.buffer, cursor_position, 1);
    text_on_modified(&e->text);
}

static void editor_mov_char_right(struct editor* e);
static void editor_insert_char(struct editor* e, char32_t chr) {
    editor_save_undo(e);
    string32_insert_char(
        &e->text.buffer,
        e->text.lines.data[e->cursor.row].start + e->cursor.column,
        chr);
    text_on_modified(&e->text);
    editor_mov_char_right(e);
}

static void editor_insert_tab(struct editor* e, uint8_t tab_size) {
    editor_save_undo(e);
    char32_t tab[tab_size];
    for (size_t i = 0; i < tab_size; i += 1) tab[i] = U' ';
    string32_insert_buf(
        &e->text.buffer,
        e->text.lines.data[e->cursor.row].start + e->cursor.column,
        tab, tab_size);
    text_on_modified(&e->text);
    for (size_t i = 0; i < tab_size; i += 1) editor_mov_char_right(e);
}

static void editor_mov_line_down(struct editor* e);
static void editor_mov_char_right(struct editor* e) {
    struct line line = e->text.lines.data[e->cursor.row];
    if (e->cursor.column >= line_length(line))
        return editor_mov_line_down(e);
    e->cursor.column += 1;
    e->cursor_moved = true;
}

static void editor_mov_char_left(struct editor* e);
static void editor_mov_line_up(struct editor* e) {
    if (e->cursor.row == 0) return editor_mov_char_left(e);
    e->cursor.column = 0;  // TODO
    e->cursor.row -= 1;
    e->cursor_moved = true;
}

static void editor_mov_end_of_line(struct editor* e);
static void editor_mov_char_left(struct editor* e) {
    if (e->cursor.column == 0) {
        if (e->cursor.row == 0) return;
        editor_mov_line_up(e);
        return editor_mov_end_of_line(e);
    }
    e->cursor.column -= 1;
    e->cursor_moved = true;
}

static void editor_mov_beg_of_line(struct editor* e) {
    e->cursor.column = 0;
}

static void editor_mov_end_of_line(struct editor* e) {
    struct line line = e->text.lines.data[e->cursor.row];
    e->cursor.column = line_length(line);
    e->cursor_moved = true;
}

static void editor_mov_line_down(struct editor* e) {
    if (e->cursor.row >= e->text.lines.size - 1) return;
    e->cursor.column = 0;
    e->cursor.row += 1;
    e->cursor_moved = true;
}

static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

static void editor_begin_selection_mode(struct editor* e) {
    e->editor_mode = editor_mode_selection;
    e->selection_begin = e->cursor;
}

static void editor_end_selection_mode(struct editor* e) {
    e->editor_mode = editor_mode_none;
    text_clear_selection(&e->text);
}

static void emacs_keybinds(struct editor* e) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_F)) return editor_mov_char_right(e);
        if (is_key_sticky(KEY_B)) return editor_mov_char_left(e);
        if (is_key_sticky(KEY_H)) return editor_backspace(e);
        if (is_key_sticky(KEY_D)) return editor_delete(e);
        if (is_key_sticky(KEY_A)) return editor_mov_beg_of_line(e);
        if (is_key_sticky(KEY_E)) return editor_mov_end_of_line(e);
        if (is_key_sticky(KEY_N)) return editor_mov_line_down(e);
        if (is_key_sticky(KEY_P)) return editor_mov_line_up(e);
        if (is_key_sticky(KEY_SLASH)) return editor_undo(e);
        if (is_key_sticky(KEY_U)) return editor_redo(e);
        if (is_key_sticky(KEY_SPACE))
            return editor_begin_selection_mode(e);
        if (is_key_sticky(KEY_G)) return editor_end_selection_mode(e);
    }
}

static void editor_handle_interactions(struct editor* e,
                                       struct ff_typography typo,
                                       Rectangle bounds) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (!CheckCollisionPointRec(mouse, bounds)) return;
        ulong hover_line = text_get_mouse_hover_line(
            &e->text, typo.size, mouse, bounds.y);
        ulong hover_col = text_get_mouse_hover_col(
            &e->text, typo, mouse, bounds.x, hover_line);
        e->cursor.row = hover_line;
        e->cursor.column = hover_col;
    }

    emacs_keybinds(e);
    switch (e->editor_mode) {
        case editor_mode_selection: {
            if (IsKeyPressed(KEY_ESCAPE))
                editor_end_selection_mode(e);

            if (!e->cursor_moved) return;
            text_select(&e->text,
                        (struct selection){
                            .from_line = e->selection_begin.row,
                            .from_col = e->selection_begin.column,
                            .to_line = e->cursor.row,
                            .to_col = e->cursor.column});

            return;
        } break;
        default: break;
    }

    if (!(e->editor_flags & editor_flag_single_line_mode_t ||
          e->editor_flags & editor_flag_ignore_enter_t) &&
        is_key_sticky(KEY_ENTER))
        return editor_insert_char(e, U'\n');
    if (is_key_sticky(KEY_TAB)) return editor_insert_tab(e, 4);
    if (is_key_sticky(KEY_BACKSPACE)) return editor_backspace(e);
    int char_ = GetCharPressed();
    if (char_) {
        return editor_insert_char(e, char_);
    }
}

void editor_draw(struct editor* e, struct ff_typography typo,
                 Rectangle bounds, int focus_flags) {
    if (focus_flags & focus_flag_can_interact)
        editor_handle_interactions(e, typo, bounds);
    text_draw_with_cursor(&e->text, typo, bounds, e->cursor,
                          e->cursor_moved, focus_flags);
    e->cursor_moved = false;
}
void editor_clear(struct editor* e) {
    text_clear(&e->text);
    e->cursor.row = 0;
    e->cursor.column = 0;
}
