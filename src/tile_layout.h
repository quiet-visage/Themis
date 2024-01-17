#pragma once

#include <raylib.h>
#include <stddef.h>

enum split_kind { split_horizontal, split_vertical };

Rectangle tile_get_rect(size_t n);
void tile_set_root(Rectangle rect);
void tile_perform(void);
size_t tile_get_count(void);
bool tile_split(enum split_kind split_kind, size_t n);
size_t tile_get_sorted_n(size_t n);
size_t tile_get_non_sorted_n(size_t n);
void tile_remove(size_t n);
