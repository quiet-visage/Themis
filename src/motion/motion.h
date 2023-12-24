#pragma once

typedef struct {
  float f;
  float z;
  float r;
  float position[2];
  float velocity[2];
  float previous_input[2];
  float critical_threshold;
}motion_t;

motion_t motion_new();
void motion_update(motion_t* m, float target[2], float delta_time);
