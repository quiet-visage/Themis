#include <assert.h>
#include <raylib.h>
#include <resources/resources.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define RESOURCES_DIR "resources"

static char resources_dir_path[256] = {0};
static size_t resources_dir_path_len = 0;

void resources_init(void) {
    const char* app_dir = GetApplicationDirectory();
    size_t app_dir_len = strlen(app_dir);
    assert(app_dir_len + strlen(RESOURCES_DIR) < 256);

    memcpy(resources_dir_path, app_dir, app_dir_len);
    memcpy(&resources_dir_path[app_dir_len], RESOURCES_DIR, strlen(RESOURCES_DIR));

    resources_dir_path_len = strlen(resources_dir_path);
}


char* resource_file_load(const char* name) {
    assert(resources_dir_path_len != 0);
    char file_path[256] = {0};
    memcpy(file_path, resources_dir_path, resources_dir_path_len);
    memcpy(&file_path[resources_dir_path_len], "/", 1);
    memcpy(&file_path[resources_dir_path_len + 1], name, strlen(name));

    assert(FileExists(file_path));

    FILE* file = fopen(file_path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file); 
    rewind(file);

    char* result = calloc(1, file_size+1);
    fread(result, 1, file_size, file);
    fclose(file);

    return result;
}

void resource_file_free(char* resource_file){
    free(resource_file);
    resource_file = 0;
}
