#include "file_preview.h"

#include <assert.h>
#include <fieldfusion.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "../buffer_handler.h"

#define MAP_SIZE 0x800
#define FILE_PREVIEW_PATH_CAP 0x100

struct map_entry {
    struct file_preview file_preview;
    struct map_entry* next;
    char key[FILE_PREVIEW_PATH_CAP];
};

struct map {
    struct map_entry* entries;
};

static struct map file_preview_cache = {0};

void file_preview_create(struct file_preview* o, const char* path) {
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
                             o->text.buffer->str.size);
    }
}

static void file_preview_destroy(struct file_preview* fp) {
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

static struct map map_create(void) {
    return (struct map){
        .entries = calloc(sizeof(struct map_entry), MAP_SIZE)};
}

static void map_entry_destroy(struct map_entry* entry) {
    entry->key[0] = 0;
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

static struct map_entry* map_ll_find(struct map_entry* entry, const char* key) {
    assert(entry);
    size_t key_len = strlen(key);
    while (entry) {
        if (!strncmp(entry->key, key, key_len)) return entry;
        entry = entry->next;
    }
    return NULL;
}

static void map_update_entry(struct map* o, const char* path) {
    size_t slot = path_key_hash(path) % MAP_SIZE;

    struct map_entry* entry = &o->entries[slot];

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
        struct map_entry* ll_entry = map_ll_find(entry, path);

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

        struct map_entry* tail = map_ll_tail(entry);
        tail->next = calloc(sizeof(struct map_entry), 1);
        map_assign_path_to_key(tail->next->key, path);
        file_preview_create(&tail->next->file_preview, path);
    }
}

static void map_destroy(struct map* map) {
    for (size_t i = 0; i < MAP_SIZE; i += 1) {
        if (!map->entries[i].key[0]) continue;
        map_entry_destroy(&map->entries[i]);
    }
    free(map->entries);
    map->entries = NULL;
}

void preview_init(void) { file_preview_cache = map_create(); }

void preview_terminate(void) { map_destroy(&file_preview_cache); }

struct file_preview* get_preview(const char* path) {
    if (!IsPathFile(path)) return NULL;

    map_update_entry(&file_preview_cache, path);
    struct file_preview* result = map_get(&file_preview_cache, path);
    assert(result);
    return result;
}
