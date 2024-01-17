#include "editor.h"

#include <uchar.h>

#include "../config/config.h"
#include "../resources/resources.h"
#include "../text/text.h"
#include "history.h"
#include "raylib.h"

inline static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}
static void editor_copy_selection_to_clipboard(struct editor* this);
static struct editor_search_highlights
editor_search_highlights_create(void);
static void editor_search_highlights_destroy(
    struct editor_search_highlights* this);
static void editor_search_highlights_clear(
    struct editor_search_highlights* this);
static void editor_search_highlights_push(
    struct editor_search_highlights* this,
    struct selection highlight_selection);
static size_t text_get_index_line_num(struct editor* this,
                                      size_t index);
static void editor_search_highlights_find(struct editor* this);
static void editor_mov_char_right(struct editor* this);
static void editor_paste_from_clipboard(struct editor* this);
static void editor_end_selection_mode(struct editor* this);
static void editor_delete_selection(struct editor* this);
static void editor_cut_selection(struct editor* this);
static void editor_insert_tab(struct editor* this, uint8_t tab_size);
static void editor_mov_line_down(struct editor* this);
static void editor_mov_line_up(struct editor* this);
static void editor_mov_char_left(struct editor* this);
static void editor_mov_beg_of_line(struct editor* this);
static void editor_mov_end_of_line(struct editor* this);
static void editor_end_search_mode(struct editor* this);

static struct editor_search_highlights
editor_search_highlights_create(void) {
    return (struct editor_search_highlights){
        .highlights =
            calloc(sizeof(struct editor_search_highlights), 2),
        .capactiy = sizeof(struct editor_search_highlights) * 2,
        .length = 0};
}

static void editor_search_highlights_destroy(
    struct editor_search_highlights* this) {
    free(this->highlights);
    this->highlights = 0;
    this->capactiy = 0;
    this->length = 0;
}

static void editor_search_highlights_clear(
    struct editor_search_highlights* this) {
    this->length = 0;
}

static void editor_search_highlights_push(
    struct editor_search_highlights* this,
    struct selection highlight_selection) {
    size_t required_cap =
        (this->length + 1) * sizeof(struct selection);

    while (required_cap > this->capactiy) {
        this->capactiy *= 2;
        this->highlights = realloc(this->highlights, this->capactiy);
        assert(this->highlights);
    }

    this->highlights[this->length++] = highlight_selection;
}

char32_t* str32str32(char32_t* pattern, size_t pattern_length,
                     char32_t* s2, size_t s2_length) {
    assert(pattern);
    assert(s2);
    assert(s2_length);
    assert(pattern_length);

    if (pattern_length > s2_length) return 0;

    for (size_t i = 0; i < s2_length; i += 1) {
        if (pattern[0] != s2[i]) continue;
        bool found = !memcmp(pattern, &s2[i],
                             pattern_length * sizeof(char32_t));
        if (found) return &s2[i];
    }

    return 0;
}

static size_t text_get_index_line_num(struct editor* this,
                                      size_t index) {
    assert(index < this->text.buffer.size);

    for (size_t i = 0; i < this->text.lines.size; i += 1)
        if (index >= this->text.lines.data[i].start &&
            index <= this->text.lines.data[i].end)
            return i;

    assert(0 && "failed to find line of index");
}

static void editor_search_highlights_find(struct editor* this) {
    if (this->text.buffer.size == 0) return;
    assert(this->search_editor.text.buffer.size > 0);

    char32_t* sub_str_ptr = 0;
    size_t search_pos = 0;
    while ((sub_str_ptr =
                str32str32(this->search_editor.text.buffer.data,
                           this->search_editor.text.buffer.size,
                           &this->text.buffer.data[search_pos],
                           this->text.buffer.size - search_pos))) {
        size_t match_pos = sub_str_ptr - &this->text.buffer.data[0];
        size_t match_line_num =
            text_get_index_line_num(this, match_pos);
        size_t match_column =
            match_pos - this->text.lines.data[match_line_num].start;

        editor_search_highlights_push(
            &this->search_highlights,
            (struct selection){
                .from_line = match_line_num,
                .from_col = match_column,
                .to_line = match_line_num,
                .to_col = match_column +
                          this->search_editor.text.buffer.size});

        size_t next_search_pos =
            match_pos + this->search_editor.text.buffer.size;
        if (next_search_pos > this->text.buffer.size) return;
        search_pos = next_search_pos;
    }
}

