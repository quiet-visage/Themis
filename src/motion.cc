#include "motion.hh"

#include <math.h>

void Motion::init() {
    f = 1.;
    z = .5;
    r = 2.;
}

void Motion::hard_set(Vec2<float> position) {}

void Motion::update(Vec2<float> target, float delta) {
    static const double pi = M_PI;
    assert(f <= 6.0f);

    float k1 = z / (pi * f);
    float k2 = 1 / ((2 * pi * f) * (2 * pi * f));
    float k3 = r * z / (2 * pi * f);
    critical_threshold = 0.8f * (sqrtf(4 * k2 + k1 * k1) - k1);

    Vec2<float> estimated_velocity = {target.x - prev_input.x,
                                      target.y - prev_input.y};
    prev_input.x = target.x;
    prev_input.y = target.y;

    ulong iterations = (ulong)ceilf(delta / critical_threshold);
    delta = delta / iterations;

    for (ulong i = 0; i < iterations; i++) {
        position = position + delta * velocity;
        position.x = position.x + delta * velocity.x;
        position.y = position.y + delta * velocity.y;

        velocity.x =
            velocity.x + delta *
                             (target.x + k3 * estimated_velocity.x -
                              position.x - k1 * velocity.x) /
                             k2;
        velocity.y =
            velocity.y + delta *
                             (target.y + k3 * estimated_velocity.y -
                              position.y - k1 * velocity.y) /
                             k2;
    }
}
