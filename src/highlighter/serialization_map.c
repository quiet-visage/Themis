#include <assert.h>
#include <highlighter/serialization_map.h>
#include <stdlib.h>
#include <string.h>

#define SERIALIZATION_MAP_SIZE 0x400

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

struct serialization_map_entry* serialization_map_ll_tail(
    struct serialization_map_entry* o) {
    assert(o != NULL);
    struct serialization_map_entry* result = o;
    while (result->next) result = result->next;
    return result;
}

void serialization_map_set(struct serialization_map* o,
                           const char* key, int value) {
    size_t slot =
        cooked_hash(key, strlen(key)) % SERIALIZATION_MAP_SIZE;

    struct serialization_map_entry* entry = &o->entries[slot];

    if (entry->pair.key == NULL) {
        entry->pair.key = key;
        entry->pair.value = value;
        return;
    } else if (strcmp(entry->pair.key, key) == 0) {
        entry->pair.value = value;
        return;
    }

    while (entry->next) {
        entry = entry->next;

        if (entry && strcmp(entry->pair.key, key) == 0) {
            entry->pair.value = value;
            return;
        }
    }

    entry->next = calloc(sizeof(struct serialization_map_entry), 1);
    assert(entry->next);
    entry->next->pair.key = key;
    entry->next->pair.value = value;
}

int serialization_map_get(struct serialization_map* o,
                          const char* key, size_t key_len) {
    size_t slot = cooked_hash(key, key_len) % SERIALIZATION_MAP_SIZE;

    struct serialization_map_entry* entry = &o->entries[slot];
    while (entry) {
        if (entry->pair.key && strcmp(entry->pair.key, key) == 0)
            return entry->pair.value;
        entry = entry->next;
    }

    return -1;
}

struct serialization_map serialization_map_create(void) {
    struct serialization_map result = {
        .entries = calloc(sizeof(struct serialization_map_entry),
                          SERIALIZATION_MAP_SIZE)};
    assert(result.entries);

    return result;
}

static void serialization_map_entry_destroy(
    struct serialization_map_entry* o) {
    if (o->next) serialization_map_entry_destroy(o->next);
    o->pair.key = 0;
    o->pair.value = 0;
    if (o->next) {
        free(o->next);
        o->next = NULL;
    }
}

void serialization_map_destroy(struct serialization_map* o) {
    for (size_t i = 0; i < SERIALIZATION_MAP_SIZE; i += 1) {
        if (o->entries[i].pair.key)
            serialization_map_entry_destroy(&o->entries[i]);
    }
    free(o->entries);
}
