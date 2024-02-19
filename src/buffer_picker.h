#pragma once
#include "buffer_handler.h"

void buffer_picker_init(void);
void buffer_picker_terminate(void);
void buffer_picker_update(void);
struct buffer* buffer_picker_perform(struct ff_typography typo,
                                     int focus_flags);
