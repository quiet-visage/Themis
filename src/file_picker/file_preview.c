#include "file_preview.h"

#include <assert.h>
#include <fieldfusion.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_SIZE 0x800
#define FILE_PREVIEW_PATH_CAP 0x100

typedef struct map_entry {
    file_preview_t file_preview;
    struct map_entry* next;
    char key[FILE_PREVIEW_PATH_CAP];
} map_entry_t;

typedef struct {
    map_entry_t* entries;
} map_t;

static map_t file_preview_cache = {0};

void file_preview_create(file_preview_t* o, const char* path) {
    buffer_create(&o->buffer, utf32_str_create());
    o->path_last_modified = GetFileModTime(path);
    o->text = text_view_create();
    o->text.buffer = &o->buffer;

    buffer_read_file(o->text.buffer, path);

    const char* file_extension = GetFileExtension(path);
    if (file_extension) {
        enum language file_language =
            hlr_get_extension_language(file_extension);
        buffer_syntax_set_language(&o->text.buffer->syntax,
                                   file_language);
        buffer_syntax_update(&o->text.buffer->syntax,
                             o->text.buffer->str.data,
                             o->text.buffer->str.length);
    }
}

static void file_preview_destroy(file_preview_t* fp) {
    text_view_destroy(&fp->text);
    buffer_destroy(&fp->buffer);
}

static size_t path_key_hash(const char* path) {
    size_t path_len = strlen(path);
    size_t len = path_len > FILE_PREVIEW_PATH_CAP
                     ? path_len
                     : FILE_PREVIEW_PATH_CAP;

    size_t hash = 0xcbf29ce484222325;
    for (size_t i = len; i-- > len;) {
        hash *= 0x100000001b3;
        hash ^= path[i];
    }

    return hash;
}

static map_t map_create(void) {
    return (map_t){.entries = calloc(sizeof(map_entry_t), MAP_SIZE)};
}

static void map_entry_destroy(map_entry_t* entry) {
    entry->key[0] = 0;
    file_preview_destroy(&entry->file_preview);
    if (entry->next) {
        map_entry_destroy(entry->next);
        free(entry->next);
        entry->next = NULL;
    }
}

static map_entry_t* map_ll_tail(map_entry_t* entry) {
    assert(entry != NULL);
    map_entry_t* result = entry;
    while (result->next) result = result->next;

    return result;
}

static file_preview_t* map_get(map_t* map, const char* path) {
    size_t slot = path_key_hash(path) % MAP_SIZE;
    map_entry_t* entry = &map->entries[slot];
    while (entry) {
        if (!entry->key[0]) return NULL;
        if (strcmp(entry->key, path) == 0) {
            return &entry->file_preview;
        }
        entry = entry->next;
    }
    return NULL;
}

static void map_assign_path_to_key(char* key, const char* path) {
    size_t path_len = strlen(path);

    if (path_len > FILE_PREVIEW_PATH_CAP) {
        memcpy(key, &path[path_len - FILE_PREVIEW_PATH_CAP],
               FILE_PREVIEW_PATH_CAP);
        return;
    }

    memcpy(key, path, path_len);
}

static map_entry_t* map_ll_find(map_entry_t* entry, const char* key) {
    assert(entry);
    size_t key_len = strlen(key);
    while (entry) {
        if (!strncmp(entry->key, key, key_len)) return entry;
        entry = entry->next;
    }
    return NULL;
}

static void map_update_entry(map_t* o, const char* path) {
    size_t slot = path_key_hash(path) % MAP_SIZE;

    map_entry_t* entry = &o->entries[slot];

    if (!entry->key[0]) {
        map_assign_path_to_key(entry->key, path);
        file_preview_create(&entry->file_preview, path);
    } else if (!strcmp(entry->key, path)) {
        long file_mod_time = GetFileModTime(path);
        if (file_mod_time > entry->file_preview.path_last_modified) {
            entry->file_preview.path_last_modified = file_mod_time;
            buffer_read_file(&entry->file_preview.buffer, path);
        }
    } else {
        map_entry_t* ll_entry = map_ll_find(entry, path);

        if (ll_entry) {
            long file_mod_time = GetFileModTime(path);
            if (file_mod_time >
                ll_entry->file_preview.path_last_modified) {
                ll_entry->file_preview.path_last_modified =
                    file_mod_time;
                buffer_read_file(&ll_entry->file_preview.buffer,
                                 path);
            }
            return;
        }

        map_entry_t* tail = map_ll_tail(entry);
        tail->next = calloc(sizeof(map_entry_t), 1);
        map_assign_path_to_key(tail->next->key, path);
        file_preview_create(&tail->next->file_preview, path);
    }
}

static void map_destroy(map_t* map) {
    for (size_t i = 0; i < MAP_SIZE; i += 1) {
        if (!map->entries[i].key[0]) continue;
        map_entry_destroy(&map->entries[i]);
    }
    free(map->entries);
    map->entries = NULL;
}

void preview_init(void) { file_preview_cache = map_create(); }

void preview_terminate(void) { map_destroy(&file_preview_cache); }

file_preview_t* get_preview(const char* path) {
    if (!IsPathFile(path)) return NULL;

    map_update_entry(&file_preview_cache, path);
    file_preview_t* result = map_get(&file_preview_cache, path);
    if (!result) return 0;
    return result;
}
