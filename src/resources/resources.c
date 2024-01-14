#include "resources.h"

#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESOURCES_DIR "resources"
#define ICON_SIZE 14

static char g_resources_dir_path[256] = {0};
static size_t g_resources_dir_path_len = 0;
static const char* g_icon_path[icon_count_t] = {
    [icon_folder_t] = "icons/bxs-folder.svg",
    [icon_file_t] = "icons/bxs-file-blank.svg",
};
Texture g_icons[icon_count_t] = {0};

void resources_init(void) {
    const char* app_dir = GetApplicationDirectory();
    size_t app_dir_len = strlen(app_dir);
    assert(app_dir_len + strlen(RESOURCES_DIR) < 256);

    memcpy(g_resources_dir_path, app_dir, app_dir_len);
    memcpy(&g_resources_dir_path[app_dir_len], RESOURCES_DIR,
           strlen(RESOURCES_DIR));
    g_resources_dir_path_len = strlen(g_resources_dir_path);

    char icon_path[256] = {0};
    memcpy(icon_path, g_resources_dir_path, g_resources_dir_path_len);
    icon_path[g_resources_dir_path_len] = '/';
    for (size_t i = 0; i < icon_count_t; i += 1) {
        memcpy(&icon_path[g_resources_dir_path_len + 1],
               g_icon_path[i], strlen(g_icon_path[i]));
        assert(FileExists(icon_path));

        printf("%s\n", icon_path);
        Image icon_image =
            LoadImageSvg(icon_path, ICON_SIZE, ICON_SIZE);
        ImageColorInvert(&icon_image);
        g_icons[i] = LoadTextureFromImage(icon_image);
        UnloadImage(icon_image);

        icon_path[g_resources_dir_path_len + 1] = 0;
    }
}

char* resource_file_load(const char* name) {
    assert(g_resources_dir_path_len != 0);
    char file_path[256] = {0};
    memcpy(file_path, g_resources_dir_path, g_resources_dir_path_len);
    memcpy(&file_path[g_resources_dir_path_len], "/", 1);
    memcpy(&file_path[g_resources_dir_path_len + 1], name,
           strlen(name));

    assert(FileExists(file_path));

    FILE* file = fopen(file_path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* result = calloc(1, file_size + 1);
    fread(result, 1, file_size, file);
    fclose(file);

    return result;
}

void resource_file_free(char* resource_file) {
    free(resource_file);
    resource_file = 0;
}

Texture get_icon(enum icon icon) { return g_icons[icon]; }
