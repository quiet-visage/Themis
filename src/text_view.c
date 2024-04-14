#include "text_view.h"

#include <assert.h>
#include <fieldfusion.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "commands.h"
#include "config.h"
#include "cursor.h"
#include "error_link.h"
#include "focus.h"
#include "highlighter/highlighter.h"
#include "motion.h"
#include "selection.h"

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

static void utf32_substr(c32_t* dest, c32_t* src, ulong len) {
    memcpy(dest, src, sizeof(c32_t) * len);
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
    float line_pos_x = bounds.x;
    float line_pos_y = bounds.y;

    size_t token_index = 0;
    for (ulong i = 0; i < m->buffer->lines.length; i += 1) {
        if (text_view_is_line_above_view(m, typo, i)) {
            line_pos_y += font_space(typo.size);
            continue;
        }
        if (text_view_is_line_below_view(m, typo, bounds, i)) break;

        struct line line = m->buffer->lines.data[i];
        ulong line_length = line_len(&line);
        c32_t* line_str = malloc(line_length * sizeof(c32_t));
        memset(line_str, 0, sizeof(c32_t) * line_length);
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     line_length);
        ff_print_utf32(
            &m->glyphs, line_str, line_length,
            (struct ff_print){
                .typography = typo,
                .options = ff_get_default_print_flags(),
                .characteristics = ff_get_default_characteristics()},
            line_pos_x, line_pos_y);

        if (m->buffer->syntax.highlighter.language != language_none_t)
            highlight_glyph_line(m, &token_index, line_length, i);
        line_pos_y += font_space(typo.size);
        free(line_str);
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

bool text_view_is_line_within_view(struct text_view* m,
                                   struct ff_typography typo,
                                   Rectangle bounds, ulong line) {
    return !(text_view_is_line_below_view(m, typo, bounds, line) ||
             text_view_is_line_above_view(m, typo, line));
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

    c32_t matching_line_str[matching_line_len + 1];
    matching_line_str[matching_line_len] = 0;
    utf32_substr(matching_line_str,
                 &m->buffer->str.data[matching_line.start],
                 matching_line_len);

    ulong result = 0;
    float character_x = 0;
    for (size_t i = 1; i < matching_line_len; i++) {
        struct ff_dimensions measurement = ff_measure_utf32(
            typo.font, matching_line_str, i, typo.size, true);
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
        c32_t line_str[column + 1];
        line_str[column] = 0;
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     column);
        struct ff_dimensions measurement = ff_measure_utf32(
            typo.font, line_str, column, typo.size, true);
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
        c32_t line_str[column + 1];
        line_str[column] = 0;
        utf32_substr(line_str, &m->buffer->str.data[line.start],
                     column);
        struct ff_dimensions measurement = ff_measure_utf32(
            typo.font, line_str, column, typo.size, true);
        bool cursor_is_out_of_view =
            measurement.width < m->scroll.horizontal;
        if (cursor_is_out_of_view)
            m->scroll.horizontal = measurement.width;
    }
}

void text_view_handle_cursor_scrolling(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position curs_pos) {
    text_scroll_with_cursor_vertically(m, typo, bounds, curs_pos);
    text_scroll_with_cursor_horizontally(m, typo, bounds, curs_pos);
}

void text_view_handle_wheel_scrolling(struct text_view* t,
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

    c32_t cursor_line_str[pos.column + 1];
    cursor_line_str[pos.column] = 0;
    utf32_substr(cursor_line_str,
                 &m->buffer->str.data[cursor_line.start], pos.column);

    float result = ff_measure_utf32(typo.font, cursor_line_str,
                                    pos.column, typo.size, true)
                       .width +
                   bounds.x;

    return result;
}

