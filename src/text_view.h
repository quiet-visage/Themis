#pragma once

#include <fieldfusion.h>
#include <raylib.h>
#include <sys/types.h>

#include "buffer/buffer.h"
#include "cursor.h"
#include "dyn_strings/utf32_string.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "motion.h"
#include "raylib.h"

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

struct text_view {
    struct ff_glyphs_vector glyphs;
    struct scroll scroll;
    struct motion scroll_motion;
    ulong mouse_press_start_line;
    ulong mouse_press_start_col;
    struct buffer* buffer;
    struct selection selection;
    int text_flags;
    struct cursor cursor;
};

struct text_search_highlight {
    struct selection* highlights;
    size_t length;
};

struct text_view text_view_create();
void text_view_destroy(struct text_view* m);
void text_view_update_glyphs(struct text_view* m,
                             struct ff_typography typo,
                             Rectangle bounds);
bool text_view_is_line_below_view(struct text_view* m,
                                  struct ff_typography typo,
                                  Rectangle bounds, ulong line);
bool text_view_is_line_above_view(struct text_view* m,
                                  struct ff_typography typo,
                                  ulong line);
void text_view_select(struct text_view* m, struct selection sel);
int text_view_selection_is_invalid(struct text_view* m);
ulong text_view_get_mouse_hover_line(struct text_view* m,
                                     float font_size, Vector2 mouse,
                                     float y_offset);
ulong text_view_get_mouse_hover_col(struct text_view* m,
                                    struct ff_typography typo,
                                    Vector2 mouse, float x_offset,
                                    ulong hovering_line);
void text_view_handle_mouse(struct text_view* m,
                            struct ff_typography typo,
                            Rectangle bounds);
void text_view_scroll_with_cursor(struct text_view* m,
                                  struct ff_typography typo,
                                  Rectangle bounds,
                                  struct text_position curs_pos);
void text_view_scroll_with_wheel(struct text_view* m,
                                 struct ff_typography typo,
                                 Rectangle bounds);
void text_view_draw(struct text_view* m, struct ff_typography typo,
                    Rectangle bounds, int focus_flags);
void text_view_draw_with_cursor(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position pos, bool cursor_moved, int focus_flags,
    struct text_search_highlight* search_highlights);
void text_view_clear_selection(struct text_view* m);
