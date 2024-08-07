#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dyn_strings/utf32_string.h"
#include "selection.h"

typedef struct {
    c32_t* path;
    size_t path_len;
    size_t row;
    size_t column;
} file_link_t;

typedef struct {
    selection_t link_selection;
    file_link_t file_link;
} error_link_t;

typedef struct {
    error_link_t* data;
    size_t length;
    size_t capacity;
} error_links_t;

void error_links_create(error_links_t* m);
void error_links_clear(error_links_t* m);
void error_links_push(error_links_t* m, error_link_t error_link);
void error_links_destroy(error_links_t* m);

#ifdef __cplusplus
}
#endif
