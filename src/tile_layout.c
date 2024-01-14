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
    return a->x - b->x;
}

void tile_perform(void) {
    g_rects_count = 1;
    g_rects[0] = g_root_rectangle;

    for (size_t i = 0; i < g_layout_actions_count; i += 1) {
        struct layout_action action = g_layout_actions[i];
        assert(action.split_index < g_rects_count);

        if (action.split_kind == split_horizontal) {
            g_rects[action.split_index].width *= 0.5f;
            g_rects[g_rects_count] = g_rects[action.split_index];
            g_rects[g_rects_count++].x +=
                g_rects[action.split_index].width;
        } else {
            g_rects[action.split_index].height *= 0.5f;
            g_rects[g_rects_count] = g_rects[action.split_index];
            g_rects[g_rects_count++].y +=
                g_rects[action.split_index].height;
        }
        qsort(g_rects, g_rects_count, sizeof(Rectangle),
              tile_rect_sort_cmp);
    }
}

size_t tile_get_count(void) { return g_rects_count; }

void tile_split(enum split_kind split_kind, size_t n) {
    assert(n < g_rects_count);
    assert(g_layout_actions_count + 1 < MAX_ACTIONS);

    Rectangle sim_rec = g_rects[n];
    if (sim_rec.width < MIN_SIZE || sim_rec.height < MIN_SIZE) return;

    g_layout_actions[g_layout_actions_count++] =
        (struct layout_action){.split_kind = split_kind,
                               .split_index = n};
    tile_perform();
}

void tile_remove(size_t n) {
    assert(n < g_rects_count);
    g_layout_actions_count -= 1;
    g_rects_count -= 1;
    tile_perform();
}
