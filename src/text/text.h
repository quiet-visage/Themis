#pragma once

#include <highlighter/highlighter.h>
#include <motion/motion.h>
#include <raylib.h>
#include <sys/types.h>

#include "cursor.h"
#include "field_fusion/fieldfusion.h"
#include "raylib.h"
#include "unicode_string.h"

typedef struct {
    ulong start;
    ulong end;
} line_t;

ulong line_length(line_t line);

typedef struct {
    ulong from_line;
    ulong from_col;
    ulong to_line;
    ulong to_col;
} selection_t;

typedef struct {
    line_t* data;
    ulong size;
    ulong capacity;
} lines_vector_t;

typedef struct {
    float horizontal;
    float vertical;
} scroll_t;

typedef struct {
    int foreground;
    int syntax[token_kind_count_t];
} text_colors_t;

typedef struct {
    ff_glyphs_vector_t glyphs;
    lines_vector_t lines;
    scroll_t scroll;
    motion_t scroll_motion;
    ulong mouse_press_start_line;
    ulong mouse_press_start_col;
    string32_t buffer;
    selection_t selection;
    bool has_selection;
    bool mouse_was_pressed;
    cursor_t cursor;
    highlighter_t highlighter;
    tokens_t tokens;
} text_t;

text_t text_create();
void text_destroy(text_t*);
void text_update_lines(text_t* t);
void text_update_glyphs(text_t* t, ff_typography_t typo,
                        Rectangle bounds);
void text_update_syntax_tree(text_t* t);
void text_set_syntax_language(text_t* t, language_t lang);
void text_on_modified(text_t* t);
bool text_is_line_below_view(text_t* t, ff_typography_t typo,
                             Rectangle bounds, ulong line);
bool text_is_line_above_view(text_t* t, ff_typography_t typo,
                             ulong line);
void text_select(text_t* t, selection_t sel);
ulong text_get_mouse_hover_line(text_t* t, float font_size,
                                Vector2 mouse, float y_offset);
ulong text_get_mouse_hover_col(text_t* t, ff_typography_t typo,
                               Vector2 mouse, float x_offset,
                               ulong hovering_line);
void text_handle_mouse(text_t* t, ff_typography_t typo,
                       Rectangle bounds);
void text_scroll_with_cursor(text_t* t, ff_typography_t typo,
                             Rectangle bounds, position_t curs_pos);
void text_scroll_with_wheel(text_t* t, ff_typography_t typo,
                            Rectangle bounds);
void text_draw(text_t* t, ff_typography_t typo, Rectangle bounds);
void text_draw_with_cursor(text_t* t, ff_typography_t typo,
                           Rectangle bounds, position_t pos,
                           bool cursor_moved);
