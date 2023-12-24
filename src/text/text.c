#include <assert.h>
#include <field_fusion/fieldfusion.h>
#include <highlighter/highlighter.h>
#include <motion/motion.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <text/cursor.h>
#include <text/text.h>
#include <text/unicode_string.h>
#include <uchar.h>

static const int rosewater = 0xf5e0dcff;
static const int flamingo = 0xf2cdcdff;
static const int pink = 0xf5c2e7ff;
static const int mauve = 0xcba6f7ff;
static const int red = 0xf38ba8ff;
static const int maroon = 0xeba0acff;
static const int peach = 0xfab387ff;
static const int yellow = 0xf9e2afff;
static const int green = 0xa6e3a1ff;
static const int teal = 0x94e2d5ff;
static const int sky = 0x89dcebff;
static const int sapphire = 0x74c7ecff;
static const int blue = 0x89b4faff;
static const int lavender = 0xb4befeff;
static const int text = 0xcdd6f4ff;
static const int overlay2 = 0x9399b2ff;
static const int overlay0 = 0x6c7086ff;
static const float spacing = 6.0f;

static text_colors_t g_colors = {
    .foreground = 0xcdd6f4ff,
    .syntax = {
        [token_kind_keyword_t] = mauve,
        [token_kind_function_t] = blue,
        [token_kind_string_t] = green,
        [token_kind_number_t] = peach,
        [token_kind_operator_t] = sky,
        [token_kind_type_t] = yellow,
        [token_kind_constant_t] = text,
        [token_kind_constant_numeric_t] = peach,
        [token_kind_variable_t] = text,
        [token_kind_variable_parameter_t] = maroon,
        [token_kind_variable_other_member_t] = text,
        [token_kind_delimiter_t] = overlay2,
        [token_kind_property_t] = teal,
        [token_kind_comment_t] = overlay0,
        [token_kind_keyword_directive_t] = mauve,
        [token_keyword_control_return_t] = mauve,
        [token_keyword_control_conditional_t] = mauve,
        [token_keyword_control_repeat_t] = mauve,
        [token_keyword_storage_type_t] = yellow,
        [token_keyword_storage_modifier_t] = mauve,
        [token_keyword_control_t] = mauve,
        [token_type_builtin_t] = red,
        [token_punctuation_t] = overlay2,
        [token_punctuation_delimiter_t] = overlay2,
        [token_punctuation_bracket_t] = overlay2,
        [token_constant_builtin_boolean_t] = peach,
        [token_type_enum_variant_t] = yellow,
        [token_constant_character_t] = peach,
        [token_constant_character_escape_t] = pink,
        [token_label_t] = mauve,

    }};

static float font_space(float font_size) {
    return font_size + spacing;
}
ulong line_length(line_t line) { return line.end - line.start; }
lines_vector_t lines_vector_create() {
    static const ulong initial_prealloc_n = 4;
    lines_vector_t result = {0};
    result.capacity = sizeof(line_t) * initial_prealloc_n;
    result.data = calloc(sizeof(line_t), initial_prealloc_n);
    assert(result.data);
    return result;
}

void lines_vector_destroy(lines_vector_t* l) {
    free(l->data);
    l->data = NULL;
    l->capacity = 0;
    l->size = 0;
}

void lines_vector_clear(lines_vector_t* l) { l->size = 0; }

void lines_vector_push(lines_vector_t* l, line_t line) {
    ulong new_size = sizeof(line_t) * (l->size + 1);

    if (new_size > l->capacity) {
        l->capacity *= 2;
        l->data = realloc(l->data, l->capacity);
        assert(l->data);
    }

    l->data[l->size++] = line;
}

text_t text_create() {
    text_t result = {0};
    result.scroll_motion = motion_new();
    result.scroll_motion.f = 1.5f;
    result.scroll_motion.z = 0.8f;
    result.glyphs = ff_glyphs_vector_create();
    result.lines = lines_vector_create();
    result.buffer = string32_create();
    text_update_lines(&result);
    // string32_insert_char(&result.buffer, 0, U'\n');

    result.cursor = cursor_new();
    result.highlighter =
        hlr_highlighter_create(language_none_t, NULL, 0);
    result.tokens = hlr_tokens_create();
    return result;
}

