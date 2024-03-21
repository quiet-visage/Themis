#include "highlighter.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../resources/resources.h"
#include "serialization_map.h"
#include "tree_sitter/api.h"
#include "tree_sitter/parser.h"

typedef struct {
    char *source;
    ulong size;
} query_source_t;

TSLanguage *tree_sitter_json();
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_c();

static const char *g_query_file[] = {
    [language_c_t] = "queries/c/highlights.scm",
    [language_cpp_t] = "queries/cpp/highlights.scm",
    [language_json_t] = "queries/json/highlights.scm",
};
static TSLanguage *g_languages[language_count_t] = {0};
static query_source_t g_querie_sources[language_count_t] = {0};
static TSQuery *g_queries[language_count_t] = {0};
static TSParser *g_parser = {0};
static struct serialization_map g_token_map = {0};
static struct serialization_map g_extension_map = {0};

static void load_query(enum language lang) {
    TSQueryError error;
    g_queries[lang] =
        ts_query_new(g_languages[lang], g_querie_sources[lang].source,
                     g_querie_sources[lang].size, 0, &error);
    assert(error == TSQueryErrorNone);
}

void hlr_init() {
    g_languages[language_c_t] = tree_sitter_c();
    g_languages[language_cpp_t] = tree_sitter_cpp();
    g_languages[language_json_t] = tree_sitter_json();

    for (ulong i = 0; i < language_count_t; i += 1) {
        g_querie_sources[i].source =
            resource_file_load(g_query_file[i]);
        g_querie_sources[i].size = strlen(g_querie_sources[i].source);
        load_query(i);
        resource_file_free(g_querie_sources[i].source);
    }

    g_parser = ts_parser_new();
    g_token_map = serialization_map_create();

    // clang-format off
    serialization_map_set(&g_token_map, "keyword", token_kind_keyword_t);
    serialization_map_set(&g_token_map, "function", token_kind_function_t);
    serialization_map_set(&g_token_map, "string", token_kind_string_t);
    serialization_map_set(&g_token_map, "number", token_kind_number_t);
    serialization_map_set(&g_token_map, "operator", token_kind_operator_t);
    serialization_map_set(&g_token_map, "type", token_kind_type_t);
    serialization_map_set(&g_token_map, "constant", token_kind_constant_t);
    serialization_map_set(&g_token_map, "constant.numeric", token_kind_constant_numeric_t);
    serialization_map_set(&g_token_map, "variable", token_kind_variable_t);
    serialization_map_set(&g_token_map, "variable.parameter", token_kind_variable_parameter_t);
    serialization_map_set(&g_token_map, "variable.other.member", token_kind_variable_other_member_t);
    serialization_map_set(&g_token_map, "delimiter", token_kind_delimiter_t);
    serialization_map_set(&g_token_map, "property", token_kind_property_t);
    serialization_map_set(&g_token_map, "comment", token_kind_comment_t);
    serialization_map_set(&g_token_map, "keyword.directive", token_kind_keyword_directive_t);
    serialization_map_set(&g_token_map, "keyword.control.return", token_keyword_control_return_t);
    serialization_map_set(&g_token_map, "keyword.control.conditional", token_keyword_control_conditional_t);
    serialization_map_set(&g_token_map, "keyword.control.repeat", token_keyword_control_repeat_t);
    serialization_map_set(&g_token_map, "keyword.storage.type", token_keyword_storage_type_t);
    serialization_map_set(&g_token_map, "keyword.storage.modifier", token_keyword_storage_modifier_t);
    serialization_map_set(&g_token_map, "keyword.control", token_keyword_control_t);
    serialization_map_set(&g_token_map, "type.builtin", token_type_builtin_t);
    serialization_map_set(&g_token_map, "punctuation", token_punctuation_t);
    serialization_map_set(&g_token_map, "punctuation.delimiter", token_punctuation_delimiter_t);
    serialization_map_set(&g_token_map, "punctuation.bracket", token_punctuation_bracket_t);
    serialization_map_set(&g_token_map, "constant.builtin.boolean", token_constant_builtin_boolean_t);
    serialization_map_set(&g_token_map, "type.enum.variant", token_type_enum_variant_t);
    serialization_map_set(&g_token_map, "constant.character", token_constant_character_t);
    serialization_map_set(&g_token_map, "constant.character.escape", token_constant_character_escape_t);
    serialization_map_set(&g_token_map, "label", token_label_t);

    g_extension_map = serialization_map_create();
    serialization_map_set(&g_extension_map, ".c", language_c_t);
    serialization_map_set(&g_extension_map, ".h", language_c_t);
    serialization_map_set(&g_extension_map, ".cpp", language_cpp_t);
    serialization_map_set(&g_extension_map, ".json", language_json_t);
    // clang-format on
}

