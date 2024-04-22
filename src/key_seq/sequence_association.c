#include "sequence_association.h"

#include <assert.h>
#include <stdlib.h>

sequence_association_t* sequence_association_create(void) {
    sequence_association_t* result =
        calloc(1, sizeof(sequence_association_t));
    assert(result);
    return result;
}

void sequence_association_destroy(sequence_association_t* m) {
    for (size_t i = 0; i < SEQUENCE_ASSOCIATION_CHILD_LIMIT; i += 1) {
        if (!m->children[i]) continue;
        sequence_association_destroy(m->children[i]);
    }
    free(m);
}

void sequence_association_push(sequence_association_t* m,
                               sequence_association_t* seq) {
    assert(m->children_count < SEQUENCE_ASSOCIATION_CHILD_LIMIT);
    m->children[m->children_count++] = seq;
}
