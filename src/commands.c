#include "commands.h"

#include <stdlib.h>
#include <string.h>

static cmd_arg_t g_cmd_args[] = {
    [command_group_editor] = {-1, 0},
    [command_group_line_editor] = {-1, 0},
    [command_group_pane] = {-1, 0},
    [command_group_main] = {-1, 0},
    [command_group_menu] = {-1, 0},
};

void cmd_arg_set(enum command_group group, int cmd, void* arg,
                 size_t arg_size) {
    g_cmd_args[group].cmd = cmd;
    g_cmd_args[group].arg = malloc(arg_size);
    memcpy(g_cmd_args[group].arg, arg, arg_size);
}

cmd_arg_t cmd_arg_get(enum command_group group) {
    cmd_arg_t res = g_cmd_args[group];
    memset(&g_cmd_args[group], 0, sizeof(*g_cmd_args));
    return res;
}

void cmd_arg_destroy(cmd_arg_t* m) {
    if (m->arg) free(m->arg);
}