struct editor editor_create(void) {
    struct editor result = {0};
    result.redo_stack = editor_history_stack_create();
    result.undo_stack = editor_history_stack_create();
    result.text = text_create();
    result.search_editor = line_editor_create();
    result.search_highlights = editor_search_highlights_create();
    return result;
}

static void editor_copy_selection_to_clipboard(struct editor* this) {
    assert(this->editor_mode == editor_mode_selection);

    size_t index_begin =
        this->text.lines.data[this->text.selection.from_line].start +
        this->text.selection.from_col;
    size_t index_end =
        this->text.lines.data[this->text.selection.to_line].start +
        this->text.selection.to_col;

    size_t count = index_end - index_begin;
    char utf8_selection_str[count + 1];
    utf8_selection_str[count] = 0;
    ff_utf32_to_utf8(utf8_selection_str,
                     &this->text.buffer.data[index_begin], count);

    SetClipboardText(utf8_selection_str);
    this->editor_mode = editor_mode_normal;
}

static void editor_paste_from_clipboard(struct editor* this) {
    const char* clipboard_utf8 = GetClipboardText();
    if (!clipboard_utf8) return;

    char32_t clipboard_utf8_len = strlen(clipboard_utf8);
    char32_t clipboard_utf32[clipboard_utf8_len + 1];
    clipboard_utf32[clipboard_utf8_len] = 0;

    int err = ff_utf8_to_utf32(clipboard_utf32, clipboard_utf8,
                               clipboard_utf8_len);
    if (err) return;

    editor_save_history(this, &this->undo_stack);
    size_t cursor_index =
        this->text.lines.data[this->cursor.row].start +
        this->cursor.column;
    string32_insert_buf(&this->text.buffer, cursor_index,
                        clipboard_utf32, clipboard_utf8_len);
    text_on_modified(&this->text);

    for (size_t i = 0; i < clipboard_utf8_len; i += 1) {
        editor_mov_char_right(this);
    }
}

static void editor_delete_selection(struct editor* e) {
    if (!(e->text.text_flags & text_flag_has_selection)) {
        editor_end_selection_mode(e);
    }

    e->cursor.row = e->text.selection.from_line;
    e->cursor.column = e->text.selection.from_col;

    struct line line_beg =
        e->text.lines.data[e->text.selection.from_line];
    struct line line_end =
        e->text.lines.data[e->text.selection.to_line];

    size_t beg_index = line_beg.start + e->text.selection.from_col;
    size_t end_index = line_end.start + e->text.selection.to_col;
    size_t count = end_index - beg_index;

    string32_delete(&e->text.buffer, beg_index, count);
    editor_end_selection_mode(e);
    text_on_modified(&e->text);
}

static void editor_cut_selection(struct editor* this) {
    assert(this->editor_mode == editor_mode_selection);
    editor_copy_selection_to_clipboard(this);
    editor_delete_selection(this);
    editor_end_selection_mode(this);
}

void editor_save_history(struct editor* this,
                         struct editor_history_stack* stack) {
    editor_history_stack_push(
        stack,
        editor_history_create(&this->text.buffer, this->cursor));
}

