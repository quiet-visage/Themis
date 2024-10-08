#include "search_mod.h"

#include <string.h>

#include "../config.h"
#include "../resources/resources.h"
#include "fieldfusion.h"
#include "line_editor.h"

static void search_matches_create(search_matches_t* m) {
    memset(m, 0, sizeof(search_matches_t));
    m->capactiy = sizeof(search_matches_t) * 2;
    m->data = malloc(m->capactiy);
    assert(m->data);
}

static void search_matches_push(search_matches_t* m,
                                selection_t item) {
    size_t required_capacity = (m->length + 1) * sizeof(selection_t);

    while (required_capacity > m->capactiy) {
        m->capactiy *= 2;
        m->data = realloc(m->data, m->capactiy);
        assert(m->data);
    }

    m->data[m->length] = item;
    m->length += 1;
}

static void search_matches_destroy(search_matches_t* m) {
    free(m->data);
}

static void search_matches_clear(search_matches_t* m) {
    m->length = 0;
}

void search_mod_find(search_mod_t* m, buffer_t* buffer) {
    search_matches_clear(&m->search_matches);

    bool search_buffer_empty =
        !m->search_editor.text.buffer->str.length;
    bool buf_empty = !buffer->str.length;
    if (search_buffer_empty || buf_empty) return;

    c32_t* sub_str_ptr = 0;
    size_t search_pos = 0;
    while ((sub_str_ptr =
                str32str32(m->search_editor.text.buffer->str.data,
                           m->search_editor.text.buffer->str.length,
                           &buffer->str.data[search_pos],
                           buffer->str.length - search_pos))) {
        size_t match_pos = sub_str_ptr - &buffer->str.data[0];
        size_t match_line_num = buffer_lines_get_line_num_from_idx(
            &buffer->lines, match_pos);
        size_t match_column =
            match_pos - buffer->lines.data[match_line_num].start;

        search_matches_push(
            &m->search_matches,
            (selection_t){
                .from_line = match_line_num,
                .from_col = match_column,
                .to_line = match_line_num,
                .to_col = match_column +
                          m->search_editor.text.buffer->str.length});

        size_t next_search_pos =
            match_pos + m->search_editor.text.buffer->str.length;
        if (next_search_pos > buffer->str.length) return;
        search_pos = next_search_pos;
    }

    m->prev_search_buffer_size =
        m->search_editor.text.buffer->str.length;
}

void search_mod_create(search_mod_t* m) {
    memset(m, 0, sizeof(search_mod_t));
    search_matches_create(&m->search_matches);
    line_editor_create(&m->search_editor);
    m->search_editor.text.buffer = malloc(sizeof(buffer_t));
    buffer_create(m->search_editor.text.buffer, utf32_str_create());
}

void search_mod_destroy(search_mod_t* m) {
    search_matches_destroy(&m->search_matches);
    line_editor_destroy(&m->search_editor);
    buffer_destroy(m->search_editor.text.buffer);
    free(m->search_editor.text.buffer);
}

void search_mod_select_next(search_mod_t* m) {
    if (!m->search_matches.length) return;

    m->selected_match_idx += 1;
    if (m->selected_match_idx > m->search_matches.length - 1)
        m->selected_match_idx = 0;
}

void search_mod_select_prev(search_mod_t* m) {
    if (!m->search_matches.length) return;

    if (!m->selected_match_idx) {
        m->selected_match_idx = m->search_matches.length - 1;
    } else {
        m->selected_match_idx -= 1;
    }
}

void search_mod_select_first(search_mod_t* m) {
    m->selected_match_idx = 0;
}

void search_mod_clear_matches(search_mod_t* m) {
    search_matches_clear(&m->search_matches);
    m->selected_match_idx = 0;
}

bool search_mod_input_changed(search_mod_t* m) {
    return m->prev_search_buffer_size !=
           m->search_editor.text.buffer->str.length;
}

bool search_mod_is_empty(search_mod_t* m) {
    return !m->search_matches.length;
}

selection_t* search_mod_get_selected_match(search_mod_t* m) {
    if (m->selected_match_idx >= m->search_matches.length) return 0;
    return &m->search_matches.data[m->selected_match_idx];
}

void search_mod_draw(search_mod_t* m, ff_typo_t typo,
                     Rectangle outer_bounds, int focus_flags) {
    Texture search_icon = get_icon(icon_search_t);

    Rectangle bg;
    bg.width = outer_bounds.width < g_cfg.layout.search_box_width
                   ? outer_bounds.width
                   : g_cfg.layout.search_box_width;
    bg.x =
        outer_bounds.x + outer_bounds.width * 0.5f - bg.width * 0.5f;
    bg.y = outer_bounds.height * 0.5f - typo.size * 0.5f;
    bg.height = typo.size + g_cfg.layout.padding * 2;

    DrawRectangleRec(bg, GetColor(g_cfg.color_scheme.surface0_bg));

    Rectangle icon_source = {.x = 0,
                             .y = 0,
                             .width = search_icon.width,
                             .height = search_icon.height};

    bg.x += g_cfg.layout.padding;
    bg.y += g_cfg.layout.padding;
    Rectangle icon_dest = {.x = bg.x,
                           .y = bg.y,
                           .width = search_icon.width,
                           .height = search_icon.height};
    DrawTexturePro(search_icon, icon_source, icon_dest, (Vector2){0},
                   0, WHITE);

    bg.x += search_icon.width + g_cfg.layout.padding;
    bg.width -= search_icon.width + g_cfg.layout.padding * 3;
    line_editor_draw(&m->search_editor, typo, bg, focus_flags);
}