void text_set_syntax_language(text_t* t, language_t lang) {
    t->highlighter.language = lang;
}

void text_destroy(text_t* t) {
    ff_glyphs_vector_destroy(&t->glyphs);
    lines_vector_destroy(&t->lines);
    string32_destroy(&t->buffer);
    hlr_tokens_destroy(&t->tokens);
    hlr_highlighter_destroy(&t->highlighter);
}

void text_update_lines(text_t* t) {
    lines_vector_clear(&t->lines);
    ulong line_start = 0;
    ulong line_end = 0;
    for (ulong i = 0; i < t->buffer.size; i += 1) {
        if (t->buffer.data[i] != '\n') continue;
        line_end = i;
        lines_vector_push(&t->lines, (line_t){.start = line_start,
                                              .end = line_end});
        line_start = i + 1;
    }
    line_end = t->buffer.size;
    lines_vector_push(&t->lines,
                      (line_t){.start = line_start, .end = line_end});
}

void text_update_syntax_tree(text_t* t) {
    char utf8_buffer[t->buffer.size + 1];
    if (t->highlighter.language != language_none_t) {
        utf8_buffer[t->buffer.size] = 0;
        ff_utf32_to_utf8(utf8_buffer, t->buffer.data, t->buffer.size);
        hlr_highlighter_update(&t->highlighter, utf8_buffer,
                               t->buffer.size);
        hlr_tokens_update(&t->highlighter, &t->tokens);
    }
}

void text_on_modified(text_t* t) {
    text_update_lines(t);
    text_update_syntax_tree(t);
}

static void utf32_substr(char32_t* dest, char32_t* src, ulong len) {
    memcpy(dest, src, sizeof(char32_t) * len);
}

static int get_token_color(token_kind_t kind) {
    if (kind == token_kind_unknown_t) return g_colors.foreground;
    return g_colors.syntax[kind];
}

static void highlight_glyph_line(text_t* t, size_t* token_index,
                                 size_t len, size_t line_n) {
    static size_t last_row = -1;
    static size_t last_col = -1;
    for (; *token_index < t->tokens.size; *token_index += 1) {
        token_t token = t->tokens.data[*token_index];
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
            size_t index = t->glyphs.size - len + iii;
            t->glyphs.data[index].color = color;
        }
        last_row = token.position.start.row;
        last_col = token.position.start.column;
    }
}

void text_update_glyphs(text_t* t, ff_typography_t typo,
                        Rectangle bounds) {
    ff_glyphs_vector_clear(&t->glyphs);
    ff_position_t line_position = {.x = bounds.x, .y = bounds.y};

    size_t token_index = 0;
    for (ulong i = 0; i < t->lines.size; i += 1) {
        if (text_is_line_above_view(t, typo, i)) {
            line_position.y += font_space(typo.size);
            continue;
        }
        if (text_is_line_below_view(t, typo, bounds, i)) break;

        line_t line = t->lines.data[i];
        ulong line_len = line_length(line);
        char32_t line_str[line_len];
        memset(line_str, 0, sizeof(char32_t) * line_len);
        utf32_substr(line_str, &t->buffer.data[line.start], line_len);
        ff_print_utf32(
            &t->glyphs,
            (ff_utf32_str_t){.data = line_str, .size = line_len},
            (ff_print_params_t){
                .typography = typo,
                .print_flags = ff_get_default_print_flags(),
                .characteristics = ff_get_default_characteristics(),
                .draw_spaces = false},
            line_position);

        if (t->highlighter.language != language_none_t)
            highlight_glyph_line(t, &token_index, line_len, i);
        line_position.y += font_space(typo.size);
    }
}

bool text_is_line_below_view(text_t* t, ff_typography_t typo,
                             Rectangle bounds, ulong line) {
    line -= line == 0 ? 0 : 1;
    float line_pixel_position =
        typo.size * line + typo.size + spacing * line;
    float bottom = bounds.height + t->scroll_motion.position[1];
    return line_pixel_position > bottom;
}

bool text_is_line_above_view(text_t* t, ff_typography_t typo,
                             ulong line) {
    line += 1;
    float line_pixel_pos = line * font_space(typo.size);
    float top = t->scroll_motion.position[1];
    return line_pixel_pos < top;
}

