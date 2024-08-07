#pragma once
#include "tools.hh"

struct Motion {
    void init();
    void hard_set(Vec2<float> position);
    void update(Vec2<float> target, float delta);

    float f;
    float z;
    float r;
    float critical_threshold;
    Vec2<float> position;
    Vec2<float> velocity;
    Vec2<float> prev_input;
};
