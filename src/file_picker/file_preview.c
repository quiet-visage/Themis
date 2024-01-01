#include "file_preview.h"

#include <assert.h>
#include <file_picker/file_preview.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "field_fusion/fieldfusion.h"
#define MAP_SIZE 0x800

struct map_entry {
    struct file_preview file_preview;
    const char* key;
    struct map_entry* next;
};

struct map {
    struct map_entry* entries;
};

static struct map file_preview_cache = {0};

static struct file_preview file_preview_create(const char* path) {
    struct file_preview result = {0};
    result.path = path;
    result.path_last_modified = GetFileModTime(path);
    result.text = text_create();
    string32_copy_file(&result.text.buffer, path);

    const char* file_extension = GetFileExtension(path);
    if (file_extension) {
        enum language file_language = hlr_get_extension_language(file_extension);
        text_set_syntax_language(&result.text, file_language);
    }
    
    text_on_modified(&result.text);
    return result;
}

static void file_preview_destroy(struct file_preview* fp) {
    text_destroy(&fp->text);
}

static size_t path_key_hash(const char* path) {
    size_t path_len = strlen(path);
    size_t len = path_len > 32 ? path_len : 32;

    size_t hash = 0xcbf29ce484222325;
    for (size_t i = path_len; i > (path_len - (path_len>32 ? 32 : path_len)); i -= 1) {
        hash *= 0x100000001b3;
        hash ^= path[i];
    }

    return hash;
}

static struct map map_create(void) {
    return (struct map){
        .entries = calloc(sizeof(struct map_entry), MAP_SIZE)};
}

static void map_entry_destroy(struct map_entry* entry) {
    entry->key = NULL;
    file_preview_destroy(&entry->file_preview);
    if (entry->next) {
        map_entry_destroy(entry->next);
        free(entry->next);
        entry->next = NULL;
    }
}

static struct map_entry* map_ll_tail(struct map_entry* entry) {
    assert(entry != NULL);
    struct map_entry* result = entry;
    while (result->next) result = result->next;

    return result;
}

static struct file_preview* map_get(struct map* map,
                                    const char* path) {
    size_t slot = path_key_hash(path) % MAP_SIZE;
    struct map_entry* entry = &map->entries[slot];
    while (entry) {
        if (entry->key == NULL) return NULL;
        if (strcmp(entry->key, path) == 0) {
            return &entry->file_preview;
        }
        entry = entry->next;
    }
    return NULL;
}

static void map_set(struct map* map, const char* path,
                    struct file_preview file_preview) {
    size_t slot = path_key_hash(path) % MAP_SIZE;

    struct map_entry* entry = &map->entries[slot];

    if (entry->key == NULL) {
        entry->file_preview = file_preview;
        entry->key = path;
    } else if (entry->key == path) {
        file_preview_destroy(&entry->file_preview);
        entry->file_preview = file_preview;
    } else {
        struct map_entry* tail = map_ll_tail(entry);
        tail->next = calloc(sizeof(struct map_entry), 1);
        tail->next->key = path;
        tail->next->file_preview = file_preview;
    }
}

static void map_destroy(struct map* map) {
    for (size_t i = 0; i < MAP_SIZE; i += 1) {
        if (!map->entries[i].key) continue;
        map_entry_destroy(&map->entries[i]);
    }
    free(map->entries);
    map->entries = NULL;
}

void preview_init(void) { file_preview_cache = map_create(); }

void preview_terminate(void) { map_destroy(&file_preview_cache); }

struct file_preview* get_preview(const char* path) {
    struct file_preview* preview = map_get(&file_preview_cache, path);

    long file_mod_time = GetFileModTime(path);
    if (preview && file_mod_time >= preview->path_last_modified)
        return preview;

    if (!IsPathFile(path)) return NULL;

    struct file_preview new_preview = file_preview_create(path);
    map_set(&file_preview_cache, path, new_preview);
    return map_get(&file_preview_cache, path);
}