void hlr_terminate() {
    for (ulong i = 0; i < language_count_t; i += 1) {
        ts_query_delete(g_queries[i]);
    }
    ts_parser_delete(g_parser);
    serialization_map_destroy(&g_token_map);
    serialization_map_destroy(&g_extension_map);
}

struct tokens hlr_tokens_create() {
    struct tokens tokens = {
        .data = (struct token *)calloc(2, sizeof(struct token)),
        .length = 0,
        .capacity = sizeof(struct token) * 2,
    };
    return tokens;
}

static void hlr_tokens_reset(struct tokens *tokens) {
    tokens->length = 0;
}

static void tokens_push(struct tokens *o, struct token token) {
    ulong new_size = sizeof(struct token) * (o->length + 1);

    if (new_size > o->capacity) {
        o->capacity *= 2;
        o->data = (struct token *)realloc(o->data, o->capacity);
        assert(o->data != NULL);
    }

    o->data[o->length++] = token;
};

struct highlighter hlr_highlighter_create(enum language lang,
                                          const char *buffer,
                                          size_t buffer_size) {
    if (buffer_size == 0)
        return (struct highlighter){.tree = 0, .language = lang};
    ts_parser_set_language(g_parser, g_languages[lang]);
    return (struct highlighter){
        .tree = ts_parser_parse_string(g_parser, NULL, buffer,
                                       buffer_size),
        .language = lang};
}

void hlr_highlighter_destroy(struct highlighter *m) {
    ts_tree_delete(m->tree);
}

void hlr_highlighter_update(struct highlighter *m,
                            const char *buffer, size_t buffer_size) {
    ts_parser_set_language(g_parser, g_languages[m->language]);
    if (!m->tree) {
        m->tree = ts_parser_parse_string(g_parser, NULL, buffer,
                                           buffer_size);
    } else {
        TSTree *tmp = ts_parser_parse_string(g_parser, NULL, buffer,
                                             buffer_size);
        ts_tree_delete(m->tree);
        m->tree = tmp;
    }
}

void hlr_tokens_update(struct highlighter *m, struct tokens *ts) {
    assert(m->language != language_none_t);
    assert(m->tree != NULL);
    hlr_tokens_reset(ts);
    TSNode root = ts_tree_root_node(m->tree);
    assert(!ts_node_is_null(root));

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, g_queries[m->language], root);

    TSQueryMatch match = {0};
    while (ts_query_cursor_next_match(cursor, &match)) {
        TSPoint start = ts_node_start_point(match.captures->node);
        TSPoint end = ts_node_end_point(match.captures->node);
        unsigned name_length = 0;
        const char *name = ts_query_capture_name_for_id(
            g_queries[m->language], match.captures->index,
            &name_length);

        struct token token = {
            .kind = serialization_map_get(&g_token_map, name,
                                          name_length),
            .position = {
                .start = {.row = start.row, .column = start.column},
                .end = {.row = end.row, .column = end.column}}};
        tokens_push(ts, token);
    }

    ts_query_cursor_delete(cursor);
}

enum language hlr_get_extension_language(const char *dot_ext) {
    return serialization_map_get(&g_extension_map, dot_ext,
                                 strlen(dot_ext));
}

void hlr_tokens_destroy(struct tokens *m) {
    free(m->data);
    m->data = 0;
    m->length = 0;
    m->capacity = 0;
}
