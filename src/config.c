#include "config.h"

#include "commands.h"
#include "fieldfusion.h"
#include "raylib.h"

static const int flamingo = 0xf2cdcdff;
static const int pink = 0xf5c2e7ff;
static const int mauve = 0xcba6f7ff;
static const int red = 0xf38ba8ff;
static const int maroon = 0xeba0acff;
static const int peach = 0xfab387ff;
static const int yellow = 0xf9e2afff;
static const int green = 0xa6e3a1ff;
static const int teal = 0x94e2d5ff;
static const int sky = 0x89dcebff;
static const int blue = 0x89b4faff;
static const int text = 0xcdd6f4ff;
static const int overlay2 = 0x9399b2ff;
static const int overlay0 = 0x6c7086ff;

config_t g_cfg = {
    .layout = (layout_t){.text_spacing = 6.0f,
                         .padding = 8.0f,
                         .gap = 6.0f,
                         .search_box_width = 256},
    .color_scheme =
        (color_scheme_t){
            .bg = 0x1e1e2eff,
            .fg = text,
            .text_mute = 0x7f849cff,
            .text_sel_bg = 0x585b70ff,
            .selected_fg = 0xcba6f7ff,
            .selected_bg = 0x6c7086ff,
            .surface0_bg = 0x313244ff,
            .surface1_bg = 0x45475aff,
            .highlight_fg = flamingo,
            .syntax = {[token_kind_keyword_t] = mauve,
                       [token_kind_function_t] = blue,
                       [token_kind_string_t] = green,
                       [token_kind_number_t] = peach,
                       [token_kind_operator_t] = sky,
                       [token_kind_type_t] = yellow,
                       [token_kind_constant_t] = text,
                       [token_kind_constant_numeric_t] = peach,
                       [token_kind_variable_t] = text,
                       [token_kind_variable_parameter_t] = maroon,
                       [token_kind_variable_other_member_t] = text,
                       [token_kind_delimiter_t] = overlay2,
                       [token_kind_property_t] = teal,
                       [token_kind_comment_t] = overlay0,
                       [token_kind_keyword_directive_t] = mauve,
                       [token_keyword_control_return_t] = mauve,
                       [token_keyword_control_conditional_t] = mauve,
                       [token_keyword_control_repeat_t] = mauve,
                       [token_keyword_storage_type_t] = yellow,
                       [token_keyword_storage_modifier_t] = mauve,
                       [token_keyword_control_t] = mauve,
                       [token_type_builtin_t] = red,
                       [token_punctuation_t] = overlay2,
                       [token_punctuation_delimiter_t] = overlay2,
                       [token_punctuation_bracket_t] = overlay2,
                       [token_constant_builtin_boolean_t] = peach,
                       [token_type_enum_variant_t] = yellow,
                       [token_constant_character_t] = peach,
                       [token_constant_character_escape_t] = pink,
                       [token_label_t] = mauve}},
    .scroll_off = 10};

#define KEY_SEQ(MOD, KEY) \
    (key_combination_t) { .mod_combo = MOD, .key = KEY }

#define REGISTER_KEYBIND(GROUP, COMMAND, ...)                       \
    do {                                                            \
        key_combination_t combination[] = {__VA_ARGS__};            \
        key_seq_handler_register_association(                       \
            GROUP, sizeof(combination) / sizeof(key_combination_t), \
            combination, COMMAND);                                  \
    } while (0)

