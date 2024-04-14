#pragma once
#include <stddef.h>

struct selection {
    size_t from_line;
    size_t from_col;
    size_t to_line;
    size_t to_col;
};
