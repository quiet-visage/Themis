#include "error_link.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void error_links_create(struct error_links* m) {
    memset(m, 0, sizeof(*m));
    m->capacity = sizeof(m->data[0]) * 2;
    m->data = malloc(m->capacity);
}

void error_links_push(struct error_links* m,
                      struct error_link error_link) {
    size_t req_cap = (m->length + 1) * sizeof(struct error_link);
    while (req_cap > m->capacity) {
        m->capacity *= 2;
        m->data = realloc(m->data, m->capacity);
        assert(m->data);
    }
    m->data[m->length++] = error_link;
}

void error_links_clear(struct error_links* m) {
    for (size_t i = 0; i < m->length; i += 1) {
        if (m->data[i].file_link.path)
            free(m->data[i].file_link.path);
    }
    m->length = 0;
}

void error_links_destroy(struct error_links* m) {
    error_links_clear(m);
    free(m->data);
}
