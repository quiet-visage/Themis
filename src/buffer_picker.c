#include "buffer_picker.h"

#include <uchar.h>

#include "buffer_handler.h"
#include "config/config.h"
#include "dyn_strings/utf32_string.h"
#include <fieldfusion.h>
#include "menu/fuzzy_menu.h"
#include "raylib.h"
#include "text/text_view.h"

struct fuzzy_menu g_fuzzy_menu = {0};
struct text_view g_text_preview = {0};
size_t g_previous_selected = 0;

void buffer_picker_init(void) {
    fuzzy_menu_create(&g_fuzzy_menu);
    g_text_preview = text_view_create();
}

void buffer_picker_terminate(void) {
    fuzzy_menu_destroy(&g_fuzzy_menu);
    text_view_destroy(&g_text_preview);
}

struct buffer_picker_dimensions {
    struct fuzzy_menu_dimensions fz_menu_dimensions;

    float buffer_preview_bg_x;
    float buffer_preview_bounds_x;

    float buffer_preview_bg_y;
    float buffer_preview_bounds_y;

    float buffer_preview_bg_width;
    float buffer_preview_bounds_width;

    float buffer_preview_bg_height;
    float buffer_preview_bounds_height;
};

#define BUFFER_PICKER_PREVIEW_HEIGHT_PERC 0.9f
#define BUFFER_PICKER_PREVIEW_WIDTH_PERC 0.75f

struct buffer_picker_dimensions buffer_picker_get_dimensions(
    struct ff_typography typo) {
    const Vector2 window_size = {.x = GetScreenWidth(),
                                 .y = GetScreenHeight()};
    struct buffer_picker_dimensions result = {0};
    result.fz_menu_dimensions =
        fuzzy_menu_get_dimensions(&g_fuzzy_menu, typo, window_size);

    result.buffer_preview_bg_height =
        window_size.y * BUFFER_PICKER_PREVIEW_HEIGHT_PERC;
    result.buffer_preview_bounds_height =
        result.buffer_preview_bg_height - g_cfg.layout.padding * 2;

    result.buffer_preview_bg_y =
        window_size.y * 0.5f - result.buffer_preview_bg_height * 0.5f;
    result.buffer_preview_bounds_y =
        result.buffer_preview_bg_y + g_cfg.layout.padding;
    result.fz_menu_dimensions.editor_bg_y =
        result.buffer_preview_bg_y;
    result.fz_menu_dimensions.editor_y =
        result.buffer_preview_bounds_y;
    result.fz_menu_dimensions.editor_y =
        result.buffer_preview_bounds_y;
    result.fz_menu_dimensions.options_bg_y =
        result.fz_menu_dimensions.editor_bg_y +
        result.fz_menu_dimensions.editor_bg_height;
    result.fz_menu_dimensions.options_y =
        result.fz_menu_dimensions.options_bg_y + g_cfg.layout.padding;

    result.buffer_preview_bg_width =
        (window_size.x - result.fz_menu_dimensions.bg_width) *
        BUFFER_PICKER_PREVIEW_WIDTH_PERC;
    result.buffer_preview_bounds_width =
        result.buffer_preview_bg_width - g_cfg.layout.padding * 2;

    result.fz_menu_dimensions.editor_width =
        result.fz_menu_dimensions.bg_width - g_cfg.layout.padding * 2;
    result.fz_menu_dimensions.bg_x =
        window_size.x * 0.5f - (result.buffer_preview_bg_width +
                                result.fz_menu_dimensions.bg_width) *
                                   0.5f;
    result.buffer_preview_bg_x = result.fz_menu_dimensions.bg_x +
                                 result.fz_menu_dimensions.bg_width;
    result.buffer_preview_bounds_x =
        result.buffer_preview_bg_x + g_cfg.layout.padding;

    result.fz_menu_dimensions.options_bg_height =
        result.buffer_preview_bg_height -
        result.fz_menu_dimensions.editor_bg_height;

    result.fz_menu_dimensions.options_y =
        result.fz_menu_dimensions.options_bg_y + g_cfg.layout.padding;
    result.fz_menu_dimensions.bounds_x =
        result.fz_menu_dimensions.bg_x + g_cfg.layout.padding;
    result.fz_menu_dimensions.options_x =
        result.fz_menu_dimensions.bounds_x + 24;  // NOTE
    result.fz_menu_dimensions.options_height =
        result.fz_menu_dimensions.options_bg_height -
        g_cfg.layout.padding * 2;

    return result;
}

struct buffer* buffer_picker_get_selected_buffer() {
    size_t selected_name_len =
        g_fuzzy_menu.options[g_fuzzy_menu.selected].name_len;
    char selected_name[selected_name_len + 1];
    selected_name[selected_name_len] = 0;

    int conversion_failed = ff_utf32_to_utf8(
        selected_name,
        g_fuzzy_menu.options[g_fuzzy_menu.selected].name,
        selected_name_len);
    assert(!conversion_failed);
    return buffer_handler_get(selected_name);
}

void buffer_picker_update(void) {
    fuzzy_menu_reset(&g_fuzzy_menu);

    size_t buffer_count = buffer_handler_count();
    if (!buffer_count) return;

    struct utf32_str buffer_names[buffer_count];
    for (size_t i = 0; i < buffer_count; i += 1) {
        buffer_names[i] = utf32_str_create();
    }

    buffer_handler_list_names(buffer_count, buffer_names);

    for (size_t i = 0; i < buffer_count; i += 1) {
        fuzzy_menu_push_option(&g_fuzzy_menu, buffer_names[i].data,
                               buffer_names[i].size);
        utf32_str_destroy(&buffer_names[i]);
    }

    struct buffer* selected_buffer =
        buffer_picker_get_selected_buffer();
    assert(selected_buffer);

    g_text_preview.buffer = selected_buffer;
}

struct buffer* buffer_picker_perform(struct ff_typography typo,
                                     int focus_flags) {
    struct buffer_picker_dimensions dimensions =
        buffer_picker_get_dimensions(typo);
    fuzzy_menu_draw_editor(&g_fuzzy_menu, typo, focus_flags,
                           dimensions.fz_menu_dimensions);
    fuzzy_menu_draw_options(&g_fuzzy_menu, typo,
                            dimensions.fz_menu_dimensions);

    struct buffer* selected_buffer =
        buffer_picker_get_selected_buffer();
    assert(selected_buffer);

    if (g_previous_selected != g_fuzzy_menu.selected) {
        g_text_preview.buffer = selected_buffer;
    }

    Rectangle preview_bounds = (Rectangle){
        .x = dimensions.buffer_preview_bg_x,
        .y = dimensions.buffer_preview_bg_y,
        .width = dimensions.buffer_preview_bg_width,
        .height = dimensions.buffer_preview_bg_height,
    };

    DrawRectangleRec(preview_bounds,
                     GetColor(g_cfg.color_scheme.surface1_bg));
    text_view_draw(&g_text_preview, typo, preview_bounds,
                   focus_flags);

    if (fuzzy_menu_handle_interactions(&g_fuzzy_menu)) {
        return selected_buffer;
    }

    return 0;
}
