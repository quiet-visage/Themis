#include "key_seq.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../keyboard.h"

#define KEY_SEQ_GROUPS_LIMIT 16
#include "sequence_association.h"

struct sequence_association* g_groups[KEY_SEQ_GROUPS_LIMIT];
size_t g_groups_len = 0;
struct key_combination g_recorded_key_sequence[KEY_SEQUENCE_LIMIT];
size_t g_recorded_key_sequence_len = 0;
bool g_completed_seq_this_frame = 0;

void key_seq_handler_terminate(void) {
    for (size_t i = 0; i < g_groups_len; i += 1) {
        sequence_association_destroy(g_groups[i]);
    }
}

int get_current_mod_combo() {
    int combination = 0;
    if (is_key_down(GLFW_KEY_LEFT_CONTROL) ||
        is_key_down(GLFW_KEY_RIGHT_CONTROL))
        combination |= mod_key_ctrl;
    if (is_key_down(GLFW_KEY_LEFT_SHIFT) ||
        is_key_down(GLFW_KEY_RIGHT_SHIFT))
        combination |= mod_key_shift;
    if (is_key_down(GLFW_KEY_LEFT_ALT) ||
        is_key_down(GLFW_KEY_RIGHT_ALT))
        combination |= mod_key_alt;
    if (is_key_down(GLFW_KEY_LEFT_SUPER) ||
        is_key_down(GLFW_KEY_RIGHT_SUPER))
        combination |= mod_key_super;
    return combination;
}

bool key_combination_equal(struct key_combination a,
                           struct key_combination b) {
    return a.key == b.key && a.mod_combo == b.mod_combo;
}

struct sequence_association*
key_seq_handler_find_child_with_combination(
    struct sequence_association* m,
    struct key_combination combination) {
    if (!m->children_count) return 0;

    for (size_t i = 0; i < m->children_count; i += 1) {
        if (key_combination_equal(m->children[i]->combination,
                                  combination))
            return m->children[i];
    }

    return 0;
}

struct sequence_association*
key_seq_handler_find_captured_association(seq_group_id_t id) {
    size_t active_combination_idx = 0;
    struct sequence_association* sequence_node = g_groups[id];
    if (!sequence_node->children_count) return NULL;
    // *** TODO : dissallow single key such as enter from key seq
    // because it causes capture bugs
    while (sequence_node) {
        struct key_combination active_combination =
            g_recorded_key_sequence[active_combination_idx];

        struct sequence_association* child_with_current_combination =
            key_seq_handler_find_child_with_combination(
                sequence_node, active_combination);

        if (!child_with_current_combination) return 0;

        bool is_last_comparison =
            active_combination_idx >= g_recorded_key_sequence_len - 1;
        if (is_last_comparison) return child_with_current_combination;

        sequence_node = child_with_current_combination;
        active_combination_idx += 1;
    }

    return 0;
}

void key_seq_handler_register_association(
    seq_group_id_t seq_group_id, size_t sequence_len,
    struct key_combination sequence[sequence_len],
    int associated_command) {
    assert(seq_group_id < g_groups_len);
    struct sequence_association* parent = g_groups[seq_group_id];

    for (size_t i = 0; i < sequence_len; i += 1) {
        struct key_combination combination = sequence[i];
        struct sequence_association* child_with_association =
            key_seq_handler_find_child_with_combination(parent,
                                                        combination);

        if (child_with_association) {
            parent = child_with_association;
            continue;
        }

        struct sequence_association* new_association =
            sequence_association_create();
        sequence_association_push(parent, new_association);
        new_association->combination = sequence[i];
        if (i >= sequence_len - 1)
            new_association->command = associated_command;
        else
            parent = new_association;
    }
}

void key_seq_handler_clear_recorded_sequence(void) {
    g_recorded_key_sequence_len = 0;
}

bool is_mod_key(int key) {
    return key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_LEFT_CONTROL ||
           key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_LEFT_SUPER ||
           key == GLFW_KEY_RIGHT_ALT ||
           key == GLFW_KEY_RIGHT_CONTROL ||
           key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_RIGHT_SUPER;
}

static void key_seq_handler_capture(void) {
    struct key_combination key_combination = {
        .mod_combo = get_current_mod_combo(),
        .key = get_sticky_key(),
    };

    if (!key_combination.key || is_mod_key(key_combination.key))
        return;

    g_recorded_key_sequence[g_recorded_key_sequence_len++] =
        key_combination;
    assert(g_recorded_key_sequence_len <= 3);
}

bool key_seq_handler_is_recorded_sequence_valid(void) {
    for (size_t i = 0; i < g_groups_len; i += 1) {
        struct sequence_association* node =
            key_seq_handler_find_captured_association(i);
        if (node) return true;
    }
    return false;
}

void key_seq_handler_begin_frame(void) {
    key_seq_handler_capture();
    if (g_recorded_key_sequence_len > 0 &&
        !key_seq_handler_is_recorded_sequence_valid())
        key_seq_handler_clear_recorded_sequence();
}

void key_seq_handler_end_frame(void) {
    g_completed_seq_this_frame = 0;
}

seq_group_id_t key_seq_handler_create_group(void) {
    assert(g_groups_len + 1 <= KEY_SEQ_GROUPS_LIMIT);
    size_t id = g_groups_len;
    g_groups[id] = sequence_association_create();
    g_groups_len += 1;
    return id;
}

int key_seq_handler_get_command(seq_group_id_t seq_group_id) {
    if (!g_recorded_key_sequence_len) return -1;
    struct sequence_association* associated_node =
        key_seq_handler_find_captured_association(seq_group_id);
    if (!associated_node) return -1;
    if (associated_node->children_count > 0) return -1;
    key_seq_handler_clear_recorded_sequence();
    g_completed_seq_this_frame = 1;
    return associated_node->command;
}

bool key_seq_handler_key_seq_active(void) {
    return g_completed_seq_this_frame ||
           g_recorded_key_sequence_len > 0;
}
