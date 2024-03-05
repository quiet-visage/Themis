#include "buffer_handler.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "dyn_strings/utf32_string.h"
#include "text/text_view.h"
#define BUFFER_MAP_SIZE 1024
#define TODO #error "todo"

struct buffer_map_entry {
    struct buffer buffer;
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

static void buffer_map_entry_destroy(struct buffer_map_entry* o) {
    if (o->next) buffer_map_entry_destroy(o->next);
    buffer_destroy(&o->buffer);
    memset(&o->buffer, 0, sizeof(struct buffer));
    if (o->next) {
        free(o->next);
        o->next = NULL;
    }
}

static void buffer_map_destroy(struct buffer_map* o) {
    for (size_t i = 0; i < BUFFER_MAP_SIZE; i += 1) {
        if (o->entries[i].buffer.buffer_name[0])
            buffer_map_entry_destroy(&o->entries[i]);
    }
}

static struct buffer* buffer_map_get(struct buffer_map* o,
                                     const char* buffer_name) {
    size_t slot = cooked_hash(buffer_name, strlen(buffer_name)) %
                  BUFFER_MAP_SIZE;

    struct buffer_map_entry* entry = &o->entries[slot];
    while (entry) {
        if (entry->buffer.buffer_name[0] &&
            !strcmp(entry->buffer.buffer_name, buffer_name))
            return &entry->buffer;
        entry = entry->next;
    }

    return 0;
}

static void buffer_map_set(struct buffer_map* o,
                           const char* buffer_name,
                           struct utf32_str string) {
    size_t slot = cooked_hash(buffer_name, strlen(buffer_name)) %
                  BUFFER_MAP_SIZE;

    struct buffer_map_entry* entry = &o->entries[slot];

    if (!entry->buffer.buffer_name[0]) {
        buffer_create(&entry->buffer, string);
        buffer_set_name(&entry->buffer, buffer_name);
        o->item_count += 1;
        return;
    } else if (!strcmp(entry->buffer.buffer_name, buffer_name)) {
        assert(0 &&
               "trying to create buffer that is already present");
    }

    while (entry->next) {
        entry = entry->next;

        if (entry &&
            !strcmp(entry->buffer.buffer_name, buffer_name)) {
            entry->buffer.str = string;
            return;
        }
    }

    entry->next = calloc(sizeof(struct buffer_map_entry), 1);
    assert(entry->next);

    buffer_create(&entry->next->buffer, string);
    buffer_set_name(&entry->next->buffer, buffer_name);
    o->item_count += 1;
}

static struct buffer_map g_buffer_map = {0};

void buffer_handler_init(void) {
    buffer_map_set(&g_buffer_map, "[scratch]", utf32_str_create());
    struct buffer* scratch_buffer =
        buffer_map_get(&g_buffer_map, "[scratch]");
    buffer_save_undo(scratch_buffer);
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
        if (!entry->buffer.buffer_name[0]) continue;

        utf32_str_copy_utf8(&names[name_idx++],
                            entry->buffer.buffer_name,
                            strlen(entry->buffer.buffer_name));

        while (entry->next) {
            entry = entry->next;
            utf32_str_copy_utf8(&names[name_idx++],
                                entry->buffer.buffer_name,
                                strlen(entry->buffer.buffer_name));
        }
    }
}
