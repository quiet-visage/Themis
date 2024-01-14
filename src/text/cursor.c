#include "cursor.h"

#include <math.h>

#include "../motion/motion.h"
#include "raylib.h"


__asm__("g_cursor_frag: .incbin \"shaders/cursor.frag\"");
__asm__("g_cursor_frag_end: .byte 0");

extern const char g_cursor_frag[]; 
static Shader g_cursor_shader;

void cursor_initialize(void) {
    g_cursor_shader = LoadShaderFromMemory(0, g_cursor_frag);
}

void cursor_terminate(void) {
    UnloadShader(g_cursor_shader);
}

struct cursor cursor_new() {
    struct cursor result = {0};
    result.motion.f = 2.5f;
    result.motion.z = 1.0f;
    result.motion.r = -1.0f;
    result.smear_motion = result.motion;
    result.smear_motion.f += .05f;
    return result;
}

static void cursor_handle_alplha_change(struct cursor* c) {
    if (c->flags & cursor_flag_focused_t) {
        c->max_alpha = 0xff;
        c->alpha = c->max_alpha;
    } else {
        c->max_alpha = 0x60;
        c->alpha = c->max_alpha;
        return;
    }

    if (c->flags & cursor_flag_recently_moved_t) {
        c->flags ^= cursor_flag_recently_moved_t;
        c->flags |= _cursor_flag_blink_delay_t;
        c->blink_delay_ms = 1e3;
        c->alpha = 0xff;
    }

    if (c->flags & _cursor_flag_blink_delay_t) {
        c->blink_delay_ms -= GetFrameTime() * 1e3;
        if (c->blink_delay_ms <= 0) {
            c->blink_duration_ms = 10 * 1e3;
            c->flags ^= _cursor_flag_blink_delay_t;
            c->flags |= _cursor_flag_blink_t;
        }
    }

    if (c->flags ^ _cursor_flag_blink_delay_t &&
        c->flags & _cursor_flag_blink_t) {
        c->alpha =
            fabs(cosf(c->blink_duration_ms * 1.5)) * c->max_alpha;
        c->blink_duration_ms -= GetFrameTime() * 1e3;
        if (c->blink_duration_ms <= 0) {
            c->flags ^= _cursor_flag_blink_t;
            c->alpha = 0xff;
        }
    }
}

#define MIN(x, y) (x < y ? x : y)

static void draw_cursor_trail(struct cursor* this) {
    Vector2 top_vertex;
    Vector2 mid_vertex;
    Vector2 bot_vertex;

    if (this->motion.position[0] < this->smear_motion.position[0]) {
        top_vertex.x = this->motion.position[0];
        top_vertex.y = this->motion.position[1];
        mid_vertex.x = this->motion.position[0];
        mid_vertex.y = this->motion.position[1] + 12.f;
        bot_vertex.x = this->smear_motion.position[0];
        bot_vertex.y = this->smear_motion.position[1] + 6.0f;
    } else {
        top_vertex.x = this->motion.position[0];
        top_vertex.y = this->motion.position[1];
        mid_vertex.x = this->smear_motion.position[0];
        mid_vertex.y = this->smear_motion.position[1] + 6.0f;
        bot_vertex.x = this->motion.position[0];
        bot_vertex.y = this->motion.position[1] + 12.f;
    }


    BeginShaderMode(g_cursor_shader);
    DrawTriangle(top_vertex, mid_vertex, bot_vertex, SKYBLUE);
    EndShaderMode();
}

void cursor_draw(struct cursor* this, float x, float y) {
    cursor_handle_alplha_change(this);

    motion_update(&this->motion, (float[2]){x, y}, GetFrameTime());
    motion_update(&this->smear_motion,
                  (float[2]){this->motion.position[0],
                             this->motion.position[1]},
                  GetFrameTime());

    DrawRectangleLinesEx((Rectangle){.x = this->motion.position[0],
                                     .y = this->motion.position[1],
                                     .width = 2.0f,
                                     .height = 12.f},
                         2.0f, WHITE);

    draw_cursor_trail(this);
    // DrawRectangleRec(
    //     (Rectangle){
    //         .x = c->smear_motion.position[0] <
    //         c->motion.position[0]
    //                  ? c->smear_motion.position[0]
    //                  : c->motion.position[0],
    //         .y = c->smear_motion.position[1] >
    //         c->motion.position[1]
    //                  ? c->smear_motion.position[1]
    //                  : c->motion.position[1],
    //         .width = fabs(c->motion.position[0] -
    //                       c->smear_motion.position[0]),
    //         .height = 12.f},
    //     WHITE);
}
