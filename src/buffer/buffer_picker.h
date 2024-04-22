#pragma once
#include <fieldfusion.h>

#include "buffer.h"

void buffer_picker_init(void);
void buffer_picker_terminate(void);
void buffer_picker_update_options(void);
buffer_t* buffer_picker_ui(ff_typo_t typo, int focus_flags);