static void swap_ul(ulong* a, ulong* b) {
    *a = *a ^ *b;
    *b = *a ^ *b;
    *a = *b ^ *a;
}

void text_select(text_t* t, selection_t sel) {
    if (sel.from_line > sel.to_line) {
        swap_ul(&sel.from_line, &sel.to_line);
        swap_ul(&sel.from_col, &sel.to_col);
    }

    if (sel.to_line - sel.from_line == 0 &&
        sel.from_col > sel.to_col) {
        swap_ul(&sel.from_col, &sel.to_col);
    }

    t->selection.from_line = sel.from_line;
    t->selection.from_col = sel.from_col;
    t->selection.to_line = sel.to_line;
    t->selection.to_col = sel.to_col;

    assert(t->selection.from_line < t->lines.size);
    assert(t->selection.to_line < t->lines.size);

    ulong from_line_index = t->lines.data[sel.from_line].start;
    ulong to_line_index = t->lines.data[sel.to_line].start;
    ulong from = from_line_index + sel.from_col;
    ulong to = to_line_index + sel.to_col;
    ulong selection_char_count = to - from;

    if (selection_char_count > 0) t->has_selection = true;
}

ulong text_get_mouse_hover_line(text_t* t, float font_size,
                                Vector2 mouse, float y_offset) {
    ulong result = 0;
    mouse.y += t->scroll_motion.position[1];
    for (size_t i = 0; i < t->lines.size; i += 1) {
        float line_y =
            i * font_size + y_offset + font_size + i * spacing;
        bool mouse_is_in_this_line = line_y > mouse.y;
        if (!mouse_is_in_this_line) continue;
        result = i;
        break;
    }
    return result;
}

ulong text_get_mouse_hover_col(text_t* t, ff_typography_t typo,
                               Vector2 mouse, float x_offset,
                               ulong hovering_line) {
    assert(hovering_line < t->lines.size);
    line_t matching_line = t->lines.data[hovering_line];
    ulong matching_line_len = line_length(matching_line);
    if (matching_line_len == 0) return 0;
    char32_t matching_line_str[matching_line_len];
    utf32_substr(matching_line_str,
                 &t->buffer.data[matching_line.start],
                 matching_line_len);

    ulong result = 0;
    float character_x = x_offset;
    for (size_t i = 1; i < matching_line_len; i++) {
        ff_dimensions_t measurement = ff_measure(
            typo.font, matching_line_str, i, typo.size, true);
        character_x = measurement.width;
        if (character_x > mouse.x) break;
        result = i;
    }
    if (mouse.x > character_x) result = matching_line_len;

    return result;
}

void text_handle_mouse(text_t* t, ff_typography_t typo,
                       Rectangle bounds) {
    Vector2 mouse = GetMousePosition();
    float is_within_container = CheckCollisionPointRec(mouse, bounds);
    if (!is_within_container) return;

    bool is_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (is_pressed) {
        t->mouse_press_start_line =
            text_get_mouse_hover_line(t, typo.size, mouse, bounds.y);
        t->mouse_press_start_col = text_get_mouse_hover_col(
            t, typo, mouse, bounds.x, t->mouse_press_start_line);
        t->mouse_was_pressed = true;
        return;
    }

    bool is_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    if (is_down && t->mouse_was_pressed) {
        ulong end_mouse_press_line =
            text_get_mouse_hover_line(t, typo.size, mouse, bounds.y);
        ulong end_mouse_press_col = text_get_mouse_hover_col(
            t, typo, mouse, bounds.x, end_mouse_press_line);
        text_select(
            t, (selection_t){.from_line = t->mouse_press_start_line,
                             .from_col = t->mouse_press_start_col,
                             .to_line = end_mouse_press_line,
                             .to_col = end_mouse_press_col});
        return;
    }

    bool is_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (t->mouse_was_pressed && is_released)
        t->mouse_was_pressed = false;
}

