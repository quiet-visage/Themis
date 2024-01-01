#include "fuzzy_menu.h"

#include <config/config.h>
#include <field_fusion/fieldfusion.h>
#include <math.h>
#include <menu/fuzzy_menu.h>
#include <motion/motion.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <text/editor.h>
#include <uchar.h>

#define MENU_MIN_WIDTH 124.0f
#define MENU_PADDING 18.0f
#define MENU_MAX_HEIGHT_PERC 0.55f
#define MENU_EDITOR_OPTIONS_SPACE 6.0f

static uint min(uint a, uint b) { return a < b ? a : b; }

static uint edit_distance(const char32_t* s1, size_t len1,
                          const char32_t* s2, size_t len2) {
    size_t score = 0;

    for (size_t i = 0; i < len1; i += 1) {
        if (i >= len2) break;

        if (s1[i] == s2[i]) score += 80;
    }

    for (size_t i = 0; i < len1; i += 1) {
        for (size_t ii = 0; ii < len2; ii += 1) {
            if (s1[i] == s2[ii]) score += 1;
        }
    }

    return score;

    // uint d[len1 + 1][len2 + 1];

    // d[0][0] = 0;
    // for (unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
    // for (unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

    // for (unsigned int i = 1; i <= len1; ++i)
    //     for (unsigned int j = 1; j <= len2; ++j) {
    //         d[i][j] = min(
    //             min(d[i - 1][j] + 1, d[i][j - 1] + 1),
    //             d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 :
    //             1));
    //     }
    // return d[len1][len2];
}

static size_t char32_str_len(const char32_t* str) {
    const char32_t* ptr = str;
    size_t result = 0;
    while (*ptr != 0) {
        result += 1;
        ptr += 1;
    }
    return result;
}

static bool is_key_sticky(int key) {
    return IsKeyPressed(key) || IsKeyPressedRepeat(key);
}

int compare(const void* element_1, const void* element_2) {
    struct fuzzy_menu_option* a =
        (struct fuzzy_menu_option*)element_1;
    struct fuzzy_menu_option* b =
        (struct fuzzy_menu_option*)element_2;

    if (a->edit_distance < b->edit_distance) return -1;
    if (a->edit_distance > b->edit_distance) return 1;
    return 0;
}

struct fuzzy_menu fuzzy_menu_create() {
    struct fuzzy_menu result = {.editor = editor_create(),
                                .vertical_scroll = 0,
                                .glyphs = ff_glyphs_vector_create(),
                                .previous_buffer_size = 0,
                                .selected = 0,
                                .options = {{0}},
                                .options_count = 0,
                                .motion = motion_new()};
    result.editor.editor_flags |= editor_flag_single_line_mode_t;
    result.motion.f = 1.9f;
    result.motion.z = 0.9f;
    return result;
}

void fuzzy_menu_destroy(struct fuzzy_menu* fm) {
    editor_destroy(&fm->editor);
    ff_glyphs_vector_destroy(&fm->glyphs);
}

static void fuzzy_menu_auto_scroll(struct fuzzy_menu* fm,
                                   float options_y, float size,
                                   float selected_y,
                                   float options_height) {
    float cursor_pixel_pos = selected_y;
    // TODO
    {  // top scroll
        float top_position = options_y + fm->vertical_scroll + size;
        if (cursor_pixel_pos < top_position) {
            float offset = top_position - cursor_pixel_pos;
            fm->vertical_scroll -= offset;
        }
    }
    {  // bottom scroll
        float cursor_offset_position =
            cursor_pixel_pos - fm->vertical_scroll;
        float bottom_position = options_height + options_y - size;
        if (cursor_offset_position > bottom_position) {
            float offset = cursor_pixel_pos - bottom_position;
            fm->vertical_scroll = offset;
        }
    }
}

static float largest_string_width(size_t count,
                                  struct fuzzy_menu_option* options,
                                  struct ff_typography typo) {
    float result = 0;
    for (size_t i = 0; i < count; i += 1) {
        float str_width = ff_measure(typo.font, options[i].name,
                                     char32_str_len(options[i].name),
                                     typo.size, true)
                              .width;
        result = fmaxf(result, str_width);
    }
    return result;
}

static Vector2 center(Vector2 what, Vector2 where) {
    return (Vector2){.x = where.x * 0.5f - what.x * 0.5f,
                     .y = where.y * 0.5f - what.y * 0.5f};
}

void fuzzy_menu_push_option(struct fuzzy_menu* fm,
                            const char32_t* option, size_t len) {
    assert(len < FUZZY_MENU_OPTION_NAME_CAP);
    fm->options[fm->options_count].name_len = len;
    fm->options[fm->options_count].icon = icon_none_t;
    memcpy(fm->options[fm->options_count++].name, option,
           len * sizeof(char32_t));
    assert(fm->options_count < FUZZY_MENU_OPTIONS_CAP);
}

void fuzzy_menu_push_option_with_icon(struct fuzzy_menu* fm,
                                      const char32_t* option,
                                      size_t len, enum icon icon) {
    assert(len < FUZZY_MENU_OPTION_NAME_CAP);
    fm->options[fm->options_count].name_len = len;
    fm->options[fm->options_count].icon = icon;
    memcpy(fm->options[fm->options_count++].name, option,
           len * sizeof(char32_t));
    assert(fm->options_count < FUZZY_MENU_OPTIONS_CAP);
}

