#pragma once

#include <sys/types.h>

#include "motion.h"

enum cursor_flags {
    cursor_flag_recently_moved_t = (1 << 0),
    cursor_flag_focused_t = (1 << 1),
    _cursor_flag_blink_delay_t = (1 << 2),
    _cursor_flag_blink_t = (1 << 3),
};

typedef struct {
    motion_t motion;
    // motion_t smear_motion;
    float blink_duration_ms;
    float blink_delay_ms;
    unsigned char max_alpha;
    unsigned char alpha;
    unsigned char flags;
} cursor_t;

void cursor_initialize(void);
void cursor_terminate(void);
cursor_t cursor_new(void);
void cursor_draw(cursor_t* c, float x, float y);
