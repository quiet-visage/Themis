#pragma once
#include <raylib.h>
#include <stddef.h>

enum icon {
    icon_none_t = -1,
    icon_folder_t,
    icon_file_t,
    icon_count_t
};

void resources_init(void);
char* resource_file_load(const char* name);
void resource_file_free(char* file);
Texture get_icon(enum icon icon);
