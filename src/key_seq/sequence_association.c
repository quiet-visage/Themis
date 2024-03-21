#include "sequence_association.h"
#include <assert.h>
#include <stdlib.h>

struct sequence_association* sequence_association_create(void) {
    struct sequence_association* result =
        calloc(1, sizeof(struct sequence_association));
    assert(result);
    return result;
}

void sequence_association_destroy(struct sequence_association* m) {
    for (size_t i = 0; i < SEQUENCE_ASSOCIATION_CHILD_LIMIT; i += 1) {
        if (!m->children[i]) continue;
        sequence_association_destroy(m->children[i]);
    }
    free(m);
}

void sequence_association_push(struct sequence_association* m,
                               struct sequence_association* seq) {
    assert(m->children_count < SEQUENCE_ASSOCIATION_CHILD_LIMIT);
    m->children[m->children_count++] = seq;
}
