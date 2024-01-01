#pragma once
#include <stddef.h>

struct serialization_map_pair {
    const char *key;
    int value;
};

struct serialization_map_entry {
    struct serialization_map_pair pair;
    struct serialization_map_entry *next;
};

struct serialization_map {
    struct serialization_map_entry *entries;
};

struct serialization_map serialization_map_create(void);
void serialization_map_set(struct serialization_map *o,
                           const char *key, int value);
int serialization_map_get(struct serialization_map *o,
                          const char *key, size_t key_len);
void serialization_map_destroy(struct serialization_map *o);