static void init_editor_keybinds() {
    // clang-format off
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_char_left,        KEY_SEQ(mod_key_ctrl,                 KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_char_right,       KEY_SEQ(mod_key_ctrl,                 KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_line_up,          KEY_SEQ(mod_key_ctrl,                 KEY_P));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_line_down,        KEY_SEQ(mod_key_ctrl,                 KEY_N));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_end_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_E));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_beg_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_A));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_begin_mode_selection,  KEY_SEQ(mod_key_ctrl,                 KEY_SPACE));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_begin_mode_search,     KEY_SEQ(mod_key_ctrl,                 KEY_S));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_delete,                KEY_SEQ(mod_key_ctrl,                 KEY_D));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_backspace,             KEY_SEQ(mod_key_ctrl,                 KEY_H));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_backspace,             KEY_SEQ(0,                            KEY_BACKSPACE));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_undo,                  KEY_SEQ(mod_key_ctrl,                 KEY_SLASH));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_redo,                  KEY_SEQ(mod_key_ctrl,                 KEY_U));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_paste,                 KEY_SEQ(mod_key_ctrl,                 KEY_Y));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_delete_rest_of_line,   KEY_SEQ(mod_key_ctrl,                 KEY_K));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_word_right,       KEY_SEQ(mod_key_alt,                  KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_word_left,        KEY_SEQ(mod_key_alt,                  KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_buffer_begin,     KEY_SEQ(mod_key_alt | mod_key_shift,  KEY_COMMA));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_normal, editor_cmd_move_buffer_end,       KEY_SEQ(mod_key_alt | mod_key_shift,  KEY_PERIOD));

    // editor selection mode
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_end_of_line,         KEY_SEQ(mod_key_ctrl,                 KEY_E));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_beg_of_line,         KEY_SEQ(mod_key_ctrl,                 KEY_A));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_char_left,           KEY_SEQ(mod_key_ctrl,                 KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_char_right,          KEY_SEQ(mod_key_ctrl,                 KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_line_up,             KEY_SEQ(mod_key_ctrl,                 KEY_P));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_line_down,           KEY_SEQ(mod_key_ctrl,                 KEY_N));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_copy,                     KEY_SEQ(mod_key_alt,                  KEY_W));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_paste,                    KEY_SEQ(mod_key_ctrl,                 KEY_Y));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_cut,                      KEY_SEQ(mod_key_ctrl,                 KEY_W));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_end_mode_selection,       KEY_SEQ(0,                            KEY_ESCAPE));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_end_mode_selection,       KEY_SEQ(mod_key_ctrl,                 KEY_G));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_undo,                     KEY_SEQ(mod_key_ctrl,                 KEY_SLASH));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_redo,                     KEY_SEQ(mod_key_ctrl,                 KEY_U));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_delete,                   KEY_SEQ(mod_key_ctrl,                 KEY_D));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_backspace,                KEY_SEQ(mod_key_ctrl,                 KEY_H));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_end_mode_selection,       KEY_SEQ(mod_key_ctrl,                 KEY_G));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_word_right,          KEY_SEQ(mod_key_alt,                  KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_word_left,           KEY_SEQ(mod_key_alt,                  KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_buffer_begin,        KEY_SEQ(mod_key_alt | mod_key_shift,  KEY_COMMA));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_selection, editor_cmd_move_buffer_end,          KEY_SEQ(mod_key_alt | mod_key_shift,  KEY_PERIOD));

    // editor search mode
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_search, editor_cmd_next_search_match,           KEY_SEQ(mod_key_ctrl,                 KEY_S));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_search, editor_cmd_previous_search_match,       KEY_SEQ(mod_key_ctrl,                 KEY_R));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_search, editor_cmd_end_mode_search,             KEY_SEQ(mod_key_ctrl,                 KEY_G));
    REGISTER_KEYBIND(g_cfg.keybinds.editor.mode_search, editor_cmd_end_mode_search,             KEY_SEQ(0,                            KEY_ESCAPE));
}

static void init_line_editor_keybinds() {
    // clang-format off
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_char_left,        KEY_SEQ(mod_key_ctrl,                 KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_char_right,       KEY_SEQ(mod_key_ctrl,                 KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_end_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_E));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_beg_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_A));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_begin_mode_selection,  KEY_SEQ(mod_key_ctrl,                 KEY_SPACE));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_delete,                KEY_SEQ(mod_key_ctrl,                 KEY_D));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_backspace,             KEY_SEQ(mod_key_ctrl,                 KEY_H));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_backspace,             KEY_SEQ(0,                            KEY_BACKSPACE));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_undo,                  KEY_SEQ(mod_key_ctrl,                 KEY_SLASH));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_redo,                  KEY_SEQ(mod_key_ctrl,                 KEY_U));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_paste,                 KEY_SEQ(mod_key_ctrl,                 KEY_Y));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_delete_rest_of_line,   KEY_SEQ(mod_key_ctrl,                 KEY_K));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_word_right,       KEY_SEQ(mod_key_alt,                  KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_normal, line_editor_cmd_move_word_left,        KEY_SEQ(mod_key_alt,                  KEY_B));

    // editor selection mode
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_char_left,        KEY_SEQ(mod_key_ctrl,                 KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_char_right,       KEY_SEQ(mod_key_ctrl,                 KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_end_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_E));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_beg_of_line,      KEY_SEQ(mod_key_ctrl,                 KEY_A));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_copy,                  KEY_SEQ(mod_key_alt,                  KEY_W));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_paste,                 KEY_SEQ(mod_key_ctrl,                 KEY_Y));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_cut,                   KEY_SEQ(mod_key_ctrl,                 KEY_W));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_end_mode_selection,    KEY_SEQ(0,                            KEY_ESCAPE));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_undo,                  KEY_SEQ(mod_key_ctrl,                 KEY_SLASH));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_redo,                  KEY_SEQ(mod_key_ctrl,                 KEY_U));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_delete,                KEY_SEQ(mod_key_ctrl,                 KEY_D));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_backspace,             KEY_SEQ(mod_key_ctrl,                 KEY_H));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_word_right,       KEY_SEQ(mod_key_alt,                  KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.line_editor.mode_selection, line_editor_cmd_move_word_left,        KEY_SEQ(mod_key_alt,                  KEY_B));
}

