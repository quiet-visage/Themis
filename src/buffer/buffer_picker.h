#pragma once
#include <fieldfusion.h>

void buffer_picker_init(void);
void buffer_picker_terminate(void);
void buffer_picker_update_options(void);
struct buffer* buffer_picker_ui(struct ff_typography typo,
                                int focus_flags);
