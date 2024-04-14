#pragma once

#include "dyn_strings/utf32_string.h"
#include "selection.h"

struct file_link {
    c32_t* path;
    size_t path_len;
    size_t row;
    size_t column;
};

struct error_link {
    struct selection link_selection;
    struct file_link file_link;
};

struct error_links {
    struct error_link* data;
    size_t length;
    size_t capacity;
};

void error_links_create(struct error_links* m);
void error_links_clear(struct error_links* m);
void error_links_push(struct error_links* m,
                      struct error_link error_link);
void error_links_destroy(struct error_links* m);
