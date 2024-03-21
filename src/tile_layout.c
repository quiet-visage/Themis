#include "tile_layout.h"

#include <assert.h>
#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

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
            recs[action.split_index].width =
                recs[action.split_index].width * 0.5f -
                g_cfg.layout.padding * .5f;
            recs[(*recs_count)] = recs[action.split_index];
            recs[(*recs_count)].x += g_cfg.layout.padding;
            recs[(*recs_count)++].x += recs[action.split_index].width;
        } else {
            recs[action.split_index].height =
                recs[action.split_index].height * 0.5f -
                g_cfg.layout.padding * .5f;
            recs[(*recs_count)] = recs[action.split_index];
            recs[(*recs_count)].y += g_cfg.layout.padding;
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

    assert(0 &&
           "This function should return a valid index, if it doesn't "
           "then there's a bug outside this function");
    return (size_t)-1;
}

size_t tile_get_non_sorted_n(size_t n) {
    assert(n < g_rects_count);

    Rectangle a = g_sorted_mirror[n];
    for (size_t i = 0; i < g_rects_count; i += 1) {
        Rectangle b = g_rects[i];
        if (!memcmp(&a, &b, sizeof(Rectangle))) return i;
    }

    assert(0 &&
           "This function should return a valid index, if it doesn't "
           "then there's a bug outside this function");
    return (size_t)-1;
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
    if (sim_rec.width < MIN_SIZE || sim_rec.height < MIN_SIZE)
        return false;

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

static size_t tile_get_right_rectangles_count(size_t n) {
    size_t count = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].x < g_rects[i].x) {
            count += 1;
        }
    }

    return count;
}

static size_t tile_get_left_rectangles_count(size_t n) {
    size_t count = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].x > g_rects[i].x) {
            count += 1;
        }
    }

    return count;
}

static size_t tile_get_up_rectangles_count(size_t n) {
    size_t count = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].y > g_rects[i].y) {
            count += 1;
        }
    }

    return count;
}

static size_t tile_get_down_rectangles_count(size_t n) {
    size_t count = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].y < g_rects[i].y) {
            count += 1;
        }
    }

    return count;
}

static void tile_get_right_rectangles(size_t n, size_t count,
                                      size_t rectangles[count]) {
    size_t idx = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].x < g_rects[i].x) {
            rectangles[idx] = i;
            idx += 1;
        }
    }

    assert(idx == count);
}

static void tile_get_left_rectangles(size_t n, size_t count,
                                     size_t rectangles[count]) {
    size_t idx = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].x > g_rects[i].x) {
            rectangles[idx] = i;
            idx += 1;
        }
    }

    assert(idx == count);
}

static void tile_get_up_rectangles(size_t n, size_t count,
                                   size_t rectangles[count]) {
    size_t idx = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].y > g_rects[i].y) {
            rectangles[idx] = i;
            idx += 1;
        }
    }

    assert(idx == count);
}

static void tile_get_down_rectangles(size_t n, size_t count,
                                     size_t rectangles[count]) {
    size_t idx = 0;

    for (size_t i = 0; i < g_rects_count; i += 1) {
        if (i == n) continue;
        if (g_rects[n].y < g_rects[i].y) {
            rectangles[idx] = i;
            idx += 1;
        }
    }

    assert(idx == count);
}

size_t tile_get_closest_rectangle(size_t n, size_t count,
                                  size_t rectangles[count]) {
    size_t result = -1;
    float previous_distance = FLT_MAX;

    for (size_t i = 0; i < count; i += 1) {
        Vector2 v1 = {.x = g_rects[n].x, .y = g_rects[n].y};
        Vector2 v2 = {.x = g_rects[rectangles[i]].x,
                      .y = g_rects[rectangles[i]].y};

        float distance = Vector2Distance(v1, v2);

        if (distance < previous_distance) {
            result = rectangles[i];
            previous_distance = distance;
        }
    }

    return result;
}

size_t tile_get_right_of(size_t n) {
    size_t right_rectangles_count =
        tile_get_right_rectangles_count(n);
    if (!right_rectangles_count) return -1;

    size_t right_rectangles[right_rectangles_count];
    tile_get_right_rectangles(n, right_rectangles_count,
                              right_rectangles);

    return tile_get_closest_rectangle(n, right_rectangles_count,
                                      right_rectangles);
}

size_t tile_get_left_of(size_t n) {
    size_t left_rectangles_count = tile_get_left_rectangles_count(n);
    if (!left_rectangles_count) return -1;

    size_t left_rectangles[left_rectangles_count];
    tile_get_left_rectangles(n, left_rectangles_count,
                             left_rectangles);

    return tile_get_closest_rectangle(n, left_rectangles_count,
                                      left_rectangles);
}

size_t tile_get_up_of(size_t n) {
    size_t up_rectangles_count = tile_get_up_rectangles_count(n);
    if (!up_rectangles_count) return -1;

    size_t up_rectangles[up_rectangles_count];
    tile_get_up_rectangles(n, up_rectangles_count, up_rectangles);

    return tile_get_closest_rectangle(n, up_rectangles_count,
                                      up_rectangles);
}

size_t tile_get_down_of(size_t n) {
    size_t down_rectangles_count = tile_get_down_rectangles_count(n);
    if (!down_rectangles_count) return -1;

    size_t down_rectangles[down_rectangles_count];
    tile_get_down_rectangles(n, down_rectangles_count,
                             down_rectangles);

    return tile_get_closest_rectangle(n, down_rectangles_count,
                                      down_rectangles);
}
