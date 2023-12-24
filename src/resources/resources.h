#pragma once
#include <stddef.h>

void resources_init(void);
char* resource_file_load(const char* name);
void resource_file_free(char* file);
