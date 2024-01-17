#pragma once

#include <raylib.h>
#include <stddef.h>

#include "field_fusion/fieldfusion.h"

void pane_controller_init(Rectangle bounds);
void pane_controller_update_bounds(Rectangle bounds);
void pane_controler_draw(struct ff_typography typo, int focus_flags);
void pane_controller_open_in_focused(const char* file_name);
void pane_controller_terminate(void);