static void editor_undo(struct editor* this) {
    if (!this->undo_stack.size) return;
    editor_save_history(this, &this->redo_stack);

    struct editor_history* top =
        editor_history_stack_top(&this->undo_stack);
    string32_copy(&this->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    this->cursor = top->cursor;
    editor_history_stack_pop(&this->undo_stack);
    text_on_modified(&this->text);
}

static void editor_redo(struct editor* this) {
    if (!this->redo_stack.size) return;
    editor_save_history(this, &this->undo_stack);

    struct editor_history* top =
        editor_history_stack_top(&this->redo_stack);
    string32_copy(&this->text.buffer, top->text_buffer.data,
                  top->text_buffer.size);
    this->cursor = top->cursor;
    editor_history_stack_pop(&this->redo_stack);
    text_on_modified(&this->text);
}

void editor_destroy(struct editor* this) {
    editor_history_stack_destroy(&this->redo_stack);
    editor_history_stack_destroy(&this->undo_stack);
    line_editor_destroy(&this->search_editor);
    text_destroy(&this->text);
    editor_search_highlights_destroy(&this->search_highlights);
}

static void editor_backspace(struct editor* this) {
    editor_save_history(this, &this->undo_stack);

    if (this->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(this);
    if (this->editor_mode & editor_mode_selection)
        editor_end_selection_mode(this);

    size_t cursor_position =
        this->text.lines.data[this->cursor.row].start +
        this->cursor.column;
    if (cursor_position == 0) return;

    string32_delete(&this->text.buffer, cursor_position - 1, 1);
    editor_mov_char_left(this);
    text_on_modified(&this->text);
}

static void editor_delete(struct editor* this) {
    editor_save_history(this, &this->undo_stack);

    if (this->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(this);
    size_t cursor_position =
        this->text.lines.data[this->cursor.row].start +
        this->cursor.column;
    if (cursor_position >= this->text.buffer.size) return;
    string32_delete(&this->text.buffer, cursor_position, 1);
    text_on_modified(&this->text);
}

static void editor_insert_char(struct editor* this, char32_t chr) {
    editor_save_history(this, &this->undo_stack);

    string32_insert_char(
        &this->text.buffer,
        this->text.lines.data[this->cursor.row].start +
            this->cursor.column,
        chr);
    text_on_modified(&this->text);
    editor_mov_char_right(this);
}

static void editor_insert_tab(struct editor* this, uint8_t tab_size) {
    editor_save_history(this, &this->undo_stack);

    char32_t tab[tab_size];
    for (size_t i = 0; i < tab_size; i += 1) tab[i] = U' ';
    string32_insert_buf(
        &this->text.buffer,
        this->text.lines.data[this->cursor.row].start +
            this->cursor.column,
        tab, tab_size);
    text_on_modified(&this->text);
    for (size_t i = 0; i < tab_size; i += 1)
        editor_mov_char_right(this);
}

static void editor_mov_char_right(struct editor* this) {
    struct line line = this->text.lines.data[this->cursor.row];
    if (this->cursor.column >= line_length(line))
        return editor_mov_line_down(this);
    this->cursor.column += 1;
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_mov_line_up(struct editor* this) {
    if (this->cursor.row == 0) return editor_mov_char_left(this);
    this->cursor.column = 0;  // TODO
    this->cursor.row -= 1;
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_mov_char_left(struct editor* this) {
    if (this->cursor.column == 0) {
        if (this->cursor.row == 0) return;
        editor_mov_line_up(this);
        return editor_mov_end_of_line(this);
    }
    this->cursor.column -= 1;
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_mov_beg_of_line(struct editor* this) {
    this->cursor.column = 0;
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_mov_end_of_line(struct editor* this) {
    struct line line = this->text.lines.data[this->cursor.row];
    this->cursor.column = line_length(line);
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_mov_line_down(struct editor* this) {
    if (this->cursor.row >= this->text.lines.size - 1) return;
    this->cursor.column = 0;
    this->cursor.row += 1;
    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_begin_selection_mode(struct editor* this) {
    this->editor_mode = editor_mode_selection;
    this->selection_begin = this->cursor;
}

static void editor_end_selection_mode(struct editor* this) {
    this->editor_mode = editor_mode_normal;
    text_clear_selection(&this->text);
}

static void editor_begin_search_input_mode(struct editor* this) {
    this->editor_mode = editor_mode_search_input;
}

static void editor_end_search_input_mode(struct editor* this) {
    this->editor_mode = editor_mode_normal;
}

static void editor_begin_search_mode(struct editor* this) {
    if (!this->search_editor.text.buffer.size) {
        this->editor_mode = editor_mode_normal;
        return;
    }
    this->editor_mode = editor_mode_search;
    editor_search_highlights_find(this);
}

static void editor_end_search_mode(struct editor* this) {
    this->editor_mode = editor_mode_normal;
    this->match_select_counter = 0;
    editor_search_highlights_clear(&this->search_highlights);
}

static void editor_next_search_match(struct editor* this) {
    assert(this->editor_mode == editor_mode_search);

    if (this->match_select_counter >
        this->search_highlights.length - 1)
        this->match_select_counter = 0;

    struct selection highlight =
        this->search_highlights
            .highlights[this->match_select_counter++];

    this->cursor.row = highlight.from_line;
    this->cursor.column = highlight.from_col;

    this->editor_flags |= editor_flag_cursor_moved;
}

static void editor_search_mode_keybinds(struct editor* this) {
    if (IsKeyPressed(KEY_ESCAPE)) editor_end_search_mode(this);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_S)) editor_next_search_match(this);
        if (is_key_sticky(KEY_G)) editor_end_search_mode(this);
        if (is_key_sticky(KEY_SPACE)) {
            editor_end_search_mode(this);
            editor_begin_selection_mode(this);
        }
    }

    int char_pressed = GetCharPressed();
    if (char_pressed) {
        editor_insert_char(this, char_pressed);
        editor_end_search_mode(this);
    }
}

static void editor_movement_keybinds(struct editor* this) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_F)) return editor_mov_char_right(this);
        if (is_key_sticky(KEY_B)) return editor_mov_char_left(this);
        if (is_key_sticky(KEY_A)) return editor_mov_beg_of_line(this);
        if (is_key_sticky(KEY_E)) return editor_mov_end_of_line(this);
        if (is_key_sticky(KEY_N)) return editor_mov_line_down(this);
        if (is_key_sticky(KEY_P)) return editor_mov_line_up(this);
    }
}

static void editor_delete_rest_of_line(struct editor* this) {
    editor_save_history(this, &this->undo_stack);
    if (this->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(this);

    struct line line = this->text.lines.data[this->cursor.row];
    size_t start_index = line.start + this->cursor.column;
    size_t delete_len = line.end - start_index;

    string32_delete(&this->text.buffer, start_index, delete_len);
    text_on_modified(&this->text);
}

static void editor_modifier_keybinds(struct editor* this) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_H)) return editor_backspace(this);
        if (is_key_sticky(KEY_D)) return editor_delete(this);
        if (is_key_sticky(KEY_K))
            return editor_delete_rest_of_line(this);
        if (is_key_sticky(KEY_SLASH)) return editor_undo(this);
        if (IsKeyDown(KEY_LEFT_SHIFT) && is_key_sticky(KEY_U))
            return editor_redo(this);
    }

    if (is_key_sticky(KEY_ENTER))
        return editor_insert_char(this, U'\n');
    if (is_key_sticky(KEY_TAB)) return editor_insert_tab(this, 4);
    if (is_key_sticky(KEY_BACKSPACE)) return editor_backspace(this);

    char32_t char_pressed = GetCharPressed();
    if (char_pressed) return editor_insert_char(this, char_pressed);
}