static void quick_sort_options(struct fuzzy_menu_option options[],
                               int low, int high) {
    if (low < high) {
        struct fuzzy_menu_option pivot = options[(
            size_t)((low + high) * 0.5f)];  // Selecting the middle
                                            // element as the pivot
        int i = low;
        int j = high;

        while (i <= j) {
            while (options[i].edit_distance > pivot.edit_distance)
                i++;  // Moving elements smaller than pivot to the
                      // left
            while (options[j].edit_distance < pivot.edit_distance)
                j--;  // Moving elements greater than pivot to the
                      // right

            if (i <= j) {
                struct fuzzy_menu_option temp =
                    options[i];  // Swapping elements
                options[i] = options[j];
                options[j] = temp;

                i++;
                j--;
            }
        }

        // Recursively sort the two partitions
        if (low < j) quick_sort_options(options, low, j);
        if (i < high) quick_sort_options(options, i, high);
    }
}

static bool fuzzy_menu_icons_enabled(struct fuzzy_menu* fm) {
    for (size_t i = 0; i < fm->options_count; i += 1)
        if (fm->options[i].icon != icon_none_t) return true;

    return false;
}

struct fuzzy_menu_dimensions fuzzy_menu_get_dimensions(
    struct fuzzy_menu* fm, struct ff_typography typo,
    Vector2 window_size) {
    struct fuzzy_menu_dimensions result = {0};

    const float total_padding = g_layout.padding * 2;
    const float total_spacing = g_layout.gap * 2;

    result.bounds_width =
        largest_string_width(fm->options_count, fm->options, typo);
    if (fuzzy_menu_icons_enabled(fm)) {
        result.bounds_width += 24;  // icon size + spacing
    }
    result.bounds_width = fmaxf(result.bounds_width, MENU_MIN_WIDTH);

    result.bg_width = result.bounds_width + g_layout.padding * 2;
    result.bg_x = window_size.x * .5f - result.bounds_width * .5f;
    result.bounds_x = result.bg_x + g_layout.padding;
    result.options_x = result.bounds_x;
    if (fuzzy_menu_icons_enabled(fm)) {
        result.options_x += 24;  // icon size + spacing
    }

    result.editor_height = typo.size + 4;
    result.editor_bg_height =
        result.editor_height + g_layout.padding * 2;

    result.options_height =
        (typo.size + g_layout.gap) * fm->options_count;
    result.options_height = fminf(
        result.options_height, window_size.y * MENU_MAX_HEIGHT_PERC);
    result.options_bg_height =
        result.options_height + g_layout.padding * 2;

    result.editor_bg_y =
        window_size.y * .5f -
        (result.editor_bg_height + result.options_bg_height) * .5f;
    result.editor_y = result.editor_bg_y +
                      (result.editor_bg_height * .5f) -
                      typo.size * .7f;

    result.options_bg_y = result.editor_bg_y +
                          result.editor_bg_height +
                          MENU_EDITOR_OPTIONS_SPACE;
    result.options_y = result.options_bg_y + g_layout.padding;

    return result;
}

void fuzzy_menu_draw_editor(struct fuzzy_menu* fm,
                            struct ff_typography typo,
                            int focus_flags,
                            struct fuzzy_menu_dimensions dimensions) {
    Rectangle editor_bg_rec = {.x = dimensions.bg_x,
                               .y = dimensions.editor_bg_y,
                               .width = dimensions.bg_width,
                               .height = dimensions.editor_bg_height};
    Rectangle editor_rec = {.x = dimensions.bounds_x,
                            .y = dimensions.editor_y,
                            .width = dimensions.bounds_width,
                            .height = dimensions.editor_height};

    DrawRectangleRec(editor_bg_rec,
                     hex_to_color(g_color_scheme.surface0_bg));

    BeginScissorMode(editor_rec.x, editor_rec.y, editor_rec.width,
                     editor_rec.height);
    editor_draw(&fm->editor, typo, editor_rec,
                focus_flags);  // NOTE: FIX HORZ SCROLL
    EndScissorMode();
}