void text_scroll_with_cursor(text_t* t, ff_typography_t typo,
                             Rectangle bounds, position_t curs_pos) {
    float glyph_offset = font_space(typo.size);
    {  // bottom scroll
        float cursor_pixel_position =
            font_space(typo.size) * curs_pos.row;
        bool cursor_is_out_of_view =
            cursor_pixel_position >
            bounds.height - glyph_offset + t->scroll.vertical;
        if (cursor_is_out_of_view)
            t->scroll.vertical = cursor_pixel_position -
                                 (bounds.height - glyph_offset);
    }
    {  // top scroll
        float cursor_pixel_position =
            bounds.y + font_space(typo.size) * curs_pos.row;
        float top = bounds.y + t->scroll.vertical;
        bool cursor_is_out_of_view = cursor_pixel_position < top;
        if (cursor_is_out_of_view)
            t->scroll.vertical -= top - cursor_pixel_position;
    }

    line_t line = t->lines.data[curs_pos.row];
    ulong line_len = line_length(line);
    char32_t line_str[line_len];
    memset(line_str, 0, sizeof(char32_t) * line_len);
    utf32_substr(line_str, &t->buffer.data[line.start], line_len);
    ff_dimensions_t measurement =
        ff_measure(typo.font, line_str, line_len, typo.size, true);
    {  // right horizontal scroll
        bool cursor_is_out_of_view =
            measurement.width > bounds.width + t->scroll.horizontal;
        if (cursor_is_out_of_view)
            t->scroll.horizontal += font_space(typo.size);
    }
    {  // left horizontal scroll
        bool cursor_is_out_of_view =
            measurement.width < t->scroll.horizontal;
        if (cursor_is_out_of_view)
            t->scroll.horizontal -= font_space(typo.size);
    }
}

void text_scroll_with_wheel(text_t* t, ff_typography_t typo,
                            Rectangle bounds) {
    Vector2 mouse_position = GetMousePosition();
    bool mouse_within_bounds =
        CheckCollisionPointRec(mouse_position, bounds);
    if (!mouse_within_bounds) return;
    float scrolled = GetMouseWheelMove();
    if (!scrolled) return;
    t->scroll.vertical += font_space(typo.size) * scrolled;
    if (t->scroll.vertical < 0) t->scroll.vertical = 0;
    if (t->scroll.vertical >
        font_space(typo.size) * t->lines.size - bounds.height * 0.5f)
        t->scroll.vertical = font_space(typo.size) * t->lines.size -
                             bounds.height * 0.5f;
}

static float text_get_cursor_x(text_t* t, ff_typography_t typo,
                               Rectangle bounds, position_t pos) {
    if (pos.column == 0) return bounds.x;

    line_t cursor_line = t->lines.data[pos.row];

    char32_t cursor_line_str[pos.column + 1];
    cursor_line_str[pos.column] = 0;
    utf32_substr(cursor_line_str, &t->buffer.data[cursor_line.start],
                 pos.column);

    float result = ff_measure(typo.font, cursor_line_str, pos.column,
                              typo.size, true)
                       .width +
                   bounds.x;

    return result;
}

static void text_draw_single_line_selection(text_t* t,
                                            ff_typography_t typo,
                                            Rectangle bounds,
                                            ulong row, ulong col,
                                            ulong length) {
    if (length == 0) return;
    if (text_is_line_above_view(t, typo, row)) return;
    if (text_is_line_below_view(t, typo, bounds, row)) return;

    float y = row * font_space(typo.size) + bounds.y +
              -t->scroll_motion.position[1];
    float x = bounds.x;

    line_t line = t->lines.data[row];
    assert(line_length(line) >= col + length);

    if (col != 0) {
        char32_t line_str[col + 1];
        line_str[col] = 0;
        utf32_substr(line_str, &t->buffer.data[line.start], col);

        x += ff_measure(typo.font, line_str, col, typo.size, true)
                 .width;
    }

    ulong start_index = line.start + col;
    char32_t selected_str[length + 1];
    selected_str[length] = 0;
    utf32_substr(selected_str, &t->buffer.data[start_index], length);

    float width =
        ff_measure(typo.font, selected_str, length, typo.size, true)
            .width;

    Rectangle selected_area = {.x = x,
                               .y = y,
                               .width = width,
                               .height = font_space(typo.size)};
    DrawRectangleRec(selected_area, GRAY);
}

