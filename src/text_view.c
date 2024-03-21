#include "text_view.h"

#include <assert.h>
#include <fieldfusion.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <uchar.h>

#include "config.h"
#include "cursor.h"
#include "highlighter/highlighter.h"
#include "motion.h"

inline static float font_space(float font_size) {
    return font_size + g_cfg.layout.text_spacing;
}
struct text_view text_view_create() {
    struct text_view result = {0};
    result.scroll_motion = motion_new();
    result.scroll_motion.f = 2.0f;
    result.scroll_motion.z = 1.0f;
    result.scroll_motion.r = -1.f;
    result.glyphs = ff_glyphs_vector_create();
    result.buffer = NULL;
    result.cursor = cursor_new();
    return result;
}

void text_view_destroy(struct text_view* m) {
    ff_glyphs_vector_destroy(&m->glyphs);
}

static void utf32_substr(char32_t* dest, char32_t* src, ulong len) {
    memcpy(dest, src, sizeof(char32_t) * len);
}

static int get_token_color(enum token_kind kind) {
    if (kind == token_kind_unknown_t) return g_cfg.color_scheme.fg;
    return g_cfg.color_scheme.syntax[kind];
}

static void highlight_glyph_line(struct text_view* m,
                                 size_t* token_index, size_t len,
                                 size_t line_n) {
    static size_t last_row = -1;
    static size_t last_col = -1;
    for (; *token_index < m->buffer->syntax.tokens.length;
         *token_index += 1) {
        struct token token =
            m->buffer->syntax.tokens.data[*token_index];
        if (token.position.start.row < line_n ||
            (token.position.start.row == last_row &&
             token.position.start.column == last_col) ||
            token.kind == token_kind_unknown_t)
            continue;
        if (token.position.start.row > line_n) break;
        bool is_multiline =
            token.position.start.row != token.position.end.row;
        if (is_multiline) continue;

        size_t highlight_col_beg = token.position.start.column;
        size_t highlight_col_end = token.position.end.column > len
                                       ? len
                                       : token.position.end.column;
        int color = get_token_color(token.kind);
        for (size_t iii = highlight_col_beg; iii < highlight_col_end;
             iii += 1) {
            size_t index = m->glyphs.size - len + iii;
            m->glyphs.data[index].color = color;
        }
        last_row = token.position.start.row;
        last_col = token.position.start.column;
    }
}

void text_view_update_glyphs(struct text_view* m,
                             struct ff_typography typo,
                             Rectangle bounds) {
    if (!m->buffer || m->buffer->str.length == 0) {
        m->glyphs.size = 0;
        return;
    }
    ff_glyphs_vector_clear(&m->glyphs);
    struct ff_position line_position = {.x = bounds.x, .y = bounds.y};

    size_t token_index = 0;
    for (ulong i = 0; i < m->buffer->lines.length; i += 1) {
        if (text_view_is_line_above_view(m, typo, i)) {
            line_position.y += font_space(typo.size);
            continue;
        }
        if (text_view_is_line_below_view(m, typo, bounds, i)) break;

        struct line line = m->buffer->lines.data[i];
        ulong line_length = line_len(&line);
        char32_t line_str[line_length];
        memset(line_str, 0, sizeof(char32_t) * line_length);
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     line_length);
        ff_print_utf32(
            &m->glyphs,
            (struct ff_utf32_str){.data = line_str,
                                  .length = line_length},
            (struct ff_print_params){
                .typography = typo,
                .print_flags = ff_get_default_print_flags(),
                .characteristics = ff_get_default_characteristics(),
                .draw_spaces = false},
            line_position);

        if (m->buffer->syntax.highlighter.language != language_none_t)
            highlight_glyph_line(m, &token_index, line_length, i);
        line_position.y += font_space(typo.size);
    }
}

bool text_view_is_line_below_view(struct text_view* m,
                                  struct ff_typography typo,
                                  Rectangle bounds, ulong line) {
    line -= line == 0 ? 0 : 1;
    float line_pixel_position = typo.size * line + typo.size +
                                g_cfg.layout.text_spacing * line;
    float bottom = bounds.height + m->scroll_motion.position[1];
    return line_pixel_position > bottom;
}

bool text_view_is_line_above_view(struct text_view* m,
                                  struct ff_typography typo,
                                  ulong line) {
    line += 1;
    float line_pixel_pos = line * font_space(typo.size);
    float top = m->scroll_motion.position[1];
    return line_pixel_pos < top;
}

