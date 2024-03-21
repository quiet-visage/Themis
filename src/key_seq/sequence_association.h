#pragma once
#include <stddef.h>

#define SEQUENCE_ASSOCIATION_CHILD_LIMIT 64

struct key_combination {
    int mod_combo;
    int key;
};

struct sequence_association {
    struct key_combination combination;
    int command;
    struct sequence_association*
        children[SEQUENCE_ASSOCIATION_CHILD_LIMIT];
    size_t children_count;
};

struct sequence_association* sequence_association_create(void);
void sequence_association_destroy(struct sequence_association* m);
void sequence_association_push(struct sequence_association* m,
                               struct sequence_association* seq);
