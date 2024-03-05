#include "editor.h"

#include <uchar.h>

#include "../buffer.h"
#include "../config/config.h"
#include "../resources/resources.h"
#include "cursor_history.h"
#include "raylib.h"

inline static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}
static void editor_copy_selection_to_clipboard(struct editor* o);
static struct editor_search_highlights
editor_search_highlights_create(void);
static void editor_search_highlights_destroy(
    struct editor_search_highlights* o);
static void editor_search_highlights_clear(
    struct editor_search_highlights* o);
static void editor_search_highlights_push(
    struct editor_search_highlights* o,
    struct selection highlight_selection);
static size_t text_get_index_line_num(struct editor* o, size_t index);
static void editor_search_highlights_find(struct editor* o);
static void editor_mov_char_right(struct editor* o);
static void editor_paste_from_clipboard(struct editor* o);
static void editor_end_selection_mode(struct editor* o);
static void editor_delete_selection(struct editor* o);
static void editor_cut_selection(struct editor* o);
static void editor_insert_tab(struct editor* o, uint8_t tab_size);
static void editor_mov_line_down(struct editor* o);
static void editor_mov_line_up(struct editor* o);
static void editor_mov_char_left(struct editor* o);
static void editor_mov_beg_of_line(struct editor* o);
static void editor_mov_end_of_line(struct editor* o);
static void editor_end_search_mode(struct editor* o);

static void editor_save_undo(struct editor* o) {
    buffer_save_undo(o->text.buffer);
    cursor_history_push(&o->cursor_undo, o->cursor);
}

static void editor_undo(struct editor* o) {
    if (!o->text.buffer->undo_history.length) return;
    buffer_undo(o->text.buffer);
    cursor_history_push(&o->cursor_redo, o->cursor);
    o->cursor = cursor_history_top(&o->cursor_undo);
    cursor_history_pop(&o->cursor_undo);
    o->editor_flags |= editor_flag_cursor_moved;
}

static void editor_redo(struct editor* o) {
    if (!o->text.buffer->redo_history.length) return;
    buffer_redo(o->text.buffer);
    cursor_history_push(&o->cursor_undo, o->cursor);
    o->cursor = cursor_history_top(&o->cursor_redo);
    cursor_history_pop(&o->cursor_redo);
    o->editor_flags |= editor_flag_cursor_moved;
}

static struct editor_search_highlights
editor_search_highlights_create(void) {
    return (struct editor_search_highlights){
        .highlights =
            calloc(sizeof(struct editor_search_highlights), 2),
        .capactiy = sizeof(struct editor_search_highlights) * 2,
        .length = 0};
}

static void editor_ensure_cursor_idx_within_str(struct editor* o) {
    if (o->cursor.row >= o->text.buffer->lines.length) {
        o->cursor.row = o->text.buffer->lines.length - 1;
        o->editor_flags |= editor_flag_cursor_moved;
        o->editor_flags |= editor_flag_cursor_moved_manually;
    }

    struct line* cursor_line =
        &o->text.buffer->lines.data[o->cursor.row];
    size_t cursor_line_len = line_len(cursor_line);

    if (o->cursor.column > cursor_line_len) {
        o->cursor.column = cursor_line_len;
        o->editor_flags |= editor_flag_cursor_moved;
        o->editor_flags |= editor_flag_cursor_moved_manually;
    }
}

static void editor_search_highlights_destroy(
    struct editor_search_highlights* o) {
    free(o->highlights);
    o->highlights = 0;
    o->capactiy = 0;
    o->length = 0;
}

static void editor_search_highlights_clear(
    struct editor_search_highlights* o) {
    o->length = 0;
}

