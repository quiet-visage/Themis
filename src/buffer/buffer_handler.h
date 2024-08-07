#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>

#include "../dyn_strings/utf32_string.h"
#include "buffer.h"

void buffer_handler_init(void);
buffer_t* buffer_handler_get(const char* name);
buffer_t* buffer_handler_create_buffer(const char* name);
void buffer_handler_terminate(void);
size_t buffer_handler_count(void);
void buffer_handler_list_names(size_t count, utf32_str_t* names);

#ifdef __cplusplus
}
#endif
