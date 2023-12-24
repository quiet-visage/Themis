#include "highlighter.h"

#include <assert.h>
#include <stdio.h>
#include <resources/resources.h>
#include <string.h>

#include "tree_sitter/api.h"
#include "tree_sitter/parser.h"

typedef struct {
    const char *key;
    token_kind_t value;
} ht_pair_t;

typedef struct {
    ht_pair_t *entries;
} ht_t;

static size_t cooked_hash(const char *str, size_t str_len) {
    static const size_t max_len = 26;
    size_t len = max_len < str_len ? max_len : str_len;
    size_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i += 1) {
        hash *= 0x100000001b3;
        hash ^= str[i];
    }
    return hash;
}
static const size_t ht_table_size = 5096;
static ht_t ht_create() {
    ht_t hashtable = {0};
    hashtable.entries = calloc(ht_table_size, sizeof(ht_pair_t));
    return hashtable;
}

static void ht_destroy(ht_t *ht) {
    free(ht->entries);
    ht->entries = 0;
}

static void ht_set(ht_t *ht, const char *key, token_kind_t value) {
    unsigned int slot = cooked_hash(key, strlen(key)) % ht_table_size;
    assert(ht->entries[slot].value == 0 && "collision happened");
    ht->entries[slot] = (ht_pair_t){.key = key, .value = value};
}

static token_kind_t ht_get_value(ht_t *ht, const char *key,
                                 size_t key_len) {
    unsigned int slot = cooked_hash(key, key_len) % ht_table_size;
    ht_pair_t result = ht->entries[slot];
    assert(result.key != 0 &&
           "token string does not have equivalent enum");
    return result.value;
}

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
static ht_t g_token_enum_map = {0};

static void load_query(language_t lang) {
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
        g_querie_sources[i].source = resource_file_load(g_query_file[i]);
        g_querie_sources[i].size= strlen(g_querie_sources[i].source);
        load_query(i);
        resource_file_free(g_querie_sources[i].source);
    }

    g_parser = ts_parser_new();

    g_token_enum_map = ht_create();
    ht_set(&g_token_enum_map, "keyword", token_kind_keyword_t);
    ht_set(&g_token_enum_map, "function", token_kind_function_t);
    ht_set(&g_token_enum_map, "string", token_kind_string_t);
    ht_set(&g_token_enum_map, "number", token_kind_number_t);
    ht_set(&g_token_enum_map, "operator", token_kind_operator_t);
    ht_set(&g_token_enum_map, "type", token_kind_type_t);
    ht_set(&g_token_enum_map, "constant", token_kind_constant_t);
    ht_set(&g_token_enum_map, "constant.numeric",
           token_kind_constant_numeric_t);
    ht_set(&g_token_enum_map, "variable", token_kind_variable_t);
    ht_set(&g_token_enum_map, "variable.parameter",
           token_kind_variable_parameter_t);
    ht_set(&g_token_enum_map, "variable.other.member",
           token_kind_variable_other_member_t);
    ht_set(&g_token_enum_map, "delimiter", token_kind_delimiter_t);
    ht_set(&g_token_enum_map, "property", token_kind_property_t);
    ht_set(&g_token_enum_map, "comment", token_kind_comment_t);
    ht_set(&g_token_enum_map, "keyword.directive",
           token_kind_keyword_directive_t);
    ht_set(&g_token_enum_map, "keyword.control.return",
           token_keyword_control_return_t);
    ht_set(&g_token_enum_map, "keyword.control.conditional",
           token_keyword_control_conditional_t);
    ht_set(&g_token_enum_map, "keyword.control.repeat",
           token_keyword_control_repeat_t);
    ht_set(&g_token_enum_map, "keyword.storage.type",
           token_keyword_storage_type_t);
    ht_set(&g_token_enum_map, "keyword.storage.modifier",
           token_keyword_storage_modifier_t);
    ht_set(&g_token_enum_map, "keyword.control",
           token_keyword_control_t);
    ht_set(&g_token_enum_map, "type.builtin", token_type_builtin_t);
    ht_set(&g_token_enum_map, "punctuation", token_punctuation_t);
    ht_set(&g_token_enum_map, "punctuation.delimiter",
           token_punctuation_delimiter_t);
    ht_set(&g_token_enum_map, "punctuation.bracket",
           token_punctuation_bracket_t);
    ht_set(&g_token_enum_map, "constant.builtin.boolean",
           token_constant_builtin_boolean_t);
    ht_set(&g_token_enum_map, "type.enum.variant",
           token_type_enum_variant_t);
    ht_set(&g_token_enum_map, "constant.character",
           token_constant_character_t);
    ht_set(&g_token_enum_map, "constant.character.escape",
           token_constant_character_escape_t);
    ht_set(&g_token_enum_map, "label", token_label_t);
}

