#pragma once

#include <sys/types.h>

#include "motion.h"

enum cursor_flags {
    cursor_flag_recently_moved_t = (1 << 0),
    cursor_flag_focused_t = (1 << 1),
    _cursor_flag_blink_delay_t = (1 << 2),
    _cursor_flag_blink_t = (1 << 3),
};

struct cursor {
    struct motion motion;
    // struct motion smear_motion;
    float blink_duration_ms;
    float blink_delay_ms;
    unsigned char max_alpha;
    unsigned char alpha;
    unsigned char flags;
};

void cursor_initialize(void);
void cursor_terminate(void);
struct cursor cursor_new(void);
void cursor_draw(struct cursor* c, float x, float y);
