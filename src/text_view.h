#pragma once

#include <fieldfusion.h>
#include <raylib.h>
#include <sys/types.h>

#include "buffer/buffer.h"
#include "cursor.h"
#include "error_link.h"
#include "highlighter/highlighter.h"
#include "motion.h"
#include "raylib.h"
#include "selection.h"

typedef struct {
    float horizontal;
    float vertical;
} scroll_t;

enum decoration_kind { decoration_selection, decoration_squiggly };

typedef struct {
    enum decoration_kind kind;
    selection_t* selections;
    size_t selections_len;
} decoration_t;

enum text_flags {
    text_flag_none = 0,
    text_flag_has_selection = 0x1,
    text_flag_mouse_was_pressed = 0x2
};

typedef struct {
    ff_glyph_vec_t glyphs;
    scroll_t scroll;
    motion_t scroll_motion;
    ulong mouse_press_start_line;
    ulong mouse_press_start_col;
    buffer_t* buffer;
    selection_t selection;
    int text_flags;
    cursor_t cursor;
} text_view_t;

text_view_t text_view_create();
void text_view_destroy(text_view_t* m);
void text_view_update_glyphs(text_view_t* m, ff_typo_t typo,
                             Rectangle bounds);
bool text_view_is_line_below_view(text_view_t* m, ff_typo_t typo,
                                  Rectangle bounds, ulong line);
bool text_view_is_line_above_view(text_view_t* m, ff_typo_t typo,
                                  ulong line);
void text_view_select(text_view_t* m, selection_t sel);
int text_view_selection_is_invalid(text_view_t* m);
ulong text_view_get_mouse_hover_line(text_view_t* m, float font_size,
                                     Vector2 mouse, float y_offset);
ulong text_view_get_mouse_hover_col(text_view_t* m, ff_typo_t typo,
                                    Vector2 mouse, float x_offset,
                                    ulong hovering_line);
void text_view_handle_mouse(text_view_t* m, ff_typo_t typo,
                            Rectangle bounds);
void text_view_handle_cursor_scrolling(text_view_t* m, ff_typo_t typo,
                                       Rectangle bounds,
                                       text_pos_t curs_pos);
void text_view_handle_wheel_scrolling(text_view_t* m, ff_typo_t typo,
                                      Rectangle bounds);
void text_view_draw(text_view_t* m, ff_typo_t typo, Rectangle bounds,
                    int focus_flags, decoration_t* decorations,
                    size_t decorations_len, error_link_t* error_links,
                    size_t error_links_len);
void text_view_draw_with_cursor(text_view_t* m, ff_typo_t typo,
                                Rectangle bounds, text_pos_t pos,
                                bool cursor_moved, int focus_flags,
                                decoration_t* decorations,
                                size_t decorations_len);
void text_view_clear_selection(text_view_t* m);