static void editor_normal_mode_keybinds(struct editor* this) {
    editor_movement_keybinds(this);
    editor_modifier_keybinds(this);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_SPACE))
            return editor_begin_selection_mode(this);
        if (is_key_sticky(KEY_S))
            return editor_begin_search_input_mode(this);
        if (is_key_sticky(KEY_Y))
            return editor_paste_from_clipboard(this);
    }
}

static void editor_move_cursor_on_click(struct editor* e,
                                        struct ff_typography typo,
                                        Rectangle bounds) {
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, bounds)) return;

    ulong hover_line = text_get_mouse_hover_line(&e->text, typo.size,
                                                 mouse, bounds.y);
    ulong hover_col = text_get_mouse_hover_col(&e->text, typo, mouse,
                                               bounds.x, hover_line);

    e->cursor.row = hover_line;
    e->cursor.column = hover_col;
}

static void editor_handle_selection_mode_interactions(
    struct editor* this, struct ff_typography typo,
    Rectangle bounds) {
    editor_move_cursor_on_click(this, typo, bounds);
    editor_movement_keybinds(this);
    if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_G)) ||
        IsKeyPressed(KEY_ESCAPE)) {
        return editor_end_selection_mode(this);
    }
    if ((IsKeyDown(KEY_LEFT_ALT)) && IsKeyPressed(KEY_W)) {
        editor_copy_selection_to_clipboard(this);
        editor_end_selection_mode(this);
    }
    if ((IsKeyDown(KEY_LEFT_CONTROL)) && IsKeyPressed(KEY_W)) {
        editor_cut_selection(this);
        editor_end_selection_mode(this);
    }
    if (this->editor_flags & editor_flag_cursor_moved) {
        text_select(&this->text,
                    (struct selection){
                        .from_line = this->selection_begin.row,
                        .from_col = this->selection_begin.column,
                        .to_line = this->cursor.row,
                        .to_col = this->cursor.column,
                    });
    }
}

