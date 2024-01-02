#pragma once
#include <highlighter/highlighter.h>
#include <raylib.h>

struct layout {
    float text_spacing;
    float padding;
    float gap;
};

struct color_scheme {
    int bg;
    int fg;
    int text_sel_bg;
    int selected_fg;
    int selected_bg;
    int surface0_bg;
    int surface1_bg;
    int syntax[token_kind_count_t];
};

extern struct color_scheme g_color_scheme;
extern struct layout g_layout;
