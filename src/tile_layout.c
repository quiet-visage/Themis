#include "tile_layout.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

struct layout_action {
    enum split_kind split_kind;
    size_t split_index;
};

#define MAX_ACTIONS 512
#define MAX_RECTS 512
#define MIN_SIZE 256

static Rectangle g_root_rectangle = {0};
static Rectangle g_rects[MAX_RECTS] = {0};
static Rectangle g_sorted_mirror[MAX_RECTS] = {0};
static size_t g_sorted_mirror_count = 0;
static size_t g_rects_count = 0;
static struct layout_action g_layout_actions[MAX_ACTIONS] = {0};
static size_t g_layout_actions_count = 0;

Rectangle tile_get_rect(size_t n) {
    assert(n < g_rects_count);
    return g_rects[n];
}

void tile_set_root(Rectangle rect) { g_root_rectangle = rect; }

int tile_rect_sort_cmp(const void* x, const void* y) {
    Rectangle* a = (Rectangle*)x;
    Rectangle* b = (Rectangle*)y;
    return (a->x - b->x) ? a->x - b->x : a->y - b->y;
}

static void tile_perform_internal(struct layout_action* actions,
                                  size_t actions_count,
                                  Rectangle* recs, size_t* recs_count,
                                  bool sort) {
    (*recs_count) = 1;
    recs[0] = g_root_rectangle;

    for (size_t i = 0; i < actions_count; i += 1) {
        struct layout_action action = actions[i];
        assert(action.split_index < *recs_count);

        if (action.split_kind == split_horizontal) {
            recs[action.split_index].width *= 0.5f;
            recs[(*recs_count)] = recs[action.split_index];
            recs[(*recs_count)++].x += recs[action.split_index].width;
        } else {
            recs[action.split_index].height *= 0.5f;
            recs[(*recs_count)] = recs[action.split_index];
            recs[(*recs_count)++].y +=
                recs[action.split_index].height;
        }
    }
    if (sort) {
        qsort(recs, *recs_count, sizeof(Rectangle),
              tile_rect_sort_cmp);
    }
}

static void tile_update_mirror(void) {
    tile_perform_internal(g_layout_actions, g_layout_actions_count,
                          g_sorted_mirror, &g_sorted_mirror_count,
                          true);
    assert(g_sorted_mirror_count == g_rects_count);
}

size_t tile_get_sorted_n(size_t n) {
    assert(n < g_rects_count);

    Rectangle a = g_rects[n];
    for (size_t i = 0; i < g_rects_count; i += 1) {
        Rectangle b = g_sorted_mirror[i];
        if (!memcmp(&a, &b, sizeof(Rectangle))) return i;
    }

    assert(0 && "What ever happened should not have happened");
}

size_t tile_get_non_sorted_n(size_t n) {
    assert(n < g_rects_count);

    Rectangle a = g_sorted_mirror[n];
    for (size_t i = 0; i < g_rects_count; i += 1) {
        Rectangle b = g_rects[i];
        if (!memcmp(&a, &b, sizeof(Rectangle))) return i;
    }

    assert(0 && "What ever happened should not have happened");
}

void tile_perform(void) {
    tile_perform_internal(g_layout_actions, g_layout_actions_count,
                          g_rects, &g_rects_count, false);
    tile_update_mirror();
}

size_t tile_get_count(void) { return g_rects_count; }

bool tile_split(enum split_kind split_kind, size_t n) {
    assert(n < g_rects_count);
    assert(g_layout_actions_count + 1 < MAX_ACTIONS);

    Rectangle sim_rec = g_rects[n];
    if (sim_rec.width < MIN_SIZE || sim_rec.height < MIN_SIZE) return false;

    g_layout_actions[g_layout_actions_count++] =
        (struct layout_action){.split_kind = split_kind,
                               .split_index = n};
    tile_perform();
    return true;
}

void tile_remove(size_t n) {
    assert(n < g_rects_count);
    g_layout_actions_count -= 1;
    g_rects_count -= 1;
    tile_perform();
}
