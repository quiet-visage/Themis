#pragma once

#include <motion/motion.h>
#include <sys/types.h>

typedef enum {
    cursor_flag_recently_moved = (1 << 0),
    cursor_flag_focused = (1 << 1),
    _cursor_flag_blink_delay = (1 << 2),
    _cursor_flag_blink = (1 << 3),
} cursor_flags_t;

typedef struct {
    motion_t motion;
    unsigned char max_alpha;
    unsigned char alpha;
    int flags;
    double blink_duration_ms;
    double blink_delay_ms;
} cursor_t;

cursor_t cursor_new();
void cursor_draw(cursor_t* c, float x, float y);
