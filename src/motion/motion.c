#include "motion.h"

#include <math.h>

#include "assert.h"

typedef unsigned long ulong;

static const float pi = 3.14159265358979323846f;

struct motion motion_new() {
    struct motion result = {0};
    result.f = 1.0f;
    result.z = 0.5f;
    result.r = 2.0f;
    return result;
}

void motion_update(struct motion* m, float target[2], float delta_time) {
    assert(m->f <= 6.0f);

    float k1 = m->z / (pi * m->f);
    float k2 = 1 / ((2 * pi * m->f) * (2 * pi * m->f));
    float k3 = m->r * m->z / (2 * pi * m->f);
    m->critical_threshold = 0.8f * (sqrtf(4 * k2 + k1 * k1) - k1);

    float estimated_velocity[2] = {target[0] - m->previous_input[0],
                                   target[1] - m->previous_input[1]};
    m->previous_input[0] = target[0];
    m->previous_input[1] = target[1];

    ulong iterations =
        (ulong)ceilf(delta_time / m->critical_threshold);
    delta_time = delta_time / iterations;

    for (ulong i = 0; i < iterations; i++) {
        m->position[0] = m->position[0] + delta_time * m->velocity[0];
        m->position[1] = m->position[1] + delta_time * m->velocity[1];

        m->velocity[0] = m->velocity[0] +
                         delta_time *
                             (target[0] + k3 * estimated_velocity[0] -
                              m->position[0] - k1 * m->velocity[0]) /
                             k2;
        m->velocity[1] = m->velocity[1] +
                         delta_time *
                             (target[1] + k3 * estimated_velocity[1] -
                              m->position[1] - k1 * m->velocity[1]) /
                             k2;
    }
}
