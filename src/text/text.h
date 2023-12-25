#pragma once

#include <highlighter/highlighter.h>
#include <motion/motion.h>
#include <raylib.h>
#include <sys/types.h>

#include "cursor.h"
#include "field_fusion/fieldfusion.h"
#include "raylib.h"
#include "unicode_string.h"

struct line {
    ulong start;
    ulong end;
};

ulong line_length(struct line line);

struct selection {
    ulong from_line;
    ulong from_col;
    ulong to_line;
    ulong to_col;
};

struct lines_vector {
    struct line* data;
    ulong size;
    ulong capacity;
};

struct scroll {
    float horizontal;
    float vertical;
};

struct text_colors {
    int foreground;
    int syntax[token_kind_count_t];
};

struct text {
    struct ff_glyphs_vector glyphs;
    struct lines_vector lines;
    struct scroll scroll;
    struct motion scroll_motion;
    ulong mouse_press_start_line;
    ulong mouse_press_start_col;
    struct string32 buffer;
    struct selection selection;
    bool has_selection;
    bool mouse_was_pressed;
    struct cursor cursor;
    struct highlighter highlighter;
    struct tokens tokens;
};

struct text text_create();
void text_destroy(struct text*);
void text_update_lines(struct text* t);
void text_update_glyphs(struct text* t, struct ff_typography typo,
                        Rectangle bounds);
void text_update_syntax_tree(struct text* t);
void text_set_syntax_language(struct text* t, enum language lang);
void text_on_modified(struct text* t);
bool text_is_line_below_view(struct text* t,
                             struct ff_typography typo,
                             Rectangle bounds, ulong line);
bool text_is_line_above_view(struct text* t,
                             struct ff_typography typo, ulong line);
void text_select(struct text* t, struct selection sel);
ulong text_get_mouse_hover_line(struct text* t, float font_size,
                                Vector2 mouse, float y_offset);
ulong text_get_mouse_hover_col(struct text* t,
                               struct ff_typography typo,
                               Vector2 mouse, float x_offset,
                               ulong hovering_line);
void text_handle_mouse(struct text* t, struct ff_typography typo,
                       Rectangle bounds);
void text_scroll_with_cursor(struct text* t,
                             struct ff_typography typo,
                             Rectangle bounds,
                             struct text_position curs_pos);
void text_scroll_with_wheel(struct text* t, struct ff_typography typo,
                            Rectangle bounds);
void text_draw(struct text* t, struct ff_typography typo,
               Rectangle bounds);
void text_draw_with_cursor(struct text* t, struct ff_typography typo,
                           Rectangle bounds, struct text_position pos,
                           bool cursor_moved);
