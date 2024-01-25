#pragma once
#include "../dyn_strings/utf32_string.h"
#include "../highlighter/highlighter.h"

struct editor_history {
    struct utf32_str text_buffer;
    struct text_position cursor;
};

struct editor_history_stack {
    struct editor_history* data;
    ulong size;
    ulong capacity;
};

struct editor_history editor_history_create(
    struct utf32_str* buffer, struct text_position cursor);
void editor_history_destroy(struct editor_history*);

struct editor_history_stack editor_history_stack_create(void);
struct editor_history* editor_history_stack_top(
    struct editor_history_stack* this);
void editor_history_stack_destroy(struct editor_history_stack* this);
void editor_history_stack_push(struct editor_history_stack* this,
                               struct editor_history history);
void editor_history_stack_pop(struct editor_history_stack* this);
