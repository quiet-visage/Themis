#pragma once
#include <raylib.h>

#include "../highlighter/highlighter.h"

struct layout {
    float text_spacing;
    float padding;
    float gap;
};

struct color_scheme {
    int bg;
    int fg;
    int text_mute;
    int text_sel_bg;
    int selected_fg;
    int selected_bg;
    int surface0_bg;
    int surface1_bg;
    int highlight_fg;
    int syntax[token_kind_count_t];
};

struct config {
    struct layout layout;
    struct color_scheme color_scheme;
    unsigned char scroll_off;
};

extern struct config g_cfg;