static void text_draw_squiggly_wave(Vector2 start, float width) {
    static const float amplitude = 1.5f;
    static const float wave_length = 10.0f;
    static const float half_wave_length = wave_length / 2;
    size_t point_count = (size_t)(width / half_wave_length);

    Vector2 points[point_count];
    points[0] = start;
    points[1].x = start.x + half_wave_length;
    points[1].y = start.y + amplitude;
    float previous_point_x = points[1].x;
    bool below_equilibrium = false;
    for (size_t i = 2; i < point_count; i += 1) {
        points[i].x = previous_point_x + wave_length;
        points[i].y = start.y;
        if (below_equilibrium)
            points[i].y += amplitude;
        else
            points[i].y -= amplitude;
        below_equilibrium = !below_equilibrium;
        previous_point_x = points[i].x;
    }

    glEnable(GL_STENCIL_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_NEVER, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    DrawRectangle(start.x, start.y - amplitude * 4, width,
                  amplitude * 8, WHITE);
    rlDrawRenderBatchActive();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    DrawSplineCatmullRom(points, point_count, 1.0f, PINK);
    rlDrawRenderBatchActive();
    glDisable(GL_STENCIL_TEST);
}

static Rectangle get_text_dimensions(struct text_view* m,
                                     struct ff_typography typo,
                                     Rectangle bounds, ulong row,
                                     ulong col, ulong length) {
    // NOTE: this does not clip the dimensions within the bounds
    Rectangle result = {.x = bounds.x - m->scroll_motion.position[0],
                        .y = bounds.y + row * font_space(typo.size) -
                             m->scroll_motion.position[1],
                        .width = 0,
                        .height = font_space(typo.size)};

    struct line line = m->buffer->lines.data[row];
    assert(line_len(&line) + col + length);

    if (col != 0) {
        c32_t offset_str[col];
        utf32_substr(offset_str, &m->buffer->str.data[line.start],
                     col);
        result.x += ff_measure_utf32(typo.font, offset_str, col,
                                     typo.size, true)
                        .width;
    }

    ulong start_idx = line.start + col;
    c32_t selected_str[length + 1];
    selected_str[length] = 0;
    utf32_substr(selected_str, &m->buffer->str.data[start_idx],
                 length);
    result.width = ff_measure_utf32(typo.font, selected_str, length,
                                    typo.size, true)
                       .width;

    return result;
}

static void text_draw_line_decoration(struct text_view* m,
                                      struct ff_typography typo,
                                      Rectangle ln_rec,
                                      enum decoration_kind kind) {
    switch (kind) {
        case decoration_squiggly: {
            Vector2 start = {.x = ln_rec.x,
                             .y = ln_rec.y + font_space(typo.size)};
            text_draw_squiggly_wave(start, ln_rec.width);
        } break;
        case decoration_selection: {
            DrawRectangleRec(
                ln_rec, GetColor(g_cfg.color_scheme.selected_bg));
        }
    }
}

static size_t get_selection_dimensions_out_len(
    struct selection selection) {
    assert(selection.from_line <= selection.to_line);
    return selection.to_line - selection.from_line + 1;
}

static bool is_hovering_bulk(Rectangle* recs, size_t recs_len) {
    Vector2 mouse_pos = GetMousePosition();
    for (size_t i = 0; i < recs_len; i += 1)
        if (CheckCollisionPointRec(mouse_pos, recs[i])) return 1;
    return 0;
}

static void get_selection_dimensions(struct text_view* m,
                                     struct ff_typography typo,
                                     struct selection selection,
                                     Rectangle bounds,
                                     Rectangle* out) {
    Rectangle* out_ptr = out;
    ulong start_line = selection.from_line;
    ulong end_line = selection.to_line;
    assert(end_line >= start_line);

    ulong start_col = selection.from_col;
    ulong end_col = selection.to_col;

    size_t out_len = get_selection_dimensions_out_len(selection);
    bool single_line = out_len == 1;
    if (single_line &&
        text_view_is_line_within_view(m, typo, bounds, start_line)) {
        *out_ptr =
            get_text_dimensions(m, typo, bounds, start_line,
                                start_col, end_col - start_col);
        return;
    }

    for (size_t i = start_line; i <= end_line; i += 1) {
        struct line line = m->buffer->lines.data[i];
        ulong line_length = line_len(&line);

        if (i == start_line) {
            *out_ptr =
                get_text_dimensions(m, typo, bounds, i, start_col,
                                    line_length - start_col);
        } else if (i == end_line) {
            *out_ptr =
                get_text_dimensions(m, typo, bounds, i, 0, end_col);
        } else {
            *out_ptr = get_text_dimensions(m, typo, bounds, i, 0,
                                           line_length);
        }

        out_ptr += 1;
    }
}

static void text_draw_selection_decoration(
    struct text_view* m, struct ff_typography typo,
    Rectangle* selection_recs, size_t selection_recs_len,
    enum decoration_kind kind) {
    for (size_t i = 0; i < selection_recs_len; i += 1) {
        text_draw_line_decoration(m, typo, selection_recs[i], kind);
    }
}

static void text_mark_decorations(struct text_view* m,
                                  struct decoration* decorations,
                                  size_t decoration_len,
                                  struct ff_typography typo,
                                  Rectangle bounds) {
    for (size_t i = 0; i < decoration_len; i += 1) {
        for (size_t ii = 0; ii < decorations[i].selections_len;
             ii += 1) {
            struct selection sel = decorations[i].selections[ii];
            size_t sel_recs_len = get_selection_dimensions_out_len(
                decorations[i].selections[ii]);
            Rectangle sel_recs[sel_recs_len];
            get_selection_dimensions(m, typo, sel, bounds, sel_recs);
            text_draw_selection_decoration(
                m, typo, sel_recs, sel_recs_len, decorations[i].kind);
        }
    }
}

static void handle_error_links(struct text_view* m,
                               struct ff_typography typo,
                               Rectangle bounds, int focus_flags,
                               struct error_link* error_links,
                               size_t error_links_len) {
    for (size_t i = 0; i < error_links_len; i += 1) {
        struct error_link* err_link = &error_links[i];

        size_t sel_recs_len = get_selection_dimensions_out_len(
            err_link->link_selection);
        Rectangle sel_recs[sel_recs_len];
        get_selection_dimensions(m, typo, err_link->link_selection,
                                 bounds, sel_recs);

        text_draw_selection_decoration(
            m, typo, sel_recs, sel_recs_len, decoration_squiggly);

        if (!(focus_flags & focus_flag_can_interact)) return;
        if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) return;
        bool hovering = is_hovering_bulk(sel_recs, sel_recs_len);
        if (hovering) {
            cmd_arg_set(command_group_main, main_cmd_open_file_link,
                        &err_link->file_link,
                        sizeof(err_link->file_link));
        };
    }
}