void fuzzy_menu_draw_options(
    struct fuzzy_menu* fm, struct ff_typography typo,
    struct fuzzy_menu_dimensions dimensions) {
    Rectangle options_rec = {.x = dimensions.options_x,
                             .y = dimensions.options_y,
                             .width = dimensions.bounds_width,
                             .height = dimensions.options_height};
    Rectangle options_bg_rec = {
        .x = dimensions.bg_x,
        .y = dimensions.options_bg_y,
        .width = dimensions.bg_width,
        .height = dimensions.options_bg_height};

    DrawRectangleRec(options_bg_rec,
                     hex_to_color(g_color_scheme.surface0_bg));

    BeginScissorMode(dimensions.bounds_x, options_rec.y,
                     options_rec.width, options_rec.height);
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.offset.y = -fm->motion.position[0];
    BeginMode2D(cam);
    DrawRectangle(dimensions.bounds_x - g_layout.padding * .5f,
                  fm->motion.position[1] - g_layout.gap,
                  options_rec.width + g_layout.padding,
                  typo.size + g_layout.gap * 2,
                  hex_to_color(g_color_scheme.selected_bg));
    EndMode2D();

    ff_glyphs_vector_clear(&fm->glyphs);
    float selected_y = 0;
    float option_y = options_rec.y;
    for (size_t i = 0; i < fm->options_count; i++) {
        if (i == fm->selected) selected_y = option_y;

        struct ff_position draw_position = {options_rec.x, option_y};
        typo.color = i == fm->selected ? g_color_scheme.selected_fg
                                       : g_color_scheme.fg;
        ff_print_utf32(
            &fm->glyphs,
            (struct ff_utf32_str){
                .data = (char32_t*)fm->options[i].name,
                .size = fm->options[i].name_len},
            (struct ff_print_params){
                .typography = typo,
                .print_flags = ff_get_default_print_flags(),
                .characteristics = ff_get_default_characteristics()},
            draw_position);

        if (fm->options[i].icon != icon_none_t) {
            Texture icon = get_icon(fm->options[i].icon);
            Rectangle source = {.x = 0,
                                .y = 0,
                                .width = icon.width,
                                .height = icon.height};
            Rectangle dest = {.x = dimensions.bounds_x,
                              .y = draw_position.y,
                              icon.width,
                              icon.height};
            Vector2 origin = {.x = 0, .y = 0};

            BeginMode2D(cam);
            DrawTexturePro(icon, source, dest, origin, 0,
                           hex_to_color(g_color_scheme.fg));
            EndMode2D();
        }

        option_y += typo.size + g_layout.gap * 2;
    }

    fuzzy_menu_auto_scroll(fm, options_rec.y, typo.size,
                           selected_y + typo.size,
                           options_rec.height);
    motion_update(&fm->motion,
                  (float[2]){fm->vertical_scroll, selected_y},
                  GetFrameTime());
    float projection[4][4];
    ff_get_ortho_projection(
        (struct ff_ortho_params){
            0, GetScreenWidth(),
            GetScreenHeight() + fm->motion.position[0],
            fm->motion.position[0], -1, 1},
        projection);

    ff_draw(typo.font, fm->glyphs.data, fm->glyphs.size,
            (float*)projection);
    EndScissorMode();
}

void fuzzy_menu_sel_next(struct fuzzy_menu *fm) {
    if (fm->selected < fm->options_count-1) fm->selected +=1;    
}

void fuzzy_menu_sel_prev(struct fuzzy_menu *fm) {
    if (fm->selected > 0) fm->selected -=1;    
}

const char32_t* fuzzy_menu_handle_interactions(
    struct fuzzy_menu* fm) {
    if (is_key_sticky(KEY_DOWN)) {
        fuzzy_menu_sel_next(fm);
        return NULL;
    }
    if (is_key_sticky(KEY_UP)) fuzzy_menu_sel_prev(fm);
    if (IsKeyPressed(KEY_ENTER)) {
        editor_clear(&fm->editor);
        return fm->options[fm->selected].name;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (is_key_sticky(KEY_N)) {
            fuzzy_menu_sel_next(fm);
            return NULL;
        };
        if (is_key_sticky(KEY_P)) {
            fuzzy_menu_sel_prev(fm);
            return NULL;
        };
    }

    return NULL;
}

bool fuzzy_menu_buffer_changed(struct fuzzy_menu* fm) {
    return fm->editor.text.buffer.size != fm->previous_buffer_size;
}

void fuzzy_menu_on_buffer_change(struct fuzzy_menu* fm) {
    for (size_t i = 0; i < fm->options_count; i += 1) {
        fm->options[i].edit_distance = edit_distance(
            fm->editor.text.buffer.data, fm->editor.text.buffer.size,
            fm->options[i].name, fm->options[i].name_len);
    }
    quick_sort_options(fm->options, 0, fm->options_count - 1);
    fm->previous_buffer_size = fm->editor.text.buffer.size;
    fm->selected = 0;
}

const char32_t* fuzzy_menu_perform(struct fuzzy_menu* fm,
                                   struct ff_typography typo,
                                   int focus_flags) {
    const Vector2 window_size = {.x = GetScreenWidth(),
                                 .y = GetScreenHeight()};

    struct fuzzy_menu_dimensions dimensions =
        fuzzy_menu_get_dimensions(fm, typo, window_size);

    fuzzy_menu_draw_editor(fm, typo, focus_flags, dimensions);
    fuzzy_menu_draw_options(fm, typo, dimensions);

    if (fuzzy_menu_buffer_changed(fm))
        fuzzy_menu_on_buffer_change(fm);

    if (focus_flags & focus_flag_can_interact)
        return fuzzy_menu_handle_interactions(fm);

    return NULL;
}

void fuzzy_menu_reset(struct fuzzy_menu* fm) {
    memset(fm->options, 0,
           sizeof(struct fuzzy_menu_option) * fm->options_count);
    fm->options_count = 0;
    fm->selected = 0;
}