static void swap_ul(ulong* a, ulong* b) {
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *b ^ *a;
}

int text_view_selection_is_invalid(struct text_view* m) {
    if (m->selection.from_line >= m->buffer->lines.length) return -1;
    if (m->selection.to_line >= m->buffer->lines.length) return -1;

    struct line from_buffer_line =
        m->buffer->lines.data[m->selection.from_line];
    if (m->selection.from_col > from_buffer_line.end) return -1;

    struct line to_buffer_line =
        m->buffer->lines.data[m->selection.to_line];
    if (m->selection.to_col > to_buffer_line.end) return -1;

    return 0;
}

void text_view_select(struct text_view* m, struct selection sel) {
    if (sel.from_line > sel.to_line) {
        swap_ul(&sel.from_line, &sel.to_line);
        swap_ul(&sel.from_col, &sel.to_col);
    }

    if (sel.to_line - sel.from_line == 0 &&
        sel.from_col > sel.to_col) {
        swap_ul(&sel.from_col, &sel.to_col);
    }

    m->selection.from_line = sel.from_line;
    m->selection.from_col = sel.from_col;
    m->selection.to_line = sel.to_line;
    m->selection.to_col = sel.to_col;

    assert(m->selection.from_line < m->buffer->lines.length);
    assert(m->selection.to_line < m->buffer->lines.length);

    ulong from_line_index =
        m->buffer->lines.data[sel.from_line].start;
    ulong to_line_index = m->buffer->lines.data[sel.to_line].start;
    ulong from = from_line_index + sel.from_col;
    ulong to = to_line_index + sel.to_col;
    ulong selection_char_count = to - from;

    if (selection_char_count > 0)
        m->text_flags |= text_flag_has_selection;
    else
        m->text_flags &= ~text_flag_has_selection;
}

ulong text_view_get_mouse_hover_line(struct text_view* m,
                                     float font_size, Vector2 mouse,
                                     float y_offset) {
    ulong result = 0;
    mouse.y += m->scroll_motion.position[1];
    for (size_t i = 0; i < m->buffer->lines.length; i += 1) {
        float line_y = i * font_size + y_offset + font_size +
                       i * g_cfg.layout.text_spacing;
        bool mouse_is_in_this_line = line_y > mouse.y;
        if (!mouse_is_in_this_line) continue;
        result = i;
        break;
    }
    return result;
}

ulong text_view_get_mouse_hover_col(struct text_view* m,
                                    struct ff_typography typo,
                                    Vector2 mouse, float x_offset,
                                    ulong hovering_line) {
    assert(m->buffer);
    assert(hovering_line < m->buffer->lines.length);
    struct line matching_line = m->buffer->lines.data[hovering_line];
    ulong matching_line_len = line_len(&matching_line);
    if (matching_line_len == 0) return 0;

    char32_t matching_line_str[matching_line_len + 1];
    matching_line_str[matching_line_len] = 0;
    utf32_substr(matching_line_str,
                 &m->buffer->str.data[matching_line.start],
                 matching_line_len);

    ulong result = 0;
    float character_x = 0;
    for (size_t i = 1; i < matching_line_len; i++) {
        struct ff_utf32_str str = {.data = matching_line_str,
                                   .length = i};
        struct ff_dimensions measurement =
            ff_measure_utf32(typo.font, str, typo.size, true);
        character_x = measurement.width;
        if (character_x > mouse.x - x_offset) break;
        result = i;
    }
    if (mouse.x - x_offset > character_x) result = matching_line_len;

    return result;
}

void text_view_handle_mouse(struct text_view* m,
                            struct ff_typography typo,
                            Rectangle bounds) {
    Vector2 mouse = GetMousePosition();
    float is_within_container = CheckCollisionPointRec(mouse, bounds);
    if (!is_within_container) return;

    bool is_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (is_pressed) {
        m->mouse_press_start_line = text_view_get_mouse_hover_line(
            m, typo.size, mouse, bounds.y);
        m->mouse_press_start_col = text_view_get_mouse_hover_col(
            m, typo, mouse, bounds.x, m->mouse_press_start_line);
        m->text_flags |= text_flag_mouse_was_pressed;
        return;
    }

    bool is_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    if (is_down && m->text_flags & text_flag_mouse_was_pressed) {
        ulong end_mouse_press_line = text_view_get_mouse_hover_line(
            m, typo.size, mouse, bounds.y);
        ulong end_mouse_press_col = text_view_get_mouse_hover_col(
            m, typo, mouse, bounds.x, end_mouse_press_line);
        text_view_select(
            m,
            (struct selection){.from_line = m->mouse_press_start_line,
                               .from_col = m->mouse_press_start_col,
                               .to_line = end_mouse_press_line,
                               .to_col = end_mouse_press_col});
        return;
    }

    bool is_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (m->text_flags & text_flag_mouse_was_pressed && is_released)
        m->text_flags &= ~text_flag_mouse_was_pressed;
}

