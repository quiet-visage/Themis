#include <text/cursor.h>

#include <math.h>

#include <motion/motion.h>
#include "raylib.h"

cursor_t cursor_new() {
    cursor_t result = {0};
    result.motion.f = 3.0f;
    result.motion.z = 0.9f;
    return result;
}

static void cursor_handle_alplha_change(cursor_t* c) {
    if (c->flags & cursor_flag_focused) {
        c->max_alpha = 0xff;
        c->alpha = c->max_alpha;
    } else {
        c->max_alpha = 0x60;
        c->alpha = c->max_alpha;
        return;
    }

    if (c->flags & cursor_flag_recently_moved) {
        c->flags ^= cursor_flag_recently_moved;
        c->flags |= _cursor_flag_blink_delay;
        c->blink_delay_ms = 1e3;
        c->alpha = 0xff;
    }

    if (c->flags & _cursor_flag_blink_delay) {
        c->blink_delay_ms -= GetFrameTime() * 1e3;
        if (c->blink_delay_ms <= 0) {
            c->blink_duration_ms = 10 * 1e3;
            c->flags ^= _cursor_flag_blink_delay;
            c->flags |= _cursor_flag_blink;
        }
    }

    if (c->flags ^ _cursor_flag_blink_delay &&
        c->flags & _cursor_flag_blink) {
        c->alpha =
            fabs(cosf(c->blink_duration_ms * 1.5)) * c->max_alpha;
        c->blink_duration_ms -= GetFrameTime() * 1e3;
        if (c->blink_duration_ms <= 0) {
            c->flags ^= _cursor_flag_blink;
            c->alpha = 0xff;
        }
    }
}

void cursor_draw(cursor_t* c, float x, float y) {
    cursor_handle_alplha_change(c);
    motion_update(&c->motion, (float[2]){x, y}, GetFrameTime());
    DrawRectangleRec((Rectangle){.x = c->motion.position[0],
                                 .y = c->motion.position[1],
                                 .width = 2.0f,
                                 .height = 12.f},
                     WHITE);
}
