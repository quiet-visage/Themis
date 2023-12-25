#include <assert.h>
#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <raylib.h>
#include <stdint.h>
#include <text/editor.h>
#include <text/text.h>
#include <text/unicode_string.h>
#include <uchar.h>
#include "editor.h"
#include "text.h"

static struct editor_history editor_history_new(char32_t* buffer,
                                           ulong buffer_size,
                                           ulong cursor_pos) {
    struct editor_history result = {
        .buffer = calloc(sizeof(char32_t), buffer_size),
        .buffer_size = buffer_size,
        .cursor_pos = cursor_pos};
    assert(result.buffer);
    memcpy(result.buffer, buffer, sizeof(char32_t) * buffer_size);
    return result;
}

static void editor_history_free(struct editor_history* h) {
    free(h->buffer);
    h->buffer_size = 0;
}

static struct editor_history_stack editor_history_stack_new(void) {
    ulong prealloc = 2;
    struct editor_history_stack result = {
        .data = calloc(sizeof(struct editor_history), prealloc),
        .size = 0,
        .capacity = sizeof(struct editor_history) * prealloc};
    return result;
}

static void editor_history_stack_free(struct editor_history_stack* s) {
    for (ulong i = 0; i < s->size; i += 1) {
        editor_history_free(&s->data[i]);
    }
    free(s->data);
    s->data = NULL;
    s->capacity = 0;
}

static void editor_history_stack_pop(struct editor_history_stack* s) {
    assert(s->size > 0);
    editor_history_free(&s->data[s->size - 1]);
    s->size -= 1;
}

static struct editor_history* editor_history_stack_top(
    struct editor_history_stack* s) {
    assert(s->size > 0);
    return &s->data[s->size - 1];
}

struct editor editor_create(void) {
    struct editor result = {0};
    result.redo_stack = editor_history_stack_new();
    result.undo_stack = editor_history_stack_new();
    result.text = text_create();
    return result;
}

void editor_destroy(struct editor* e) {
    editor_history_stack_free(&e->redo_stack);
    editor_history_stack_free(&e->undo_stack);
    text_destroy(&e->text);
}

static void editor_mov_char_left(struct editor* e);
static void editor_backspace(struct editor* e) {
    size_t cursor_position =
        e->text.lines.data[e->cursor.row].start + e->cursor.column;
    if (cursor_position == 0) return;
    string32_delete(&e->text.buffer, cursor_position - 1, 1);
    text_on_modified(&e->text);
    editor_mov_char_left(e);
}

static void editor_delete(struct editor* e) {
    size_t cursor_position =
        e->text.lines.data[e->cursor.row].start + e->cursor.column;
    if (cursor_position >= e->text.buffer.size) return;
    string32_delete(&e->text.buffer, cursor_position, 1);
    text_on_modified(&e->text);
}

static void editor_mov_char_right(struct editor* e);
static void editor_insert_char(struct editor* e, char32_t chr) {
    string32_insert_char(
        &e->text.buffer,
        e->text.lines.data[e->cursor.row].start + e->cursor.column,
        chr);
    text_on_modified(&e->text);
    editor_mov_char_right(e);
}

static void editor_insert_tab(struct editor* e, uint8_t tab_size) {
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

static void editor_mov_char_left(struct editor* e) {
    if (e->cursor.column == 0) {
        if (e->cursor.row == 0) return;
        return editor_mov_line_up(e);
    }
    e->cursor.column -= 1;
    e->cursor_moved = true;
}

static void editor_mov_end_of_line(struct editor* e) {
    struct line line = e->text.lines.data[e->cursor.row];
    e->cursor.column = line_length(line) - 1;
    e->cursor_moved = true;
}

static void editor_mov_line_down(struct editor* e) {
    if (e->cursor.row >= e->text.lines.size - 1) return;
    e->cursor.column = 0;
    e->cursor.row += 1;
    e->cursor_moved = true;
}

static void editor_undo(struct editor* e) {}

static void editor_redo(struct editor* e) {}

static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
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

    if (is_key_sticky(KEY_LEFT)) return editor_mov_char_left(e);
    if (is_key_sticky(KEY_RIGHT)) return editor_mov_char_right(e);
    if (is_key_sticky(KEY_UP)) return editor_mov_line_up(e);
    if (is_key_sticky(KEY_DOWN)) return editor_mov_line_down(e);

    int char_pressed = GetCharPressed();
    if (char_pressed) return editor_insert_char(e, char_pressed);
    if (!(e->editor_flags & editor_flag_ignore_enter_t) &&
        is_key_sticky(KEY_ENTER))
        return editor_insert_char(e, U'\n');
    if (is_key_sticky(KEY_BACKSPACE)) return editor_backspace(e);
    if (is_key_sticky(KEY_DELETE)) return editor_delete(e);
    if (is_key_sticky(KEY_TAB)) return editor_insert_tab(e, 4);
}

void editor_draw(struct editor* e, struct ff_typography typo, Rectangle bounds,
                 bool focused) {
    if (focused) editor_handle_interactions(e, typo, bounds);
    text_draw_with_cursor(&e->text, typo, bounds, e->cursor,
                          e->cursor_moved);
    e->cursor_moved = false;
}
