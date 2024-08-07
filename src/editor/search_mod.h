#include "line_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    selection_t* data;
    size_t length;
    size_t capactiy;
} search_matches_t;

typedef struct {
    line_editor_t search_editor;
    search_matches_t search_matches;
    size_t prev_search_buffer_size;
    size_t selected_match_idx;
} search_mod_t;

void search_mod_create(search_mod_t* m);
void search_mod_destroy(search_mod_t* m);
void search_mod_select_next(search_mod_t* m);
void search_mod_select_prev(search_mod_t* m);
void search_mod_select_first(search_mod_t* m);
void search_mod_clear_matches(search_mod_t* m);
bool search_mod_input_changed(search_mod_t* m);
bool search_mod_is_empty(search_mod_t* m);
selection_t* search_mod_get_selected_match(search_mod_t* m);
void search_mod_find(search_mod_t* m, buffer_t* buffer);
void search_mod_draw(search_mod_t* m, ff_typo_t typo,
                     Rectangle outer_bounds, int focus_flags);

#ifdef __cplusplus
}
#endif
