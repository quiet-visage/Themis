#pragma once

#include <raylib.h>

#include "fieldfusion.h"
#include "file_editor.h"

void pane_controller_init(Rectangle bounds);
void pane_controller_update_bounds(Rectangle bounds);
void pane_controler_draw(ff_typo_t typo, int focus_flags);
void pane_controller_open_in_focused(const char* file_name);
void pane_controller_terminate(void);
void pane_controller_set_focused_buffer(buffer_t* buffer);
file_editor_t* pane_controller_get_focused(void);
