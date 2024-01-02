#include <config/config.h>

static const int rosewater = 0xf5e0dcff;
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
static const int sapphire = 0x74c7ecff;
static const int blue = 0x89b4faff;
static const int lavender = 0xb4befeff;
static const int text = 0xcdd6f4ff;
static const int overlay2 = 0x9399b2ff;
static const int overlay0 = 0x6c7086ff;

struct color_scheme g_color_scheme = {
    .bg = 0x1e1e2eff,
    .fg = text,
    .text_sel_bg = 0x585b70ff,
    .selected_fg = 0xcba6f7ff,
    .selected_bg = 0x6c7086ff,
    .surface0_bg = 0x313244ff,
    .surface1_bg = 0x45475aff,
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
               [token_label_t] = mauve}};

struct layout g_layout = {
    .text_spacing = 6.0f, .padding = 8.0f, .gap = 6.0f};
