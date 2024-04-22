#include "fuzzy_menu.h"

#include <fieldfusion.h>
#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "commands.h"
#include "config.h"
#include "fuzzy_menu.h"
#include "keyboard.h"
#include "motion.h"

#define MENU_MIN_WIDTH 124.0f
#define MENU_PADDING 18.0f
#define MENU_MAX_HEIGHT_PERC 0.55f
#define MENU_EDITOR_OPTIONS_SPACE 6.0f

static uint similiarity_score(const c32_t* s1, size_t len1,
                              const c32_t* s2, size_t len2) {
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
}

void fuzzy_menu_create(fuzzy_menu_t* m) {
    line_editor_create(&m->editor);
    m->editor.text.buffer = malloc(sizeof(buffer_t));
    buffer_create(m->editor.text.buffer, utf32_str_create());
    buffer_save_undo(m->editor.text.buffer, (text_pos_t){0});
    m->vertical_scroll = 0;
    m->glyphs = ff_glyph_vec_create();
    m->previous_buffer_size = 0;
    m->selected = 0;
    memset(&m->options, 0,
           sizeof(fuzzy_menu_option_t) * FUZZY_MENU_OPTIONS_CAP);
    m->options_count = 0;
    m->motion = motion_new();
    m->motion.f = 1.9f;
    m->motion.z = 0.9f;
}

void fuzzy_menu_destroy(fuzzy_menu_t* o) {
    buffer_destroy(o->editor.text.buffer);
    free(o->editor.text.buffer);
    line_editor_destroy(&o->editor);
    ff_glyph_vec_destroy(&o->glyphs);
}

static void fuzzy_menu_auto_scroll(fuzzy_menu_t* fm, float options_y,
                                   float size, float selected_y,
                                   float options_height) {
    float cursor_pixel_pos = selected_y;
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
                                  ff_typo_t typo) {
    float result = 0;
    for (size_t i = 0; i < count; i += 1) {
        float str_width =
            ff_measure_utf32(options[i].name, options[i].name_len,
                             typo.font, typo.size, true)
                .width;
        result = fmaxf(result, str_width);
    }
    return result;
}

void fuzzy_menu_push_option(fuzzy_menu_t* fm, const c32_t* option,
                            size_t len) {
    assert(len < FUZZY_MENU_OPTION_NAME_CAP);
    fm->options[fm->options_count].name_len = len;
    fm->options[fm->options_count].icon = icon_none_t;
    memcpy(fm->options[fm->options_count++].name, option,
           len * sizeof(c32_t));
    assert(fm->options_count < FUZZY_MENU_OPTIONS_CAP);
}

void fuzzy_menu_push_option_with_icon(fuzzy_menu_t* fm,
                                      const c32_t* option, size_t len,
                                      enum icon icon) {
    assert(len < FUZZY_MENU_OPTION_NAME_CAP);
    fm->options[fm->options_count].name_len = len;
    fm->options[fm->options_count].icon = icon;
    memcpy(fm->options[fm->options_count++].name, option,
           len * sizeof(c32_t));
    assert(fm->options_count < FUZZY_MENU_OPTIONS_CAP);
}

static void quick_sort_options(fuzzy_menu_option_t options[], int low,
                               int high) {
    if (low < high) {
        fuzzy_menu_option_t pivot =
            options[(size_t)((low + high) * 0.5f)];

        int i = low;
        int j = high;

        while (i <= j) {
            while (options[i].edit_distance > pivot.edit_distance)
                i++;
            while (options[j].edit_distance < pivot.edit_distance)
                j--;

            if (i <= j) {
                fuzzy_menu_option_t temp = options[i];
                options[i] = options[j];
                options[j] = temp;

                i++;
                j--;
            }
        }

        if (low < j) quick_sort_options(options, low, j);
        if (i < high) quick_sort_options(options, i, high);
    }
}

static bool fuzzy_menu_icons_enabled(fuzzy_menu_t* fm) {
    for (size_t i = 0; i < fm->options_count; i += 1)
        if (fm->options[i].icon != icon_none_t) return true;

    return false;
}

fuzzy_menu_dimensions_t fuzzy_menu_get_dimensions(
    fuzzy_menu_t* fm, ff_typo_t typo, Vector2 window_size) {
    fuzzy_menu_dimensions_t result = {0};

    result.bounds_width =
        largest_string_width(fm->options_count, fm->options, typo);
    if (fuzzy_menu_icons_enabled(fm)) {
        result.bounds_width += 24;  // icon size + spacing
    }
    result.bounds_width = fmaxf(result.bounds_width, MENU_MIN_WIDTH);
    result.editor_width = result.bounds_width;

    result.bg_width = result.bounds_width + g_cfg.layout.padding * 2;
    result.bg_x = window_size.x * .5f - result.bounds_width * .5f;
    result.bounds_x = result.bg_x + g_cfg.layout.padding;
    result.options_x = result.bounds_x;
    if (fuzzy_menu_icons_enabled(fm)) {
        result.options_x += 24;  // icon size + spacing
    }

    result.editor_height = typo.size + 4;
    result.editor_bg_height =
        result.editor_height + g_cfg.layout.padding * 2;

    result.options_height =
        (typo.size + g_cfg.layout.gap) * fm->options_count;
    result.options_height = fminf(
        result.options_height, window_size.y * MENU_MAX_HEIGHT_PERC);
    result.options_bg_height =
        result.options_height + g_cfg.layout.padding * 2;

    result.editor_bg_y =
        window_size.y * .5f -
        (result.editor_bg_height + result.options_bg_height) * .5f;
    result.editor_y = result.editor_bg_y +
                      (result.editor_bg_height * .5f) -
                      typo.size * .7f;

    result.options_bg_y = result.editor_bg_y +
                          result.editor_bg_height +
                          MENU_EDITOR_OPTIONS_SPACE;
    result.options_y = result.options_bg_y + g_cfg.layout.padding;

    return result;
}