static void editor_search_highlights_push(
    struct editor_search_highlights* o,
    struct selection highlight_selection) {
    size_t required_cap = (o->length + 1) * sizeof(struct selection);

    while (required_cap > o->capactiy) {
        o->capactiy *= 2;
        o->highlights = realloc(o->highlights, o->capactiy);
        assert(o->highlights);
    }

    o->highlights[o->length++] = highlight_selection;
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

static size_t text_get_index_line_num(struct editor* o,
                                      size_t index) {
    assert(index < o->text.buffer->str.size);

    for (size_t i = 0; i < o->text.buffer->lines.length; i += 1)
        if (index >= o->text.buffer->lines.data[i].start &&
            index <= o->text.buffer->lines.data[i].end)
            return i;

    assert(0 && "failed to find line of index");
}

static void editor_search_highlights_find(struct editor* o) {
    if (o->text.buffer->str.size == 0) return;
    assert(o->search_editor.text.buffer->str.size > 0);

    char32_t* sub_str_ptr = 0;
    size_t search_pos = 0;
    while ((sub_str_ptr =
                str32str32(o->search_editor.text.buffer->str.data,
                           o->search_editor.text.buffer->str.size,
                           &o->text.buffer->str.data[search_pos],
                           o->text.buffer->str.size - search_pos))) {
        size_t match_pos = sub_str_ptr - &o->text.buffer->str.data[0];
        size_t match_line_num = text_get_index_line_num(o, match_pos);
        size_t match_column =
            match_pos -
            o->text.buffer->lines.data[match_line_num].start;

        editor_search_highlights_push(
            &o->search_highlights,
            (struct selection){
                .from_line = match_line_num,
                .from_col = match_column,
                .to_line = match_line_num,
                .to_col = match_column +
                          o->search_editor.text.buffer->str.size});

        size_t next_search_pos =
            match_pos + o->search_editor.text.buffer->str.size;
        if (next_search_pos > o->text.buffer->str.size) return;
        search_pos = next_search_pos;
    }
}

void editor_create(struct editor* o) {
    cursor_history_create(&o->cursor_undo);
    cursor_history_create(&o->cursor_redo);
    cursor_history_push(&o->cursor_undo, o->cursor);
    o->text = text_view_create();
    o->search_editor = line_editor_create();
    o->search_editor.text.buffer = malloc(sizeof(struct buffer));
    buffer_create(o->search_editor.text.buffer, utf32_str_create());
    o->search_highlights = editor_search_highlights_create();
}

static void editor_copy_selection_to_clipboard(struct editor* o) {
    assert(o->editor_mode == editor_mode_selection);

    size_t index_begin =
        o->text.buffer->lines.data[o->text.selection.from_line]
            .start +
        o->text.selection.from_col;
    size_t index_end =
        o->text.buffer->lines.data[o->text.selection.to_line].start +
        o->text.selection.to_col;

    size_t count = index_end - index_begin;
    char utf8_selection_str[count + 1];
    utf8_selection_str[count] = 0;
    ff_utf32_to_utf8(utf8_selection_str,
                     &o->text.buffer->str.data[index_begin], count);

    SetClipboardText(utf8_selection_str);
    o->editor_mode = editor_mode_normal;
}

static void editor_paste_from_clipboard(struct editor* o) {
    const char* clipboard_utf8 = GetClipboardText();
    if (!clipboard_utf8) return;

    char32_t clipboard_utf8_len = strlen(clipboard_utf8);
    char32_t clipboard_utf32[clipboard_utf8_len + 1];
    clipboard_utf32[clipboard_utf8_len] = 0;

    int err = ff_utf8_to_utf32(clipboard_utf32, clipboard_utf8,
                               clipboard_utf8_len);
    if (err) return;

    editor_save_undo(o);
    size_t cursor_index =
        o->text.buffer->lines.data[o->cursor.row].start +
        o->cursor.column;
    buffer_insert_buf(o->text.buffer, cursor_index, clipboard_utf32,
                      clipboard_utf8_len);

    for (size_t i = 0; i < clipboard_utf8_len; i += 1) {
        editor_mov_char_right(o);
    }
}

static void editor_delete_selection(struct editor* o) {
    if (!(o->text.text_flags & text_flag_has_selection)) {
        editor_end_selection_mode(o);
    }

    o->cursor.row = o->text.selection.from_line;
    o->cursor.column = o->text.selection.from_col;

    struct line line_beg =
        o->text.buffer->lines.data[o->text.selection.from_line];
    struct line line_end =
        o->text.buffer->lines.data[o->text.selection.to_line];

    size_t beg_index = line_beg.start + o->text.selection.from_col;
    size_t end_index = line_end.start + o->text.selection.to_col;
    size_t count = end_index - beg_index;

    editor_save_undo(o);
    buffer_delete(o->text.buffer, beg_index, count);
    editor_end_selection_mode(o);
}

static void editor_cut_selection(struct editor* o) {
    assert(o->editor_mode == editor_mode_selection);
    editor_copy_selection_to_clipboard(o);
    editor_delete_selection(o);
    editor_end_selection_mode(o);
}

void editor_destroy(struct editor* o) {
    cursor_history_destroy(&o->cursor_undo);
    cursor_history_destroy(&o->cursor_redo);
    buffer_destroy(o->search_editor.text.buffer);
    free(o->search_editor.text.buffer);
    line_editor_destroy(&o->search_editor);
    text_view_destroy(&o->text);
    editor_search_highlights_destroy(&o->search_highlights);
}

static void editor_backspace(struct editor* o) {
    if (o->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(o);
    if (o->editor_mode & editor_mode_selection)
        editor_end_selection_mode(o);

    size_t cursor_position =
        o->text.buffer->lines.data[o->cursor.row].start +
        o->cursor.column;
    if (cursor_position == 0) return;

    buffer_delete(o->text.buffer, cursor_position - 1, 1);
    editor_mov_char_left(o);
}

static void editor_delete(struct editor* o) {
    editor_save_undo(o);

    if (o->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(o);
    size_t cursor_position =
        o->text.buffer->lines.data[o->cursor.row].start +
        o->cursor.column;
    if (cursor_position >= o->text.buffer->str.size) return;
    buffer_delete(o->text.buffer, cursor_position, 1);
}

static void editor_insert_char(struct editor* o, char32_t chr) {
    if (chr == U' ' || chr == U'\n' ||
        o->editor_flags & editor_flag_cursor_moved_manually) {
        editor_save_undo(o);
    }
    buffer_insert_char(
        o->text.buffer,
        o->text.buffer->lines.data[o->cursor.row].start +
            o->cursor.column,
        chr);
    editor_mov_char_right(o);
    o->editor_flags &= ~editor_flag_cursor_moved_manually;
}

static void editor_insert_tab(struct editor* o, uint8_t tab_size) {
    editor_save_undo(o);

    char32_t tab[tab_size];
    for (size_t i = 0; i < tab_size; i += 1) tab[i] = U' ';
    buffer_insert_buf(
        o->text.buffer,
        o->text.buffer->lines.data[o->cursor.row].start +
            o->cursor.column,
        tab, tab_size);
    for (size_t i = 0; i < tab_size; i += 1) editor_mov_char_right(o);
}

static void editor_mov_char_right(struct editor* o) {
    struct line line = o->text.buffer->lines.data[o->cursor.row];
    if (o->cursor.column >= line_len(&line))
        return editor_mov_line_down(o);
    o->cursor.column += 1;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_mov_line_up(struct editor* o) {
    if (o->cursor.row == 0) return editor_mov_char_left(o);
    o->cursor.column = 0;  // TODO
    o->cursor.row -= 1;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_mov_char_left(struct editor* o) {
    if (o->cursor.column == 0) {
        if (o->cursor.row == 0) return;
        editor_mov_line_up(o);
        return editor_mov_end_of_line(o);
    }
    o->cursor.column -= 1;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_mov_beg_of_line(struct editor* o) {
    o->cursor.column = 0;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_mov_end_of_line(struct editor* o) {
    struct line line = o->text.buffer->lines.data[o->cursor.row];
    o->cursor.column = line_len(&line);
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_mov_line_down(struct editor* o) {
    if (o->cursor.row >= o->text.buffer->lines.length - 1) return;
    o->cursor.column = 0;
    o->cursor.row += 1;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_begin_selection_mode(struct editor* o) {
    o->editor_mode = editor_mode_selection;
    o->selection_begin = o->cursor;
}

static void editor_end_selection_mode(struct editor* o) {
    o->editor_mode = editor_mode_normal;
    text_view_clear_selection(&o->text);
}

static void editor_begin_search_input_mode(struct editor* o) {
    o->editor_mode = editor_mode_search_input;
}

static void editor_end_search_input_mode(struct editor* o) {
    o->editor_mode = editor_mode_normal;
}

static void editor_begin_search_mode(struct editor* o) {
    if (!o->search_editor.text.buffer->str.size) {
        o->editor_mode = editor_mode_normal;
        return;
    }
    o->editor_mode = editor_mode_search;
    editor_search_highlights_find(o);
}

static void editor_end_search_mode(struct editor* o) {
    o->editor_mode = editor_mode_normal;
    o->match_select_counter = 0;
    editor_search_highlights_clear(&o->search_highlights);
}

static void editor_next_search_match(struct editor* o) {
    assert(o->editor_mode == editor_mode_search);

    if (o->match_select_counter > o->search_highlights.length - 1)
        o->match_select_counter = 0;

    struct selection highlight =
        o->search_highlights.highlights[o->match_select_counter++];

    o->cursor.row = highlight.from_line;
    o->cursor.column = highlight.from_col;

    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_search_mode_keybinds(struct editor* o) {
    if (IsKeyPressed(KEY_ESCAPE)) editor_end_search_mode(o);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_S)) editor_next_search_match(o);
        if (is_key_sticky(KEY_G)) editor_end_search_mode(o);
        if (is_key_sticky(KEY_SPACE)) {
            editor_end_search_mode(o);
            editor_begin_selection_mode(o);
        }
    }

    int char_pressed = GetCharPressed();
    if (char_pressed) {
        editor_insert_char(o, char_pressed);
        editor_end_search_mode(o);
    }
}

static void editor_movement_keybinds(struct editor* o) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_F)) return editor_mov_char_right(o);
        if (is_key_sticky(KEY_B)) return editor_mov_char_left(o);
        if (is_key_sticky(KEY_A)) return editor_mov_beg_of_line(o);
        if (is_key_sticky(KEY_E)) return editor_mov_end_of_line(o);
        if (is_key_sticky(KEY_N)) return editor_mov_line_down(o);
        if (is_key_sticky(KEY_P)) return editor_mov_line_up(o);
    }
}

static void editor_delete_rest_of_line(struct editor* o) {
    editor_save_undo(o);
    if (o->text.text_flags & text_flag_has_selection)
        return editor_delete_selection(o);

    struct line line = o->text.buffer->lines.data[o->cursor.row];
    size_t start_index = line.start + o->cursor.column;
    size_t delete_len = line.end - start_index;

    buffer_delete(o->text.buffer, start_index, delete_len);
}

static void editor_modifier_keybinds(struct editor* o) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_H)) return editor_backspace(o);
        if (is_key_sticky(KEY_D)) return editor_delete(o);
        if (is_key_sticky(KEY_K))
            return editor_delete_rest_of_line(o);
        if (is_key_sticky(KEY_SLASH)) {
            o->editor_flags |= editor_flag_cursor_moved_manually;
            return editor_undo(o);
        }
        if (IsKeyDown(KEY_LEFT_SHIFT) && is_key_sticky(KEY_U))
            return editor_redo(o);
    }

    if (is_key_sticky(KEY_ENTER)) return editor_insert_char(o, U'\n');
    if (is_key_sticky(KEY_TAB)) return editor_insert_tab(o, 4);
    if (is_key_sticky(KEY_BACKSPACE)) return editor_backspace(o);

    char32_t char_pressed = GetCharPressed();
    if (char_pressed) return editor_insert_char(o, char_pressed);
}