void hlr_terminate() {
    for (ulong i = 0; i < language_count_t; i += 1) {
        ts_query_delete(g_queries[i]);
    }
    ts_parser_delete(g_parser);
    ht_destroy(&g_token_enum_map);
}

tokens_t hlr_tokens_create() {
    tokens_t tokens = {
        .data = (token_t *)calloc(2, sizeof(token_t)),
        .size = 0,
        .capacity = sizeof(token_t) * 2,
    };
    return tokens;
}

static void hlr_tokens_reset(tokens_t *tokens) { tokens->size = 0; }

static void tokens_push(tokens_t *o, token_t token) {
    ulong new_size = sizeof(token_t) * (o->size + 1);

    if (new_size > o->capacity) {
        o->capacity *= 2;
        o->data = (token_t *)realloc(o->data, o->capacity);
        assert(o->data != NULL);
    }

    o->data[o->size++] = token;
};

highlighter_t hlr_highlighter_create(language_t lang, const char *buffer,
                                  size_t buffer_size) {
    if (buffer_size == 0)
        return (highlighter_t){.tree = 0, .language = lang};
    ts_parser_set_language(g_parser, g_languages[lang]);
    return (highlighter_t){.tree = ts_parser_parse_string(
                               g_parser, NULL, buffer, buffer_size),
                           .language = lang};
}

void hlr_highlighter_destroy(highlighter_t *hlr) {
    ts_tree_delete(hlr->tree);
}

void hlr_highlighter_update(highlighter_t *hlr, const char *buffer,
                            size_t buffer_size) {
    ts_parser_set_language(g_parser, g_languages[hlr->language]);
    hlr->tree =
        ts_parser_parse_string(g_parser, NULL, buffer, buffer_size);
}

void hlr_tokens_update(highlighter_t *hlr, tokens_t *ts) {
    assert(hlr->language != language_none_t);
    assert(hlr->tree != NULL);
    hlr_tokens_reset(ts);
    TSNode root = ts_tree_root_node(hlr->tree);
    assert(!ts_node_is_null(root));

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, g_queries[hlr->language], root);

    TSQueryMatch match = {0};
    while (ts_query_cursor_next_match(cursor, &match)) {
        TSPoint start = ts_node_start_point(match.captures->node);
        TSPoint end = ts_node_end_point(match.captures->node);
        unsigned name_length = 0;
        const char *name = ts_query_capture_name_for_id(
            g_queries[hlr->language], match.captures->index,
            &name_length);

        token_t token = {
            .kind =
                ht_get_value(&g_token_enum_map, name, name_length),
            .position = {
                .start = {.row = start.row, .column = start.column},
                .end = {.row = end.row, .column = end.column}}};
        tokens_push(ts, token);
    }

    ts_query_cursor_delete(cursor);
}

void hlr_tokens_destroy(tokens_t *tokens) {
    free(tokens->data);
    tokens->data = 0;
    tokens->size = 0;
    tokens->capacity = 0;
}
