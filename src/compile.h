#pragma once

#include <fieldfusion.h>
#include <raylib.h>
#include <stddef.h>

void compile_init();
void compile_terminate();
void compile_draw(struct ff_typography typo, Rectangle bounds,
                  int focus);
void compile_set_cmd(char* str, size_t str_len);
void compile_spawn();
void compile_kill();
