#pragma once

#include "../fuzzy_menu.h"
#include "fieldfusion.h"

#define PATH_MAX_LEN 256

struct File_Picker {
    struct Dimensions {
        fuzzy_menu_dimensions_t menu_dimensions;

        float preview_bg_x;
        float preview_bounds_x;

        float preview_bg_y;
        float preview_bounds_y;

        float preview_bg_width;
        float preview_bounds_width;

        float preview_bg_height;
        float preview_bounds_height;
    };

    fuzzy_menu_t menu;
    char dir[PATH_MAX_LEN];
    char result[PATH_MAX_LEN];

    void init();
    void destroy();
    void reload_options();
    void draw_preview(ff_typo_t typo,
                      File_Picker::Dimensions dimensions,
                      int focus_flags);
    Dimensions get_dimensions(ff_typo_t typo);
    char *ui_view(ff_typo_t typo, int focus_flags);
};
