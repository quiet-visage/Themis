#include "line_editor.h"

struct search_matches {
    struct selection* data;
    size_t length;
    size_t capactiy;
};

struct search_mod {
    struct line_editor search_editor;
    struct search_matches search_matches;
    size_t prev_search_buffer_size;
    size_t selected_match_idx;
};

void search_mod_create(struct search_mod* m);
void search_mod_destroy(struct search_mod* m);
void search_mod_select_next(struct search_mod* m);
void search_mod_select_prev(struct search_mod* m);
void search_mod_select_first(struct search_mod* m);
void search_mod_clear_matches(struct search_mod* m);
bool search_mod_input_changed(struct search_mod* m);
bool search_mod_is_empty(struct search_mod* m);
struct selection* search_mod_get_selected_match(struct search_mod* m);
void search_mod_find(struct search_mod* m, struct buffer* buffer);
void search_mod_draw(struct search_mod* m, struct ff_typography typo,
                     Rectangle outer_bounds, int focus_flags);
