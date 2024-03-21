#pragma once

#include "tree_sitter/api.h"

typedef unsigned long ulong;

struct text_position {
    ulong row;
    ulong column;
};

struct token_position {
    struct text_position start;
    struct text_position end;
};

enum token_kind {
    token_kind_unknown_t = -1,
    token_kind_keyword_t,
    token_kind_function_t,
    token_kind_string_t,
    token_kind_number_t,
    token_kind_operator_t,
    token_kind_type_t,
    token_kind_constant_t,
    token_kind_constant_numeric_t,
    token_kind_variable_t,
    token_kind_variable_parameter_t,
    token_kind_variable_other_member_t,
    token_kind_delimiter_t,
    token_kind_property_t,
    token_kind_comment_t,
    token_kind_keyword_directive_t,
    token_keyword_control_return_t,
    token_keyword_control_conditional_t,
    token_keyword_control_repeat_t,
    token_keyword_storage_type_t,
    token_keyword_storage_modifier_t,
    token_keyword_control_t,
    token_type_builtin_t,
    token_punctuation_t,
    token_punctuation_delimiter_t,
    token_punctuation_bracket_t,
    token_constant_builtin_boolean_t,
    token_type_enum_variant_t,
    token_constant_character_t,
    token_constant_character_escape_t,
    token_label_t,
    token_kind_count_t
};

struct token {
    enum token_kind kind;
    struct token_position position;
};

struct tokens {
    struct token* data;
    ulong length;
    ulong capacity;
};

enum language {
    language_none_t = -1,
    language_c_t = 0,
    language_cpp_t,
    language_json_t,
    language_count_t
};

struct highlighter {
    TSTree* tree;
    enum language language;
};

void hlr_init();
void hlr_terminate();
struct highlighter hlr_highlighter_create(enum language lang,
                                          const char* buffer,
                                          size_t buffer_size);
void hlr_highlighter_destroy(struct highlighter* m);
void hlr_highlighter_update(struct highlighter* m, const char* buffer,
                            size_t buffer_size);
enum language hlr_get_extension_language(const char* dot_ext);
struct tokens hlr_tokens_create();
void hlr_tokens_destroy(struct tokens* m);
void hlr_tokens_update(struct highlighter* m, struct tokens* ts);
