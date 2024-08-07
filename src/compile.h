#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <fieldfusion.h>
#include <raylib.h>
#include <stddef.h>

void compile_init();
void compile_terminate();
void compile_draw(ff_typo_t typo, Rectangle bounds, int focus);
void compile_set_cmd(char* str, size_t str_len);
void compile_spawn();
void compile_jmp_next_err();
void compile_jmp_prev_err();
void compile_kill();

#ifdef __cplusplus
}
#endif
