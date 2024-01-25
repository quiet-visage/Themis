#include "cursor.h"

#include <math.h>
#include <stdio.h>

#include "../motion/motion.h"
#include "raylib.h"

__asm__("g_cursor_frag: .incbin \"shaders/cursor.frag\"");
__asm__("g_cursor_frag_end: .byte 0");

extern const char g_cursor_frag[];
static Shader g_cursor_shader;

void cursor_initialize(void) {
    g_cursor_shader = LoadShaderFromMemory(0, g_cursor_frag);
}

void cursor_terminate(void) { UnloadShader(g_cursor_shader); }

struct cursor cursor_new() {
    struct cursor result = {0};
    result.motion.f = 4.5f;
    result.motion.z = 1.0f;
    result.motion.r = -1.0f;
    result.smear_motion = result.motion;
    result.smear_motion.f += .05f;
    return result;
}

static inline float cursor_compute_phase(void) {
    return fabs(cosf(GetTime() * 2)) * .7f;
}

static void cursor_handle_alplha_change(struct cursor* this) {
    if (this->flags & cursor_flag_focused_t) {
        this->max_alpha = 0xff;
        this->alpha = this->max_alpha;
    } else {
        this->max_alpha = 0x30;
        this->alpha = this->max_alpha;
    }

    if (this->flags & cursor_flag_recently_moved_t) {
        this->flags &= ~cursor_flag_recently_moved_t;
        this->flags |= _cursor_flag_blink_delay_t;
        this->flags &= ~_cursor_flag_blink_t;
        this->blink_delay_ms = 1e3;
        this->alpha = 0xff;
    }

    if (this->flags & _cursor_flag_blink_delay_t) {
        this->blink_delay_ms -= GetFrameTime() * 1e3;
        if (this->blink_delay_ms <= 0 &&
            (cursor_compute_phase() - .7f) >= -.01f) {
            this->blink_duration_ms = 8*1e3;
            this->flags &= ~_cursor_flag_blink_delay_t;
            this->flags |= _cursor_flag_blink_t;
        }
    }

    if (this->flags ^ _cursor_flag_blink_delay_t &&
        this->flags & _cursor_flag_blink_t) {
        float phase = cursor_compute_phase();
        this->alpha = phase * this->max_alpha;
        this->blink_duration_ms -= GetFrameTime() * 1e3;
        if (this->blink_duration_ms <= 0 && .7f - phase < .1f) {
            this->flags &= ~_cursor_flag_blink_t;
            this->alpha = 0xff;
        }
    }
}

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

    DrawRectangleRec(
        (Rectangle){.x = this->motion.position[0],
                    .y = this->motion.position[1],
                    .width = 2.0f,
                    .height = 12.f},
        (Color){.r = 0xff, .g = 0xff, .b = 0xff, .a = this->alpha});

    // draw_cursor_trail(this);
}