void editor_handle_search_input_interactions(struct editor* this) {
    if (is_key_sticky(KEY_ESCAPE)) editor_end_search_input_mode(this);

    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_G))
            return editor_end_search_input_mode(this);
    }
    if (IsKeyPressed(KEY_ENTER)) {
        editor_end_search_input_mode(this);
        editor_begin_search_mode(this);
    }
}

static void editor_handle_interactions(struct editor* e,
                                       struct ff_typography typo,
                                       Rectangle bounds) {
    switch (e->editor_mode) {
        case editor_mode_normal:
            return editor_normal_mode_keybinds(e);
        case editor_mode_selection:
            return editor_handle_selection_mode_interactions(e, typo,
                                                             bounds);
        case editor_mode_search_input:
            return editor_handle_search_input_interactions(e);

        case editor_mode_search:
            return editor_search_mode_keybinds(e);
        default:;
    }
}

#define EDITOR_SEARCH_BOX_WIDTH 256
static void editor_draw_search_input(struct editor* e,
                                     struct ff_typography typo,
                                     Rectangle bounds,
                                     int focus_flags) {
    assert(e->editor_mode == editor_mode_search_input);

    Texture search_icon = get_icon(icon_search_t);

    Rectangle bg;
    bg.width = bounds.width < EDITOR_SEARCH_BOX_WIDTH
                   ? bounds.width
                   : EDITOR_SEARCH_BOX_WIDTH;
    bg.x = bounds.x + bounds.width * 0.5f - bg.width * 0.5f;
    bg.y = bounds.height * 0.5f - typo.size * 0.5f;
    bg.height = typo.size + g_layout.padding * 2;

    DrawRectangleRec(bg, GetColor(g_color_scheme.surface0_bg));

    Rectangle icon_source = {.x = 0,
                             .y = 0,
                             .width = search_icon.width,
                             .height = search_icon.height};

    bg.x += g_layout.padding;
    bg.y += g_layout.padding;
    Rectangle icon_dest = {.x = bg.x,
                           .y = bg.y,
                           .width = search_icon.width,
                           .height = search_icon.height};
    DrawTexturePro(search_icon, icon_source, icon_dest, (Vector2){0},
                   0, WHITE);

    bg.x += search_icon.width + g_layout.padding;
    bg.width -= search_icon.width + g_layout.padding * 3;
    line_editor_draw(&e->search_editor, typo, bg, focus_flags);
};

void editor_draw(struct editor* this, struct ff_typography typo,
                 Rectangle bounds, int focus_flags) {
    if (focus_flags & focus_flag_can_interact) {
        editor_handle_interactions(this, typo, bounds);
    }

    if (this->search_highlights.length > 0) {
        struct text_search_highlight search_highlights = {
            .highlights = this->search_highlights.highlights,
            .count = this->search_highlights.length};
        text_draw_with_cursor(
            &this->text, typo, bounds, this->cursor,
            this->editor_flags & editor_flag_cursor_moved,
            focus_flags, &search_highlights);
    } else {
        text_draw_with_cursor(
            &this->text, typo, bounds, this->cursor,
            this->editor_flags & editor_flag_cursor_moved,
            focus_flags, 0);
    }

    if (this->editor_mode == editor_mode_search_input) {
        editor_draw_search_input(this, typo, bounds, focus_flags);
    }

    this->editor_flags &= ~editor_flag_cursor_moved;
}

void editor_clear(struct editor* e) {
    text_clear(&e->text);
    e->cursor.row = 0;
    e->cursor.column = 0;
}