static size_t min(size_t a, size_t b) { return a < b ? a : b; }

static size_t text_visible_lines_cout(size_t size, float height) {
    size_t acc_size = 0;
    size_t result = 0;
    while ((acc_size + size) < height) {
        acc_size += size;
        result += 1;
    }
    return result;
}

void text_scroll_with_cursor_vertically(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position curs_pos) {
    float glyph_offset = font_space(typo.size);
    size_t max_scroll_off =
        text_visible_lines_cout(font_space(typo.size),
                                bounds.height) *
        .5f;
    size_t scroll_off = min(g_cfg.scroll_off, max_scroll_off);
    {  // bottom scroll
        size_t row = curs_pos.row + scroll_off;
        float cursor_pixel_position = font_space(typo.size) * row;
        bool cursor_is_out_of_view =
            cursor_pixel_position >
            bounds.height - glyph_offset + m->scroll.vertical;
        if (cursor_is_out_of_view) {
            m->scroll.vertical = cursor_pixel_position -
                                 (bounds.height - glyph_offset);
        }
    }
    {  // top scroll
        long row = curs_pos.row - scroll_off;
        row = row > 0 ? row : 0;
        float cursor_pixel_position =
            bounds.y + (row < 1 ? g_cfg.layout.text_spacing
                                : font_space(typo.size) * row);
        float top = bounds.y + m->scroll.vertical;
        bool cursor_is_out_of_view = cursor_pixel_position < top;
        if (cursor_is_out_of_view)
            m->scroll.vertical = font_space(typo.size) * row;
    }
}

void text_scroll_with_cursor_horizontally(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position curs_pos) {
    assert(m->buffer);
    struct line line = m->buffer->lines.data[curs_pos.row];
    size_t line_length = line_len(&line);
    {  // right horizontal scroll
        size_t column = min(curs_pos.column + 5, line_length);
        char32_t line_str[column + 1];
        line_str[column] = 0;
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     column);
        struct ff_utf32_str str = {.data = line_str,
                                   .length = column};
        struct ff_dimensions measurement =
            ff_measure_utf32(typo.font, str, typo.size, true);
        bool cursor_is_out_of_view =
            measurement.width > bounds.width + m->scroll.horizontal;
        if (cursor_is_out_of_view) {
            m->scroll.horizontal = measurement.width - bounds.width;
            return;
        }
    }
    {  // left horizontal scroll
        size_t column = curs_pos.column <= 5 ? curs_pos.column
                                             : curs_pos.column - 5;
        char32_t line_str[column + 1];
        line_str[column] = 0;
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     column);
        struct ff_utf32_str str = {.data = line_str,
                                   .length = column};
        struct ff_dimensions measurement =
            ff_measure_utf32(typo.font, str, typo.size, true);
        bool cursor_is_out_of_view =
            measurement.width < m->scroll.horizontal;
        if (cursor_is_out_of_view)
            m->scroll.horizontal = measurement.width;
    }
}

void text_view_scroll_with_cursor(struct text_view* m,
                                  struct ff_typography typo,
                                  Rectangle bounds,
                                  struct text_position curs_pos) {
    text_scroll_with_cursor_vertically(m, typo, bounds, curs_pos);
    text_scroll_with_cursor_horizontally(m, typo, bounds, curs_pos);
}

void text_view_scroll_with_wheel(struct text_view* t,
                                 struct ff_typography typo,
                                 Rectangle bounds) {
    Vector2 mouse_position = GetMousePosition();
    bool mouse_within_bounds =
        CheckCollisionPointRec(mouse_position, bounds);
    if (!mouse_within_bounds) return;
    float scrolled = GetMouseWheelMove();
    if (!scrolled) return;
    t->scroll.vertical += font_space(typo.size) * scrolled * -4;
    if (t->scroll.vertical < 0) t->scroll.vertical = 0;
    if (t->scroll.vertical >
        font_space(typo.size) * t->buffer->lines.length -
            bounds.height * 0.5f)
        t->scroll.vertical =
            font_space(typo.size) * t->buffer->lines.length -
            bounds.height * 0.5f;
}