static void text_draw_selection(text_t* t, ff_typography_t typo,
                                Rectangle bounds) {
    assert(t->has_selection);

    ulong start_line = t->selection.from_line;
    ulong end_line = t->selection.to_line;

    ulong first_visible_line = 0ul;
    while (text_is_line_above_view(t, typo, first_visible_line))
        first_visible_line += 1;

    ulong last_visible_line = first_visible_line;
    while (
        !text_is_line_below_view(t, typo, bounds, last_visible_line))
        last_visible_line += 1;

    if (text_is_line_below_view(t, typo, bounds, first_visible_line))
        last_visible_line -= 1;

    if (text_is_line_above_view(t, typo, end_line)) return;
    if (text_is_line_below_view(t, typo, bounds, start_line)) return;

    bool selection_is_out_of_view =
        (start_line > last_visible_line) ||
        (end_line < first_visible_line);
    if (selection_is_out_of_view) return;

    ulong selected_lines_count = end_line - start_line + 1;
    bool is_single_line = selected_lines_count == 1;

    ulong start_col = t->selection.from_col;
    ulong end_col = t->selection.to_col;

    if (is_single_line) {
        assert(end_col >= start_col);
        return text_draw_single_line_selection(t, typo, bounds,
                                               start_line, start_col,
                                               end_col - start_col);
    }

    for (size_t i = start_line; i <= end_line; i++) {
        if (text_is_line_above_view(t, typo, i)) continue;
        if (text_is_line_below_view(t, typo, bounds, i)) return;

        line_t line = t->lines.data[i];
        ulong line_len = line_length(line);

        bool is_first = i == start_line;
        if (is_first) {
            text_draw_single_line_selection(
                t, typo, bounds, i, start_col, line_len - start_col);
            continue;
        }

        bool is_last = i == end_line;
        if (is_last) {
            text_draw_single_line_selection(t, typo, bounds, i, 0,
                                            end_col);
            continue;
        }

        text_draw_single_line_selection(t, typo, bounds, i, 0,
                                        line_len);
    }
}

void text_draw(text_t* t, ff_typography_t typo, Rectangle bounds) {
    text_scroll_with_wheel(t, typo, bounds);
    motion_update(
        &t->scroll_motion,
        (float[2]){t->scroll.horizontal, t->scroll.vertical},
        GetFrameTime());

    if (t->has_selection) {
        text_draw_selection(t, typo, bounds);
        rlDrawRenderBatchActive();
    }

    text_update_glyphs(t, typo, bounds);
    float projection[4][4];
    ortho_projection_params_t projection_params = {
        .scr_left = 0,
        .scr_right = GetScreenWidth(),
        .scr_bottom = GetScreenHeight(),
        .scr_top = 0,
        .near = -1.0f,
        .far = 1.0f};
    ff_get_ortho_projection(projection_params, projection);
    ff_draw(typo.font, t->glyphs.data, t->glyphs.size,
            (float*)projection);
}

void text_draw_with_cursor(text_t* t, ff_typography_t typo,
                           Rectangle bounds, position_t pos,
                           bool cursor_moved) {
    assert(pos.row < t->lines.size);

    if (cursor_moved)
        text_scroll_with_cursor(t, typo, bounds, pos);
    else
        text_scroll_with_wheel(t, typo, bounds);

    motion_update(
        &t->scroll_motion,
        (float[2]){t->scroll.horizontal, t->scroll.vertical},
        GetFrameTime());

    if (t->has_selection) {
        text_draw_selection(t, typo, bounds);
        rlDrawRenderBatchActive();
    }

    text_handle_mouse(t, typo, bounds);
    float cursor_x = text_get_cursor_x(t, typo, bounds, pos);
    float cursor_y = pos.row * font_space(typo.size) +
                     -t->scroll_motion.position[1] + bounds.y;
    cursor_draw(&t->cursor, cursor_x, cursor_y);

    text_update_glyphs(t, typo, bounds);
    float projection[4][4];
    ortho_projection_params_t projection_params = {
        .scr_left = 0,
        .scr_right = GetScreenWidth(),
        .scr_bottom =
            GetScreenHeight() + t->scroll_motion.position[1],
        .scr_top = t->scroll_motion.position[1],
        .near = -1.0f,
        .far = 1.0f};
    ff_get_ortho_projection(projection_params, projection);
    ff_draw(typo.font, t->glyphs.data, t->glyphs.size,
            (float*)projection);
}