static void init_pane_keybinds() {
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_split_vertical,   KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_TWO));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_split_horizontal, KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_THREE));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_close_pane,       KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_ZERO));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_next,       KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_L));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_prev,       KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_H));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_left,       KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_LEFT));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_right,      KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_RIGHT));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_down,       KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_DOWN));
    REGISTER_KEYBIND(g_cfg.keybinds.pane, pane_cmd_focus_up,         KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_UP));
}

static void init_main_keybinds() {
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_open_file_picker,   KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_F));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_open_buffer_picker, KEY_SEQ(mod_key_ctrl, KEY_X), KEY_SEQ(0, KEY_B));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_close,              KEY_SEQ(0, KEY_ESCAPE));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_compile,            KEY_SEQ(mod_key_ctrl, KEY_C), KEY_SEQ(0, KEY_C));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_compile_close,      KEY_SEQ(mod_key_ctrl, KEY_C), KEY_SEQ(0, KEY_X));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_compile_goto_next_error, KEY_SEQ(mod_key_ctrl, KEY_C), KEY_SEQ(0, KEY_N));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_compile_goto_prev_error, KEY_SEQ(mod_key_ctrl, KEY_C), KEY_SEQ(0, KEY_P));
    REGISTER_KEYBIND(g_cfg.keybinds.main, main_cmd_open_set_cmd_prompt, KEY_SEQ(mod_key_ctrl, KEY_C), KEY_SEQ(0, KEY_S));
}

static void init_menu_keybinds() {
    REGISTER_KEYBIND(g_cfg.keybinds.fuzzy_menu, menu_cmd_move_up,            KEY_SEQ(mod_key_ctrl, KEY_P));
    REGISTER_KEYBIND(g_cfg.keybinds.fuzzy_menu, menu_cmd_move_down,          KEY_SEQ(mod_key_ctrl, KEY_N));
}

static void init_file_picker_keybinds() {
    REGISTER_KEYBIND(g_cfg.keybinds.file_picker, file_picker_cmd_previous_dir,KEY_SEQ(mod_key_ctrl | mod_key_shift, KEY_COMMA));
}

static void init_file_editor_keybinds() {
    REGISTER_KEYBIND(g_cfg.keybinds.file_editor, file_editor_cmd_save,KEY_SEQ(mod_key_ctrl , KEY_X), KEY_SEQ(0, KEY_S));
}
// clang-format on
void config_init(void) {
    g_cfg.keybinds.editor.mode_normal =
        key_seq_handler_create_group();
    g_cfg.keybinds.editor.mode_search =
        key_seq_handler_create_group();
    g_cfg.keybinds.editor.mode_selection =
        key_seq_handler_create_group();
    g_cfg.keybinds.line_editor.mode_selection =
        key_seq_handler_create_group();
    g_cfg.keybinds.line_editor.mode_normal =
        key_seq_handler_create_group();
    g_cfg.keybinds.pane = key_seq_handler_create_group();
    g_cfg.keybinds.fuzzy_menu = key_seq_handler_create_group();
    g_cfg.keybinds.main = key_seq_handler_create_group();
    g_cfg.keybinds.file_picker = key_seq_handler_create_group();
    g_cfg.keybinds.file_editor = key_seq_handler_create_group();

    init_editor_keybinds();
    init_line_editor_keybinds();
    init_pane_keybinds();
    init_menu_keybinds();
    init_main_keybinds();
    init_file_picker_keybinds();
    init_file_editor_keybinds();

    g_cfg.typo =
        (ff_typo_t){.font = 0, .size = 14.f, .color = 0xffffffff};
}