static float text_get_cursor_x(struct text_view* m,
                               struct ff_typography typo,
                               Rectangle bounds,
                               struct text_position pos) {
    if (pos.column == 0) return bounds.x;

    struct line cursor_line = m->buffer->lines.data[pos.row];

    char32_t cursor_line_str[pos.column + 1];
    cursor_line_str[pos.column] = 0;
    utf32_substr(cursor_line_str,
                 &m->buffer->str.data[cursor_line.start], pos.column);

    struct ff_utf32_str str = {.data = cursor_line_str,
                               .length = pos.column};
    float result =
        ff_measure_utf32(typo.font, str, typo.size, true).width +
        bounds.x;

    return result;
}

static void text_draw_single_line_selection(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    ulong row, ulong col, ulong length, Color bg_color) {
    if (length == 0) return;
    if (text_view_is_line_above_view(m, typo, row)) return;
    if (text_view_is_line_below_view(m, typo, bounds, row)) return;

    float y = row * font_space(typo.size) + bounds.y +
              -m->scroll_motion.position[1];
    float x = bounds.x;

    struct line line = m->buffer->lines.data[row];
    assert(line_len(&line) >= col + length);

    if (col != 0) {
        char32_t line_str[col + 1];
        line_str[col] = 0;
        utf32_substr(line_str, &m->buffer->str.data[line.start], col);

        struct ff_utf32_str str = {.data = line_str, .length = col};
        x += ff_measure_utf32(typo.font, str, typo.size, true).width;
    }

    ulong start_index = line.start + col;
    char32_t selected_str[length + 1];
    selected_str[length] = 0;
    utf32_substr(selected_str, &m->buffer->str.data[start_index],
                 length);

    struct ff_utf32_str str = {.data = selected_str,
                               .length = length};
    float width =
        ff_measure_utf32(typo.font, str, typo.size, true).width;

    Rectangle selected_area = {
        .x = x,
        .y = y,
        .width = width - m->scroll_motion.position[0],
        .height = font_space(typo.size)};
    DrawRectangleRec(selected_area, bg_color);
}

static void text_draw_selection(struct text_view* m,
                                struct ff_typography typo,
                                struct selection selection,
                                Rectangle bounds, Color bg_color) {
    ulong start_line = selection.from_line;
    ulong end_line = selection.to_line;

    ulong first_visible_line = 0ul;
    while (text_view_is_line_above_view(m, typo, first_visible_line))
        first_visible_line += 1;

    ulong last_visible_line = first_visible_line;
    while (!text_view_is_line_below_view(m, typo, bounds,
                                         last_visible_line))
        last_visible_line += 1;

    if (text_view_is_line_below_view(m, typo, bounds,
                                     first_visible_line))
        last_visible_line -= 1;

    if (text_view_is_line_above_view(m, typo, end_line)) return;
    if (text_view_is_line_below_view(m, typo, bounds, start_line))
        return;

    bool selection_is_out_of_view =
        (start_line > last_visible_line) ||
        (end_line < first_visible_line);
    if (selection_is_out_of_view) return;

    ulong selected_lines_count = end_line - start_line + 1;
    bool is_single_line = selected_lines_count == 1;

    ulong start_col = selection.from_col;
    ulong end_col = selection.to_col;

    if (is_single_line) {
        assert(end_col >= start_col);
        return text_draw_single_line_selection(
            m, typo, bounds, start_line, start_col,
            end_col - start_col, bg_color);
    }

    for (size_t i = start_line; i <= end_line; i++) {
        if (text_view_is_line_above_view(m, typo, i)) continue;
        if (text_view_is_line_below_view(m, typo, bounds, i)) return;

        struct line line = m->buffer->lines.data[i];
        ulong line_length = line_len(&line);

        bool is_first = i == start_line;
        if (is_first) {
            text_draw_single_line_selection(
                m, typo, bounds, i, start_col,
                line_length - start_col, bg_color);
            continue;
        }

        bool is_last = i == end_line;
        if (is_last) {
            text_draw_single_line_selection(m, typo, bounds, i, 0,
                                            end_col, bg_color);
            continue;
        }

        text_draw_single_line_selection(m, typo, bounds, i, 0,
                                        line_length, bg_color);
    }
}

