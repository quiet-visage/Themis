#include "buffer_handler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_strings/utf32_string.h"
#include "text/text_view.h"
#define BUFFER_MAP_SIZE 1024
#define TODO #error "todo"

struct buffer_map_entry {
    struct buffer pair;
    struct buffer_map_entry* next;
};

struct buffer_map {
    struct buffer_map_entry entries[BUFFER_MAP_SIZE];
    size_t item_count;
};

static size_t cooked_hash(const char* str, size_t str_len) {
    static const size_t max_len = 26;
    size_t len = max_len < str_len ? max_len : str_len;
    size_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i += 1) {
        hash *= 0x100000001b3;
        hash ^= str[i];
    }
    return hash;
}

static void buffer_map_pair_destroy(struct buffer* o) {
    utf32_str_destroy(&o->string);
    text_view_destroy(&o->preview);
    memset(o, 0, sizeof(struct buffer));
}

static void buffer_map_entry_destroy(struct buffer_map_entry* o) {
    if (o->next) buffer_map_entry_destroy(o->next);
    buffer_map_pair_destroy(&o->pair);
    memset(&o->pair, 0, sizeof(struct buffer));
    if (o->next) {
        free(o->next);
        o->next = NULL;
    }
}

static void buffer_map_destroy(struct buffer_map* o) {
    for (size_t i = 0; i < BUFFER_MAP_SIZE; i += 1) {
        if (o->entries[i].pair.buffer_name[0])
            buffer_map_entry_destroy(&o->entries[i]);
    }
}

static struct buffer* buffer_map_get(struct buffer_map* o,
                                     const char* buffer_name) {
    size_t slot = cooked_hash(buffer_name, strlen(buffer_name)) %
                  BUFFER_MAP_SIZE;

    struct buffer_map_entry* entry = &o->entries[slot];
    while (entry) {
        if (entry->pair.buffer_name[0] &&
            !strcmp(entry->pair.buffer_name, buffer_name))
            return &entry->pair;
        entry = entry->next;
    }

    return 0;
}

static void buffer_map_assign_buffer_name(struct buffer* pair,
                                          const char* buffer_name) {
    size_t buffer_name_len = strlen(buffer_name);

    if (buffer_name_len > BUFFER_NAME_CAP) {
        memcpy(pair->buffer_name,
               &buffer_name[buffer_name_len - BUFFER_NAME_CAP],
               BUFFER_NAME_CAP);
        return;
    }

    memcpy(pair->buffer_name, buffer_name, buffer_name_len);
}

static void buffer_map_set(struct buffer_map* o,
                           const char* buffer_name,
                           struct utf32_str string) {
    size_t slot = cooked_hash(buffer_name, strlen(buffer_name)) %
                  BUFFER_MAP_SIZE;

    struct buffer_map_entry* entry = &o->entries[slot];

    if (!entry->pair.buffer_name[0]) {
        buffer_map_assign_buffer_name(&entry->pair, buffer_name);

        entry->pair.preview = text_view_create();
        entry->pair.preview.buffer = &entry->pair.string;
        text_view_on_modified(&entry->pair.preview);

        entry->pair.string = string;
        o->item_count += 1;
        return;
    } else if (!strcmp(entry->pair.buffer_name, buffer_name)) {
        entry->pair.string = string;
        text_view_on_modified(&entry->pair.preview);
        return;
    }

    while (entry->next) {
        entry = entry->next;

        if (entry && !strcmp(entry->pair.buffer_name, buffer_name)) {
            entry->pair.string = string;
            return;
        }
    }

    entry->next = calloc(sizeof(struct buffer_map_entry), 1);
    assert(entry->next);
    buffer_map_assign_buffer_name(&entry->next->pair, buffer_name);

    entry->next->pair.string = string;
    entry->next->pair.preview = text_view_create();
    entry->next->pair.preview.buffer = &entry->pair.string;
    text_view_on_modified(&entry->next->pair.preview);

    o->item_count += 1;
}

static struct buffer_map g_buffer_map = {0};

void buffer_handler_init(void) {
    buffer_map_set(&g_buffer_map, "[scratch]", utf32_str_create());
};

size_t buffer_count(void) { return g_buffer_map.item_count; }

struct buffer* buffer_handler_get(const char* name) {
    return buffer_map_get(&g_buffer_map, name);
}

struct buffer* buffer_handler_create_buffer(const char* name) {
    assert(!buffer_handler_get(name));
    buffer_map_set(&g_buffer_map, name, utf32_str_create());

    struct buffer* result = buffer_handler_get(name);
    assert(result);
    return result;
}

void buffer_handler_terminate(void) {
    buffer_map_destroy(&g_buffer_map);
}

size_t buffer_handler_count(void) { return g_buffer_map.item_count; }

void buffer_handler_list_names(size_t count,
                         struct utf32_str names[count]) {
    assert(count <= g_buffer_map.item_count);

    size_t name_idx = 0;
    for (size_t i = 0; i < BUFFER_MAP_SIZE; i += 1) {
        struct buffer_map_entry* entry = &g_buffer_map.entries[i];
        if (!entry->pair.buffer_name[0]) continue;

        utf32_str_copy_utf8(&names[name_idx++],
                            entry->pair.buffer_name,
                            strlen(entry->pair.buffer_name));

        while (entry->next) {
            entry = entry->next;
            utf32_str_copy_utf8(&names[name_idx++],
                                entry->pair.buffer_name,
                                strlen(entry->pair.buffer_name));
        }
    }
}
