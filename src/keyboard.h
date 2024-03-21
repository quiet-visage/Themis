#pragma once

#include "GLFW/glfw3.h"

void key_callback(GLFWwindow *window, int key, int scancode,
                  int action, int mods);
void char_callback(GLFWwindow *window, unsigned int key);
int get_sticky_key(void);
int get_last_pressed_key(void);
int get_char(void);
int is_key_down(int key);
void kb_end_frame(void);
int is_key_sticky(int key);