static void editor_normal_mode_keybinds(struct editor* o) {
    editor_movement_keybinds(o);
    editor_modifier_keybinds(o);
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_SPACE))
            return editor_begin_selection_mode(o);
        if (is_key_sticky(KEY_S))
            return editor_begin_search_input_mode(o);
        if (is_key_sticky(KEY_Y))
            return editor_paste_from_clipboard(o);
    }
}

static void editor_move_cursor_on_click(struct editor* o,
                                        struct ff_typography typo,
                                        Rectangle bounds) {
    if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
    Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, bounds)) return;

    ulong hover_line = text_view_get_mouse_hover_line(
        &o->text, typo.size, mouse, bounds.y);
    ulong hover_col = text_view_get_mouse_hover_col(
        &o->text, typo, mouse, bounds.x, hover_line);

    o->cursor.row = hover_line;
    o->cursor.column = hover_col;
    o->editor_flags |= editor_flag_cursor_moved;
    o->editor_flags |= editor_flag_cursor_moved_manually;
}

static void editor_handle_selection_mode_interactions(
    struct editor* o, struct ff_typography typo, Rectangle bounds) {
    editor_move_cursor_on_click(o, typo, bounds);
    editor_movement_keybinds(o);
    if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_G)) ||
        IsKeyPressed(KEY_ESCAPE)) {
        return editor_end_selection_mode(o);
    }
    if ((IsKeyDown(KEY_LEFT_ALT)) && IsKeyPressed(KEY_W)) {
        editor_copy_selection_to_clipboard(o);
        editor_end_selection_mode(o);
    }
    if ((IsKeyDown(KEY_LEFT_CONTROL)) && IsKeyPressed(KEY_W)) {
        editor_cut_selection(o);
        editor_end_selection_mode(o);
    }
    if (o->editor_flags & editor_flag_cursor_moved) {
        text_view_select(&o->text,
                         (struct selection){
                             .from_line = o->selection_begin.row,
                             .from_col = o->selection_begin.column,
                             .to_line = o->cursor.row,
                             .to_col = o->cursor.column,
                         });
    }
}

