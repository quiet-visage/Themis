#include <highlighter/serialization_map.h>
#include <highlighter/token_map.h>

#include "serialization_map.h"

struct token_map token_map_create(void) {
    struct serialization_map = serialization_map_create();

    serialization_map_set(&result, "keyword", token_kind_keyword_t);
    serialization_map_set(&result, "function", token_kind_function_t);
    serialization_map_set(&result, "string", token_kind_string_t);
    serialization_map_set(&result, "number", token_kind_number_t);
    serialization_map_set(&result, "operator", token_kind_operator_t);
    serialization_map_set(&result, "type", token_kind_type_t);
    serialization_map_set(&result, "constant", token_kind_constant_t);
    serialization_map_set(&result, "constant.numeric",
                          token_kind_constant_numeric_t);
    serialization_map_set(&result, "variable", token_kind_variable_t);
    serialization_map_set(&result, "variable.parameter",
                          token_kind_variable_parameter_t);
    serialization_map_set(&result, "variable.other.member",
                          token_kind_variable_other_member_t);
    serialization_map_set(&result, "delimiter",
                          token_kind_delimiter_t);
    serialization_map_set(&result, "property", token_kind_property_t);
    serialization_map_set(&result, "comment", token_kind_comment_t);
    serialization_map_set(&result, "keyword.directive",
                          token_kind_keyword_directive_t);
    serialization_map_set(&result, "keyword.control.return",
                          token_keyword_control_return_t);
    serialization_map_set(&result, "keyword.control.conditional",
                          token_keyword_control_conditional_t);
    serialization_map_set(&result, "keyword.control.repeat",
                          token_keyword_control_repeat_t);
    serialization_map_set(&result, "keyword.storage.type",
                          token_keyword_storage_type_t);
    serialization_map_set(&result, "keyword.storage.modifier",
                          token_keyword_storage_modifier_t);
    serialization_map_set(&result, "keyword.control",
                          token_keyword_control_t);
    serialization_map_set(&result, "type.builtin",
                          token_type_builtin_t);
    serialization_map_set(&result, "punctuation",
                          token_punctuation_t);
    serialization_map_set(&result, "punctuation.delimiter",
                          token_punctuation_delimiter_t);
    serialization_map_set(&result, "punctuation.bracket",
                          token_punctuation_bracket_t);
    serialization_map_set(&result, "constant.builtin.boolean",
                          token_constant_builtin_boolean_t);
    serialization_map_set(&result, "type.enum.variant",
                          token_type_enum_variant_t);
    serialization_map_set(&result, "constant.character",
                          token_constant_character_t);
    serialization_map_set(&result, "constant.character.escape",
                          token_constant_character_escape_t);
    serialization_map_set(&result, "label", token_label_t);

    return result;
}

void token_map_destroy(struct token_map* o) {
    for (size_t i = 0; i < TOKEN_MAP_SIZE; i += 1) {
        if (o->entries[i].pair.key)
            token_map_entry_destroy(&o->entries[i]);
    }
    free(o->entries);
}
