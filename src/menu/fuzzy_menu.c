#include <menu/fuzzy_menu.h>

#include <field_fusion/fieldfusion.h>
#include <math.h>
#include <rlgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <uchar.h>

#include <text/editor.h>
#include <motion/motion.h>
#include <raylib.h>

#define MENU_MIN_WIDTH 124.0f
#define MENU_PADDING 18.0f
#define MENU_SPACING 6.0f
#define MENU_MAX_HEIGHT_PERC 0.55f
#define MENU_EDITOR_OPTIONS_SPACE 6.0f

static fuzzy_menu_colors_t g_colors = {
    .menu_bg = 0x313244ff,
    .editor_bg = 0x585b70ff,
    .editor_fg = 0xcdd6f4ff,
    .option_fg = 0xcdd6f4ff,
    .option_sel_fg = 0xcba6f7ff,
    .option_sel_bg = 0x6c7086ff,
};

static Color hex_to_color(int hex) {
    return (Color){.r = (hex >> 24) & 0xffl,
                   .g = (hex >> 16) & 0x00ffl,
                   .b = (hex >> 8) & 0x0000ffl,
                   .a = hex & 0x000000ffl};
}

static uint min(uint a, uint b) { return a < b ? a : b; }

static uint edit_distance(const char32_t* s1, size_t len1,
                          const char32_t* s2, size_t len2) {
    uint d[len1 + 1][len2 + 1];

    d[0][0] = 0;
    for (unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
    for (unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

    for (unsigned int i = 1; i <= len1; ++i)
        for (unsigned int j = 1; j <= len2; ++j) {
            d[i][j] = min(
                min(d[i - 1][j] + 1, d[i][j - 1] + 1),
                d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
        }
    return d[len1][len2];
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
    fuzzy_menu_option_t* a = (fuzzy_menu_option_t*)element_1;
    fuzzy_menu_option_t* b = (fuzzy_menu_option_t*)element_2;

    if (a->edit_distance < b->edit_distance) return -1;
    if (a->edit_distance > b->edit_distance) return 1;
    return 0;
}

fuzzy_menu_t fuzzy_menu_create() {
    fuzzy_menu_t result = {.editor = editor_create(),
                           .vertical_scroll = 0,
                           .glyphs = ff_glyphs_vector_create(),
                           .previous_buffer_size = 0,
                           .selected = 0,
                           .options = {0},
                           .options_count = 0,
                           .motion = motion_new()};
    result.editor.editor_flags = editor_flag_ignore_enter_t;
    result.motion.f = 1.9f;
    result.motion.z = 0.9f;
    return result;
}

void fuzzy_menu_destroy(fuzzy_menu_t* fm) {
    editor_destroy(&fm->editor);
    ff_glyphs_vector_destroy(&fm->glyphs);
}

static void fuzzy_menu_auto_scroll(fuzzy_menu_t* fm, float options_y,
                                   float size, float selected_y,
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
                                  fuzzy_menu_option_t* options,
                                  ff_typography_t typo) {
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

void fuzzy_menu_push_option(fuzzy_menu_t* fm, const char32_t* option,
                            size_t len) {
    assert(len < FUZZY_MENU_OPTION_NAME_CAP);
    fm->options[fm->options_count].name_len = len;
    memcpy(fm->options[fm->options_count++].name, option,
           len * sizeof(char32_t));
    assert(fm->options_count < FUZZY_MENU_OPTIONS_CAP);
}

static void qs_options(fuzzy_menu_option_t options[], int low,
                       int high) {
    if (low < high) {
        fuzzy_menu_option_t pivot = options[(
            size_t)((low + high) * 0.5f)];  // Selecting the middle
                                            // element as the pivot
        int i = low;
        int j = high;

        while (i <= j) {
            while (options[i].edit_distance < pivot.edit_distance)
                i++;  // Moving elements smaller than pivot to the
                      // left
            while (options[j].edit_distance > pivot.edit_distance)
                j--;  // Moving elements greater than pivot to the
                      // right

            if (i <= j) {
                fuzzy_menu_option_t temp =
                    options[i];  // Swapping elements
                options[i] = options[j];
                options[j] = temp;

                i++;
                j--;
            }
        }

        // Recursively sort the two partitions
        if (low < j) qs_options(options, low, j);
        if (i < high) qs_options(options, i, high);
    }
}

typedef struct {
    float bg_x;
    float bg_width;

    float bounds_x;
    float bounds_width;

    float editor_y;
    float editor_bg_y;

    float options_y;
    float options_bg_y;

    float editor_height;
    float editor_bg_height;

    float options_height;
    float options_bg_height;

} fuzzy_menu_dimensions_t;

fuzzy_menu_dimensions_t fuzzy_menu_get_dimensions(
    fuzzy_menu_t* fm, ff_typography_t typo, Vector2 window_size) {
    fuzzy_menu_dimensions_t result = {0};

    const float total_padding = MENU_PADDING * 2;
    const float total_spacing = MENU_SPACING * 2;

    result.bounds_width =
        largest_string_width(fm->options_count, fm->options, typo);
    result.bounds_width = fmaxf(result.bounds_width, MENU_MIN_WIDTH);

    result.bg_width = result.bounds_width + MENU_PADDING * 2;
    result.bg_x = window_size.x * .5f - result.bounds_width * .5f;
    result.bounds_x = result.bg_x + MENU_PADDING;

    result.editor_height =typo.size + 4;
    result.editor_bg_height = result.editor_height + MENU_PADDING ;

    result.options_height =
        (typo.size + MENU_SPACING) * fm->options_count;
    result.options_height = fminf(
        result.options_height, window_size.y * MENU_MAX_HEIGHT_PERC);
    result.options_bg_height =
        result.options_height + MENU_PADDING * 2;

    result.editor_bg_y =
        window_size.y * .5f -
        (result.editor_bg_height + result.options_bg_height) * .5f;
    result.editor_y = result.editor_bg_y +
                      (result.editor_bg_height * .5f) -
                      typo.size * .7f;

    result.options_bg_y =
        result.editor_bg_y + result.editor_bg_height + MENU_EDITOR_OPTIONS_SPACE;
    result.options_y = result.options_bg_y + MENU_PADDING;

    return result;
}

const char32_t* fuzzy_menu_perform(fuzzy_menu_t* fm,
                                   ff_typography_t typo,
                                   bool focused) {
    const Vector2 window_size = {.x = GetScreenWidth(),
                                 .y = GetScreenHeight()};
    fuzzy_menu_dimensions_t dimensions =
        fuzzy_menu_get_dimensions(fm, typo, window_size);

    Rectangle editor_rec = {.x = dimensions.bounds_x,
                            .y = dimensions.editor_y,
                            .width = dimensions.bounds_width,
                            .height = dimensions.editor_height};

    Rectangle editor_bg_rec = {.x = dimensions.bg_x,
                               .y = dimensions.editor_bg_y,
                               .width = dimensions.bg_width,
                               .height = dimensions.editor_bg_height};

    Rectangle options_rec = {.x = dimensions.bounds_x,
                             .y = dimensions.options_y,
                             .width = dimensions.bounds_width,
                             .height = dimensions.options_height};

    Rectangle options_bg_rec = {
        .x = dimensions.bg_x,
        .y = dimensions.options_bg_y,
        .width = dimensions.bg_width,
        .height = dimensions.options_bg_height};

    DrawRectangleRec(editor_bg_rec, hex_to_color(g_colors.editor_bg));
    DrawRectangleRec(options_bg_rec, hex_to_color(g_colors.menu_bg));
    rlDrawRenderBatchActive();

    BeginScissorMode(editor_rec.x, editor_rec.y, editor_rec.width,
                     editor_rec.height);
    editor_draw(&fm->editor, typo, editor_rec,
                focused);  // NOTE: FIX HORZ SCROLL
    EndScissorMode();

    bool buffer_changed =
        fm->editor.text.buffer.size != fm->previous_buffer_size;
    if (buffer_changed) {
        for (size_t i = 0; i < fm->options_count; i += 1) {
            fm->options[i].edit_distance = edit_distance(
                fm->editor.text.buffer.data,
                fm->editor.text.buffer.size, fm->options[i].name,
                fm->options[i].name_len);
        }
        qs_options(fm->options, 0, fm->options_count - 1);
        fm->previous_buffer_size = fm->editor.text.buffer.size;
    }

    ff_glyphs_vector_clear(&fm->glyphs);
    float selected_y = 0;
    float option_y = options_rec.y;
    for (size_t i = 0; i < fm->options_count; i++) {
        if (i == fm->selected) selected_y = option_y;

        const ff_position_t draw_position = {options_rec.x, option_y};
        typo.color = i == fm->selected ? g_colors.option_sel_fg
                                       : g_colors.option_fg;
        ff_print_utf32(
            &fm->glyphs,
            (ff_utf32_str_t){.data = (char32_t*)fm->options[i].name,
                             .size = fm->options[i].name_len},
            (ff_print_params_t){
                .typography = typo,
                .print_flags = ff_get_default_print_flags(),
                .characteristics = ff_get_default_characteristics()},
            draw_position);

        option_y += typo.size + MENU_SPACING * 2;
    }
    fuzzy_menu_auto_scroll(fm, options_rec.y, typo.size,
                           selected_y + typo.size,
                           options_rec.height);
    motion_update(&fm->motion,
                  (float[2]){fm->vertical_scroll, selected_y},
                  GetFrameTime());
    float projection[4][4];
    ff_get_ortho_projection(
        (ortho_projection_params_t){
            0, GetScreenWidth(),
            GetScreenHeight() + fm->motion.position[0],
            fm->motion.position[0], -1, 1},
        projection);

    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.offset.y = -fm->motion.position[0];

    BeginMode2D(cam);
    DrawRectangle(options_rec.x - MENU_PADDING *.5f,
                  fm->motion.position[1] - MENU_SPACING,
                  options_rec.width + MENU_PADDING,
                  typo.size + MENU_SPACING * 2,
                  hex_to_color(g_colors.option_sel_bg));
    EndMode2D();

    BeginScissorMode(options_rec.x, options_rec.y, options_rec.width, options_rec.height);
    ff_draw(typo.font, fm->glyphs.data, fm->glyphs.size,
            (float*)projection);
    EndScissorMode();

    if (focused) {
        if (is_key_sticky(KEY_DOWN) &&
            (fm->selected + 1) < fm->options_count)
            fm->selected += 1;
        if (is_key_sticky(KEY_UP) && fm->selected > 0)
            fm->selected -= 1;
        if (IsKeyReleased(KEY_ENTER))
            return fm->options[fm->selected].name;
    }

    return NULL;
}

void fuzzy_menu_reset(fuzzy_menu_t* fm) {
    memset(fm->options, 0,
           sizeof(fuzzy_menu_option_t) * fm->options_count);
    fm->options_count = 0;
    fm->selected = 0;
}
