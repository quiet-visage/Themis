#pragma once

#include <stddef.h>

#include "buffer.h"
#include "dyn_strings/utf32_string.h"
#include "text/text_view.h"

void buffer_handler_init(void);
struct buffer* buffer_handler_get(const char* name);
struct buffer* buffer_handler_create_buffer(const char* name);
void buffer_handler_terminate(void);
size_t buffer_handler_count(void);
void buffer_handler_list_names(size_t count,
                               struct utf32_str names[count]);
