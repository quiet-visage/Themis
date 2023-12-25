#pragma once

struct motion {
    float f;
    float z;
    float r;
    float position[2];
    float velocity[2];
    float previous_input[2];
    float critical_threshold;
};

struct motion motion_new();
void motion_update(struct motion* m, float target[2],
                   float delta_time);
