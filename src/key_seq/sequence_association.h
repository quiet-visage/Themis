#pragma once
#include <stddef.h>

#define SEQUENCE_ASSOCIATION_CHILD_LIMIT 64

typedef struct {
    int mod_combo;
    int key;
} key_combination_t;

typedef struct sequence_association {
    key_combination_t combination;
    int command;
    struct sequence_association*
        children[SEQUENCE_ASSOCIATION_CHILD_LIMIT];
    size_t children_count;
} sequence_association_t;

sequence_association_t* sequence_association_create(void);
void sequence_association_destroy(sequence_association_t* m);
void sequence_association_push(sequence_association_t* m,
                               sequence_association_t* seq);
