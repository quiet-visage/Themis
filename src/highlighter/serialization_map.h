#pragma once
#include <stddef.h>

typedef struct {
    const char *key;
    int value;
} serialization_map_pair_t;

typedef struct serialization_map_entry {
    serialization_map_pair_t pair;
    struct serialization_map_entry *next;
} serialization_map_entry_t;

typedef struct {
    serialization_map_entry_t *entries;
} serialization_map_t;

serialization_map_t serialization_map_create(void);
void serialization_map_set(serialization_map_t *o, const char *key,
                           int value);
int serialization_map_get(serialization_map_t *o, const char *key,
                          size_t key_len);
void serialization_map_destroy(serialization_map_t *o);
