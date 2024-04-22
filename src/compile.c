#include "compile.h"

#include <math.h>
#include <stdbool.h>
#include <subprocess.h>
#include <threads.h>

#include "buffer/buffer.h"
#include "commands.h"
#include "dyn_strings/utf32_string.h"
#include "error_link.h"
#include "text_view.h"

#define CMD_OUTPUT_CHUNK_CAP 264

error_links_t g_error_links;
text_view_t g_compile_view = {0};
char* g_spawn_command[4] = {"/usr/bin/bash", "-c", 0, 0};
struct subprocess_s g_command_child_process;
bool g_should_check_child_exit_code = 0;
size_t g_sel_err_link = 0;

static bool is_digit(int code) { return code >= 48 && code <= 57; }

static void find_gcc_errors(error_links_t* m, const c32_t* str,
                            size_t str_len) {
    static const c32_t* err_label = L"error:";
    static const size_t err_label_len = 6;

    error_link_t error_link = {0};

    c32_t* match_ptr = 0;
    size_t match_pos = 0;
    while ((match_ptr =
                str32str32(err_label, err_label_len, &str[match_pos],
                           str_len - match_pos))) {
        // pointer is at the error label, bring it to the column colon
        match_pos = match_ptr - &str[0] + 1;
        while (*match_ptr != L':') match_ptr -= 1;
        c32_t* link_end = match_ptr;

        // now it's in column colon, exteract column
        assert(*match_ptr == L':');
        match_ptr -= 1;
        assert(is_digit(*match_ptr));
        size_t column_decimal_place = 0;
        size_t column_number = 0;
        while (is_digit(*match_ptr)) {
            int digit = *match_ptr - L'0';
            column_number += digit * powl(10, column_decimal_place);
            match_ptr -= 1;
            column_decimal_place += 1;
        }
        error_link.file_link.column = column_number;

        // now it's in the line number colon, extract line
        assert(*match_ptr == L':');
        match_ptr -= 1;
        assert(is_digit(*match_ptr));
        size_t line_decimal_place = 0;
        size_t line_number = 0;
        while (is_digit(*match_ptr)) {
            int digit = *match_ptr - L'0';
            line_number += digit * powl(10, line_decimal_place);
            line_decimal_place += 1;
            match_ptr -= 1;
        }
        error_link.file_link.row = line_number;

        // now it's in the path colon, extract path
        assert(*match_ptr == L':');
        c32_t* path_end = match_ptr;
        while (match_ptr - 1 != str && *(match_ptr - 1) != '\n')
            match_ptr -= 1;
        c32_t* path_beg = match_ptr;
        size_t path_len = path_end - path_beg;
        error_link.file_link.path = calloc(sizeof(c32_t), path_len);
        memcpy(error_link.file_link.path, path_beg,
               path_len * sizeof(c32_t));
        error_link.file_link.path_len = path_len;

        // find link selection
        size_t error_line = 0;
        size_t error_from_column = 0;
        size_t error_to_column = link_end - path_beg;
        size_t substr_beg_idx = match_ptr - str;
        for (size_t i = substr_beg_idx; i-- > 0;) {
            if (str[i] != '\n') continue;
            error_line += 1;
        }
        error_link.link_selection.from_line = error_line;
        error_link.link_selection.to_line = error_line;
        error_link.link_selection.from_col = error_from_column;
        error_link.link_selection.to_col = error_to_column;

        // push error link
        error_links_push(m, error_link);
    }
}

void compile_set_cmd(char* str, size_t str_len) {
    g_spawn_command[2] = calloc(1, str_len + 1);
    memcpy(g_spawn_command[2], str, str_len);
}

void compile_init() {
    g_compile_view = text_view_create();
    g_compile_view.buffer = malloc(sizeof(buffer_t));
    buffer_create(g_compile_view.buffer, utf32_str_create());
    error_links_create(&g_error_links);
}

void compile_terminate() {
    if (g_spawn_command[2]) free(g_spawn_command[2]);
    buffer_destroy(g_compile_view.buffer);
    free(g_compile_view.buffer);
    text_view_destroy(&g_compile_view);
    error_links_destroy(&g_error_links);
}

static void append_proccess_finished_msg(int return_code) {
    char* exit_str = "\nProcess finished with exit code: ";
    size_t exit_str_len = strlen(exit_str);
    buffer_append_utf8(g_compile_view.buffer, exit_str, exit_str_len);

    char exit_code_str[16];
    memset(exit_code_str, 0, 16);
    snprintf(exit_code_str, 16, "%d.", return_code);
    buffer_append_utf8(g_compile_view.buffer, exit_code_str,
                       strlen(exit_code_str));
}

static void compile_update() {
    if (!subprocess_alive(&g_command_child_process)) {
        if (g_should_check_child_exit_code) {
            int return_status = 0;
            subprocess_join(&g_command_child_process, &return_status);
            append_proccess_finished_msg(return_status);
            g_should_check_child_exit_code = 0;

            find_gcc_errors(&g_error_links,
                            g_compile_view.buffer->str.data,
                            g_compile_view.buffer->str.length);
        }
        return;
    }

    char chunk[512];
    memset(chunk, 0, 512);
    subprocess_read_stdout(&g_command_child_process, chunk, 511);
    size_t chunk_len = strlen(chunk);
    if (chunk_len)
        buffer_append_utf8(g_compile_view.buffer, chunk, chunk_len);
}

void compile_spawn() {
    error_links_clear(&g_error_links);
    if (subprocess_alive(&g_command_child_process)) compile_kill();
    buffer_clear(g_compile_view.buffer);

    static const int options =
        subprocess_option_enable_async |
        subprocess_option_combined_stdout_stderr |
        subprocess_option_inherit_environment;
    subprocess_create((const char* const*)g_spawn_command, options,
                      &g_command_child_process);
    g_should_check_child_exit_code = 1;
};

void compile_kill() {
    subprocess_terminate(&g_command_child_process);
}

void compile_draw(ff_typo_t typo, Rectangle bounds, int focus) {
    compile_update();
    text_view_draw(&g_compile_view, typo, bounds, focus, 0, 0,
                   g_error_links.data, g_error_links.length);
}

void compile_jmp_next_err() {
    if (!g_error_links.length) return;
    cmd_arg_set(command_group_main, main_cmd_open_file_link,
                &g_error_links.data[g_sel_err_link].file_link,
                sizeof(file_link_t));

    if (g_sel_err_link >= g_error_links.length - 1) return;
    g_sel_err_link += 1;
}

void compile_jmp_prev_err() {
    if (!g_error_links.length) return;
    cmd_arg_set(command_group_main, main_cmd_open_file_link,
                &g_error_links.data[g_sel_err_link].file_link,
                sizeof(file_link_t));

    if (!g_sel_err_link) return;
    g_sel_err_link -= 1;
}
