#pragma once

#include <motion/motion.h>
#include <sys/types.h>

enum cursor_flags {
    cursor_flag_recently_moved_t = (1 << 0),
    cursor_flag_focused_t = (1 << 1),
    _cursor_flag_blink_delay_t = (1 << 2),
    _cursor_flag_blink_t = (1 << 3),
};

struct cursor {
    struct motion motion;
    unsigned char max_alpha;
    unsigned char alpha;
    int flags;
    double blink_duration_ms;
    double blink_delay_ms;
};

struct cursor cursor_new();
void cursor_draw(struct cursor* c, float x, float y);