void text_view_draw(struct text_view* m, struct ff_typography typo,
                    Rectangle bounds, int focus_flags,
                    struct decoration* decorations,
                    size_t decorations_len,
                    struct error_link* error_links,
                    size_t error_links_len) {
    assert(m->buffer && "There should be a buffer to be drawn");
    if ((focus_flags & focus_flag_can_scroll) &&
        CheckCollisionPointRec(GetMousePosition(), bounds))
        text_view_handle_wheel_scrolling(m, typo, bounds);

    motion_update(
        &m->scroll_motion,
        (float[2]){m->scroll.horizontal, m->scroll.vertical},
        GetFrameTime());

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);
    if (m->text_flags & text_flag_has_selection) {
        size_t sel_recs_len =
            get_selection_dimensions_out_len(m->selection);
        Rectangle sel_recs[sel_recs_len];
        get_selection_dimensions(m, typo, m->selection, bounds,
                                 sel_recs);
        text_draw_selection_decoration(
            m, typo, sel_recs, sel_recs_len, decoration_selection);
        rlDrawRenderBatchActive();
    }
    if (decorations && decorations_len) {
        text_mark_decorations(m, decorations, decorations_len, typo,
                              bounds);
    }
    if (error_links && error_links_len) {
        for (size_t i = 0; i < error_links_len; i += 1) {
            handle_error_links(m, typo, bounds, focus_flags,
                               error_links, error_links_len);
        }
    }

    text_view_update_glyphs(m, typo, bounds);
    float projection[4][4];
    ff_get_ortho_projection(
        m->scroll_motion.position[0],
        GetScreenWidth() + m->scroll_motion.position[0],
        GetScreenHeight() + m->scroll_motion.position[1],
        m->scroll_motion.position[1], -1.0f, 1.0f, projection);
    ff_draw(typo.font, m->glyphs.data, m->glyphs.size,
            (float*)projection);
    EndScissorMode();
}

void text_view_draw_with_cursor(
    struct text_view* m, struct ff_typography typo, Rectangle bounds,
    struct text_position pos, bool cursor_moved, int focus_flags,
    struct decoration* decorations, size_t decorations_len) {
    assert(m->buffer && "There should be a buffer to be drawn");
    assert(pos.row < m->buffer->lines.length);

    if (cursor_moved)
        text_view_handle_cursor_scrolling(m, typo, bounds, pos);
    else if ((focus_flags & focus_flag_can_scroll) &&
             CheckCollisionPointRec(GetMousePosition(), bounds))
        text_view_handle_wheel_scrolling(m, typo, bounds);

    motion_update(
        &m->scroll_motion,
        (float[2]){m->scroll.horizontal, m->scroll.vertical},
        GetFrameTime());

    BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);

    if (m->text_flags & text_flag_has_selection) {
        size_t sel_recs_len =
            get_selection_dimensions_out_len(m->selection);
        Rectangle sel_recs[sel_recs_len];
        get_selection_dimensions(m, typo, m->selection, bounds,
                                 sel_recs);
        text_draw_selection_decoration(
            m, typo, sel_recs, sel_recs_len, decoration_selection);
    }
    if (decorations && decorations_len) {
        text_mark_decorations(m, decorations, decorations_len, typo,
                              bounds);
    }

    rlDrawRenderBatchActive();

    text_view_handle_mouse(m, typo, bounds);

    if (focus_flags & focus_flag_can_interact)
        m->cursor.flags |= cursor_flag_focused_t;
    else
        m->cursor.flags &= ~cursor_flag_focused_t;

    if (cursor_moved) m->cursor.flags |= cursor_flag_recently_moved_t;

    float cursor_x = text_get_cursor_x(m, typo, bounds, pos) -
                     m->scroll_motion.position[0];
    float cursor_y = pos.row * font_space(typo.size) +
                     -m->scroll_motion.position[1] + bounds.y;
    cursor_draw(&m->cursor, cursor_x, cursor_y);

    text_view_update_glyphs(m, typo, bounds);
    float projection[4][4];
    ff_get_ortho_projection(
        m->scroll_motion.position[0],
        GetScreenWidth() + m->scroll_motion.position[0],
        GetScreenHeight() + m->scroll_motion.position[1],
        m->scroll_motion.position[1], -1.0f, 1.0f, projection);
    ff_draw(typo.font, m->glyphs.data, m->glyphs.size,
            (float*)projection);
    EndScissorMode();
}

void text_view_clear_selection(struct text_view* m) {
    m->text_flags &= ~text_flag_has_selection;
}
