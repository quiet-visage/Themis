#pragma once

#include <stdbool.h>
#define KEY_SEQUENCE_LIMIT 3

#include "sequence_association.h"

typedef size_t seq_group_id_t;

enum mod_keys {
    mod_key_shift = (1 << 0),
    mod_key_ctrl = (1 << 1),
    mod_key_alt = (1 << 2),
    mod_key_super = (1 << 3)
};

void key_seq_handler_init(void);
void key_seq_handler_terminate(void);
void key_seq_handler_begin_frame(void);
void key_seq_handler_end_frame(void);

seq_group_id_t key_seq_handler_create_group(void);
int key_seq_handler_get_command(seq_group_id_t seq_group_id);
void key_seq_handler_register_association(
    seq_group_id_t seq_group_id, size_t sequence_len,
    struct key_combination sequence[sequence_len],
    int associated_command);
bool key_seq_handler_key_seq_active(void);