void fuzzy_menu_draw_editor(fuzzy_menu_t* fm, ff_typo_t typo,
                            int focus_flags,
                            fuzzy_menu_dimensions_t dimensions) {
    Rectangle editor_bg_rec = {.x = dimensions.bg_x,
                               .y = dimensions.editor_bg_y,
                               .width = dimensions.bg_width,
                               .height = dimensions.editor_bg_height};
    Rectangle editor_rec = {.x = dimensions.bounds_x,
                            .y = dimensions.editor_y,
                            .width = dimensions.bounds_width,
                            .height = dimensions.editor_height};

    DrawRectangleRec(editor_bg_rec,
                     GetColor(g_cfg.color_scheme.surface0_bg));

    BeginScissorMode(editor_rec.x, editor_rec.y, editor_rec.width,
                     editor_rec.height);
    line_editor_draw(&fm->editor, typo, editor_rec, focus_flags);
    EndScissorMode();
}

void fuzzy_menu_draw_options(fuzzy_menu_t* fm, ff_typo_t typo,
                             fuzzy_menu_dimensions_t dimensions) {
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
                     GetColor(g_cfg.color_scheme.surface0_bg));

    BeginScissorMode(dimensions.bounds_x, options_rec.y,
                     options_rec.width, options_rec.height);
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.offset.y = -fm->motion.position[0];
    BeginMode2D(cam);
    DrawRectangle(dimensions.bounds_x - g_cfg.layout.padding * .5f,
                  fm->motion.position[1] - g_cfg.layout.gap,
                  options_rec.width + g_cfg.layout.padding,
                  typo.size + g_cfg.layout.gap * 2,
                  GetColor(g_cfg.color_scheme.selected_bg));
    EndMode2D();

    ff_glyph_vec_clear(&fm->glyphs);
    float selected_y = 0;
    float option_y = options_rec.y;
    for (size_t i = 0; i < fm->options_count; i++) {
        if (i == fm->selected) selected_y = option_y;

        float draw_pos_x = options_rec.x;
        float draw_pos_y = option_y;
        typo.color = i == fm->selected
                         ? g_cfg.color_scheme.selected_fg
                         : g_cfg.color_scheme.fg;

        ff_print_utf32_vec(&fm->glyphs, fm->options[i].name,
                           fm->options[i].name_len, typo, draw_pos_x,
                           draw_pos_y, ff_flag_default, 0);

        if (fm->options[i].icon != icon_none_t) {
            Texture icon = get_icon(fm->options[i].icon);
            Rectangle source = {.x = 0,
                                .y = 0,
                                .width = icon.width,
                                .height = icon.height};
            Rectangle dest = {.x = dimensions.bounds_x,
                              .y = draw_pos_y,
                              icon.width,
                              icon.height};
            Vector2 origin = {.x = 0, .y = 0};

            BeginMode2D(cam);
            DrawTexturePro(icon, source, dest, origin, 0,
                           GetColor(g_cfg.color_scheme.fg));
            EndMode2D();
        }

        option_y += typo.size + g_cfg.layout.gap * 2;
    }

    fuzzy_menu_auto_scroll(fm, options_rec.y, typo.size,
                           selected_y + typo.size,
                           options_rec.height);
    motion_update(&fm->motion,
                  (float[2]){fm->vertical_scroll, selected_y},
                  GetFrameTime());
    float projection[4][4];
    ff_get_ortho_projection(
        0, GetScreenWidth(),
        GetScreenHeight() + fm->motion.position[0],
        fm->motion.position[0], -1.0f, 1.0f, projection);

    ff_draw(typo.font, fm->glyphs.data, fm->glyphs.len,
            (float*)projection);
    EndScissorMode();
}

void fuzzy_menu_sel_next(fuzzy_menu_t* fm) {
    if (fm->selected < fm->options_count - 1) fm->selected += 1;
}

void fuzzy_menu_sel_prev(fuzzy_menu_t* fm) {
    if (fm->selected > 0) fm->selected -= 1;
}

const c32_t* fuzzy_menu_handle_user_input(fuzzy_menu_t* fm) {
    int cmd = key_seq_handler_get_command(g_cfg.keybinds.fuzzy_menu);
    switch (cmd) {
        case menu_cmd_move_up: fuzzy_menu_sel_prev(fm); break;
        case menu_cmd_move_down: fuzzy_menu_sel_next(fm);
    }

    if (is_key_sticky(KEY_ENTER)) {
        line_editor_clear(&fm->editor);
        return fm->options[fm->selected].name;
    }

    return NULL;
}

bool fuzzy_menu_buffer_changed(fuzzy_menu_t* fm) {
    return fm->editor.text.buffer->str.length !=
           fm->previous_buffer_size;
}

void fuzzy_menu_on_buffer_change(fuzzy_menu_t* fm) {
    for (size_t i = 0; i < fm->options_count; i += 1) {
        fm->options[i].edit_distance = similiarity_score(
            fm->editor.text.buffer->str.data,
            fm->editor.text.buffer->str.length, fm->options[i].name,
            fm->options[i].name_len);
    }
    quick_sort_options(fm->options, 0, fm->options_count - 1);
    fm->previous_buffer_size = fm->editor.text.buffer->str.length;
    fm->selected = 0;
}

void fuzzy_menu_reset(fuzzy_menu_t* fm) {
    memset(fm->options, 0,
           sizeof(fuzzy_menu_option_t) * fm->options_count);
    fm->options_count = 0;
    fm->selected = 0;
}