void text_view_draw(struct text_view* m, struct ff_typography typo,
                    Rectangle bounds, int focus_flags) {
    assert(m->buffer && "There should be a buffer to be drawn");
    if ((focus_flags & focus_flag_can_scroll) &&
        CheckCollisionPointRec(GetMousePosition(), bounds))
        text_view_scroll_with_wheel(m, typo, bounds);

    motion_update(
        &m->scroll_motion,
        (float[2]){m->scroll.horizontal, m->scroll.vertical},
        GetFrameTime());

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);
    if (m->text_flags & text_flag_has_selection) {
        text_draw_selection(m, typo, m->selection, bounds,
                            GetColor(g_cfg.color_scheme.text_sel_bg));
        rlDrawRenderBatchActive();
    }

    text_view_update_glyphs(m, typo, bounds);
    float projection[4][4];
    struct ff_ortho_params projection_params = {
        .scr_left = m->scroll_motion.position[0],
        .scr_right = GetScreenWidth() + m->scroll_motion.position[0],
        .scr_bottom =
            GetScreenHeight() + m->scroll_motion.position[1],
        .scr_top = m->scroll_motion.position[1],
        .near = -1.0f,
        .far = 1.0f};
    ff_get_ortho_projection(projection_params, projection);
    ff_draw(typo.font, m->glyphs.data, m->glyphs.size,
            (float*)projection);
    EndScissorMode();
}

static void text_search_highlight_matches(
    struct text_view* m,
    struct text_search_highlight* search_highlights,
    struct ff_typography typo, Rectangle bounds) {
    for (size_t i = 0; i < search_highlights->length; i += 1) {
        text_draw_selection(m, typo, search_highlights->highlights[i],
                            bounds,
                            GetColor(g_cfg.color_scheme.selected_bg));
    }
}

void text_view_draw_with_cursor(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position pos, bool cursor_moved, int focus_flags,
    struct text_search_highlight* search_highlights) {
    assert(m->buffer && "There should be a buffer to be drawn");
    assert(pos.row < m->buffer->lines.length);

    if (cursor_moved)
        text_view_scroll_with_cursor(m, typo, bounds, pos);
    else if ((focus_flags & focus_flag_can_scroll) &&
             CheckCollisionPointRec(GetMousePosition(), bounds)) {
        text_view_scroll_with_wheel(m, typo, bounds);
    }

    motion_update(
        &m->scroll_motion,
        (float[2]){m->scroll.horizontal, m->scroll.vertical},
        GetFrameTime());

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);

    if (m->text_flags & text_flag_has_selection) {
        text_draw_selection(m, typo, m->selection, bounds,
                            GetColor(g_cfg.color_scheme.text_sel_bg));
        rlDrawRenderBatchActive();
    }

    if (search_highlights && search_highlights->length) {
        text_search_highlight_matches(m, search_highlights, typo,
                                      bounds);
        rlDrawRenderBatchActive();
    }

    text_view_handle_mouse(m, typo, bounds);

    if (focus_flags & focus_flag_can_interact) {
        m->cursor.flags |= cursor_flag_focused_t;
    } else {
        m->cursor.flags &= ~cursor_flag_focused_t;
    }

    if (cursor_moved) {
        m->cursor.flags |= cursor_flag_recently_moved_t;
    }

    float cursor_x = text_get_cursor_x(m, typo, bounds, pos) -
                     m->scroll_motion.position[0];
    float cursor_y = pos.row * font_space(typo.size) +
                     -m->scroll_motion.position[1] + bounds.y;
    cursor_draw(&m->cursor, cursor_x, cursor_y);

    text_view_update_glyphs(m, typo, bounds);
    float projection[4][4];
    struct ff_ortho_params projection_params = {
        .scr_left = m->scroll_motion.position[0],
        .scr_right = GetScreenWidth() + m->scroll_motion.position[0],
        .scr_bottom =
            GetScreenHeight() + m->scroll_motion.position[1],
        .scr_top = m->scroll_motion.position[1],
        .near = -1.0f,
        .far = 1.0f};
    ff_get_ortho_projection(projection_params, projection);
    ff_draw(typo.font, m->glyphs.data, m->glyphs.size,
            (float*)projection);
    EndScissorMode();
}

void text_view_clear_selection(struct text_view* m) {
    m->text_flags &= ~text_flag_has_selection;
}