void editor_handle_search_input_interactions(struct editor* o) {
    if (is_key_sticky(KEY_ESCAPE)) editor_end_search_input_mode(o);

    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_G))
            return editor_end_search_input_mode(o);
    }
    if (IsKeyPressed(KEY_ENTER)) {
        editor_end_search_input_mode(o);
        editor_begin_search_mode(o);
    }
}

static void editor_handle_interactions(struct editor* o,
                                       struct ff_typography typo,
                                       Rectangle bounds) {
    editor_move_cursor_on_click(o, typo, bounds);
    switch (o->editor_mode) {
        case editor_mode_normal:
            return editor_normal_mode_keybinds(o);
        case editor_mode_selection:
            return editor_handle_selection_mode_interactions(o, typo,
                                                             bounds);
        case editor_mode_search_input:
            return editor_handle_search_input_interactions(o);

        case editor_mode_search:
            return editor_search_mode_keybinds(o);
        default:;
    }
}

#define EDITOR_SEARCH_BOX_WIDTH 256
static void editor_draw_search_input(struct editor* o,
                                     struct ff_typography typo,
                                     Rectangle bounds,
                                     int focus_flags) {
    assert(o->editor_mode == editor_mode_search_input);

    Texture search_icon = get_icon(icon_search_t);

    Rectangle bg;
    bg.width = bounds.width < EDITOR_SEARCH_BOX_WIDTH
                   ? bounds.width
                   : EDITOR_SEARCH_BOX_WIDTH;
    bg.x = bounds.x + bounds.width * 0.5f - bg.width * 0.5f;
    bg.y = bounds.height * 0.5f - typo.size * 0.5f;
    bg.height = typo.size + g_cfg.layout.padding * 2;

    DrawRectangleRec(bg, GetColor(g_cfg.color_scheme.surface0_bg));

    Rectangle icon_source = {.x = 0,
                             .y = 0,
                             .width = search_icon.width,
                             .height = search_icon.height};

    bg.x += g_cfg.layout.padding;
    bg.y += g_cfg.layout.padding;
    Rectangle icon_dest = {.x = bg.x,
                           .y = bg.y,
                           .width = search_icon.width,
                           .height = search_icon.height};
    DrawTexturePro(search_icon, icon_source, icon_dest, (Vector2){0},
                   0, WHITE);

    bg.x += search_icon.width + g_cfg.layout.padding;
    bg.width -= search_icon.width + g_cfg.layout.padding * 3;
    line_editor_draw(&o->search_editor, typo, bg, focus_flags);
};

void editor_draw(struct editor* o, struct ff_typography typo,
                 Rectangle bounds, int focus_flags) {
    editor_ensure_cursor_idx_within_str(o);
    if (focus_flags & focus_flag_can_interact) {
        editor_handle_interactions(o, typo, bounds);
    }
    editor_ensure_cursor_idx_within_str(o);

    if (o->search_highlights.length > 0) {
        struct text_search_highlight search_highlights = {
            .highlights = o->search_highlights.highlights,
            .count = o->search_highlights.length};
        text_view_draw_with_cursor(
            &o->text, typo, bounds, o->cursor,
            o->editor_flags & editor_flag_cursor_moved, focus_flags,
            &search_highlights);
    } else {
        text_view_draw_with_cursor(
            &o->text, typo, bounds, o->cursor,
            o->editor_flags & editor_flag_cursor_moved, focus_flags,
            0);
    }

    if (o->editor_mode == editor_mode_search_input) {
        editor_draw_search_input(o, typo, bounds, focus_flags);
    }

    o->editor_flags &= ~editor_flag_cursor_moved;
}

void editor_clear(struct editor* o) {
    buffer_clear(o->text.buffer);
    o->cursor.row = 0;
    o->cursor.column = 0;
}
