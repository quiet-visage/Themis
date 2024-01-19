#pragma once

#include <field_fusion/fieldfusion.h>
#include <raylib.h>
#include <sys/types.h>

#include "../dyn_strings/utf32_string.h"
#include "../focus.h"
#include "../highlighter/highlighter.h"
#include "../motion/motion.h"
#include "cursor.h"
#include "raylib.h"

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

enum text_flags {
    text_flag_none = 0,
    text_flag_has_selection = 0x1,
    text_flag_mouse_was_pressed = 0x2
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
    int text_flags;
    struct cursor cursor;
    struct highlighter highlighter;
    struct tokens tokens;
};

struct text_search_highlight {
    struct selection* highlights;
    size_t count;
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
               Rectangle bounds, int focus_flags);
void text_draw_with_cursor(
    struct text* t, struct ff_typography typo, Rectangle bounds,
    struct text_position pos, bool cursor_moved, int focus_flags,
    struct text_search_highlight* search_highlights);
void text_clear_selection(struct text* t);
void text_clear(struct text* t);
