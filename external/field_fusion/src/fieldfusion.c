#include <fieldfusion.h>
#include <iconv.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wchar.h>

#include "freetype/freetype.h"
#include "serializer.h"

// clang-format off
#define __FF_EMBED_FILE(var_name, file_name)          \
    __asm__("" #var_name ": .incbin \"" #file_name "\""); \
    __asm__("" #var_name "_end: .byte 0");                 \
    __asm__("" #var_name "_len: .int "#var_name"_end - "#var_name"")

__FF_EMBED_FILE(g_font_vertex, ff_shaders/ff_font.vert);
__FF_EMBED_FILE(g_font_fragment, ff_shaders/ff_font.frag);
__FF_EMBED_FILE(g_font_geometry, ff_shaders/ff_font.geo);
__FF_EMBED_FILE(g_msdf_vertex, ff_shaders/ff_msdf.vert);
__FF_EMBED_FILE(g_msdf_fragment, ff_shaders/ff_msdf.frag);
__FF_EMBED_FILE(g_default_font, ff_fonts/SourceCodePro-Regular.ttf);
// clang-format on

enum endian { endian_le, endian_be };

extern const char g_font_fragment[];
extern const char g_font_geometry[];
extern const char g_msdf_vertex[];
extern const char g_font_vertex[];
extern const char g_msdf_fragment[];
extern const unsigned char g_default_font[];
extern const int g_default_font_len;
enum endian g_system_endianess;

iconv_t g_utf8_to_utf32;
iconv_t g_utf32_to_utf8;

struct ff_index_entry_t {
    float offset_x;
    float offset_y;
    float size_x;
    float size_y;
    float bearing_x;
    float bearing_y;
    float glyph_width;
    float glyph_height;
};

static const GLfloat kmat4_zero_init[4][4] = {
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f}};

static bool compile_shader(const char *source, GLenum type,
                           uint *shader, const char *sl_version) {
    /* Default to versio */
    if (!sl_version) sl_version = "330 core";

    *shader = glCreateShader(type);
    if (!*shader) {
        fprintf(stderr, "failed to create shader\n");
    }

    const char *src[] = {"#version ", sl_version, "\n", source};

    glShaderSource(*shader, 4, src, NULL);
    glCompileShader(*shader);

    GLint status;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(*shader, 1000, &len, log);
        fprintf(stderr, "Error: compiling: %*s\n", len, log);
        return 0;
    }

    return 1;
}

static void gen_extended_ascii(const ff_font_id_t font_handle) {
    c32_t codepoints[0xff];
    for (ulong i = 0; i < 0xff; i += 1) {
        codepoints[i] = i;
    }
    ff_gen_glyphs(font_handle, codepoints, 0xff);
}

struct ff_uniforms {
    int window_projection;
    int font_atlas_projection;
    int index;
    int atlas;
    int padding;
    int offset;
    int dpi;
    int units_per_em;
    int atlas_projection;
    int texture_offset;
    int translate;
    int scale;
    int range;
    int glyph_height;
    int meta_offset;
    int point_offset;
    int metadata;
    int point_data;
};

static FT_Library g_ft_library;
static float g_dpi[2];
static uint g_gen_shader;
static uint g_render_shader;
static struct ff_uniforms g_uniforms;
static uint g_bbox_vao;
static uint g_bbox_vbo;
static int g_max_texture_size;
static size_t g_max_handle = 0;
static struct ht_fpack_map g_fonts;

// NLM

struct ff_glyphs_vector ff_glyphs_vector_create() {
    ulong initial_size = 2;
    ulong elem_size = sizeof(struct ff_glyph);
    return (struct ff_glyphs_vector){
        .data = calloc(elem_size, initial_size),
        .size = 0,
        .capacity = elem_size * initial_size};
}

void ff_glyphs_vector_destroy(struct ff_glyphs_vector *v) {
    free(v->data);
    v->data = 0;
    v->size = 0;
    v->capacity = 0;
}

void ff_glyphs_vector_push(struct ff_glyphs_vector *v,
                           struct ff_glyph glyph) {
    ulong new_size = sizeof(struct ff_glyph) * (v->size + 1);

    if (new_size > v->capacity) {
        v->capacity *= 2;
        v->data = (struct ff_glyph *)realloc(v->data, v->capacity);
        assert(v->data != NULL && "bad alloc");
    }

    v->data[v->size++] = glyph;
}

void ff_glyphs_vector_cat(struct ff_glyphs_vector *dest,
                          struct ff_glyphs_vector *src) {
    ulong elem_size = sizeof(struct ff_glyph);
    ulong src_size = src->size * elem_size;

    if (src_size > dest->capacity) {
        dest->capacity += src_size;
        dest->data =
            (struct ff_glyph *)realloc(dest->data, dest->capacity);
    }

    memcpy(dest->data + dest->size, src->data, src_size);
}

void ff_glyphs_vector_clear(struct ff_glyphs_vector *v) {
    v->size = 0;
}

static const uint ht_fpack_table_size = 0x200;

unsigned int ff_ht_int_hash(int key) {
    ulong value = ((key >> 16) ^ key) * 0x45d9f3b;
    value = ((value >> 16) ^ value) * 0x45d9f3b;
    value = (value >> 16) ^ value;

    return value;
}

bool ht_fpack_map_entry_empty(struct ht_fpack_entry *entry) {
    return !entry->not_empty;
}

struct ht_fpack_entry ht_fpack_map_entry_new(
    int key, const struct ff_font_texture_pack value) {
    struct ht_fpack_entry result = {
        .not_empty = true,
        .key = key,
        .value = value,
        .next = 0,
    };
    return result;
}

void ht_fpack_map_entry_free(struct ht_fpack_entry entry) {
    // TODO FIX
    struct ht_fpack_entry *head = entry.next;
    while (head != NULL) {
        struct ht_fpack_entry *tmp = head;
        head = head->next;
        free(tmp);
    }
}

struct ht_fpack_map ht_fpack_map_create() {
    struct ht_fpack_map hashtable = {0};

    hashtable.entries = (struct ht_fpack_entry *)calloc(
        ht_fpack_table_size, sizeof(struct ht_fpack_entry));

    return hashtable;
}

void ht_fpack_map_set(struct ht_fpack_map *hashtable, int key,
                      struct ff_font_texture_pack value) {
    unsigned int slot = ff_ht_int_hash(key) % ht_fpack_table_size;

    struct ht_fpack_entry *entry = &hashtable->entries[slot];

    if (ht_fpack_map_entry_empty(entry)) {
        hashtable->entries[slot] = ht_fpack_map_entry_new(key, value);
        return;
    }

    struct ht_fpack_entry *prev = {0};
    struct ht_fpack_entry *head = entry;

    while (head != NULL) {
        if (head->key == key) {
            entry->value = value;
            return;
        }

        // walk to next
        prev = head;
        head = prev->next;
    }

    prev->next = (struct ht_fpack_entry *)calloc(
        1, sizeof(struct ht_fpack_entry));
    struct ht_fpack_entry next = ht_fpack_map_entry_new(key, value);
    memcpy(prev->next, &next, sizeof(struct ht_fpack_entry));
}

struct ff_font_texture_pack *ht_fpack_map_get(
    struct ht_fpack_map *hashtable, int key) {
    unsigned int slot = ff_ht_int_hash(key) % ht_fpack_table_size;

    struct ht_fpack_entry *entry = &hashtable->entries[slot];

    if (ht_fpack_map_entry_empty(entry)) {
        return NULL;
    }

    struct ht_fpack_entry *head = entry;
    while (head != NULL) {
        if (head->key == key) {
            return &head->value;
        }

        head = head->next;
    }

    // reaching here means there were >= 1 entries but no key match
    return NULL;
}

void ht_fpack_map_free(struct ht_fpack_map *ht) {
    for (ulong i = 0; i < ht_fpack_table_size; ++i) {
        struct ht_fpack_entry entry = ht->entries[i];

        if (ht_fpack_map_entry_empty(&entry)) continue;

        ht_fpack_map_entry_free(entry);
    }
    free(ht->entries);
}

////////////////////////////

static const uint ht_codepoint_table_size = 0x200;

bool ht_codepoint_map_entry_empty(struct ht_codepoint_entry *entry) {
    return !entry->not_empty;
}

struct ht_codepoint_entry ht_codepoint_map_entry_new(
    int key, struct ff_map_item value) {
    struct ht_codepoint_entry result = {
        .not_empty = true,
        .key = key,
        .value = value,
        .next = 0,
    };
    return result;
}

void ht_codepoint_map_entry_free(struct ht_codepoint_entry entry) {
    struct ht_codepoint_entry *head = entry.next;
    while (head != NULL) {
        struct ht_codepoint_entry *tmp = head;
        head = head->next;
        free(tmp);
    }
}

struct ht_codepoint_map ht_codepoint_map_create() {
    struct ht_codepoint_map hashtable = {0};

    hashtable.entries = (struct ht_codepoint_entry *)calloc(
        ht_codepoint_table_size, sizeof(struct ht_codepoint_entry));

    return hashtable;
}

void ht_codepoint_map_set(struct ht_codepoint_map *hashtable, int key,
                          struct ff_map_item value) {
    unsigned int slot = ff_ht_int_hash(key) % ht_codepoint_table_size;

    struct ht_codepoint_entry *entry = &hashtable->entries[slot];

    if (ht_codepoint_map_entry_empty(entry)) {
        hashtable->entries[slot] =
            ht_codepoint_map_entry_new(key, value);
        return;
    }

    struct ht_codepoint_entry *prev = {0};
    struct ht_codepoint_entry *head = entry;

    while (head != NULL) {
        if (head->key == key) {
            entry->value = value;
            return;
        }

        // walk to next
        prev = head;
        head = prev->next;
    }

    prev->next = (struct ht_codepoint_entry *)calloc(
        1, sizeof(struct ht_codepoint_entry));
    struct ht_codepoint_entry next =
        ht_codepoint_map_entry_new(key, value);
    memcpy(prev->next, &next, sizeof(struct ht_codepoint_entry));
}

struct ff_map_item *ht_codepoint_map_get(
    struct ht_codepoint_map *hashtable, int key) {
    unsigned int slot = ff_ht_int_hash(key) % ht_codepoint_table_size;

    struct ht_codepoint_entry *entry = &hashtable->entries[slot];

    if (ht_codepoint_map_entry_empty(entry)) {
        return NULL;
    }

    struct ht_codepoint_entry *head = entry;
    while (head != NULL) {
        if (head->key == key) {
            return &head->value;
        }

        head = head->next;
    }

    // reaching here means there were >= 1 entries but no key match
    return NULL;
}

void ht_codepoint_map_free(struct ht_codepoint_map *ht) {
    for (ulong i = 0; i < ht_codepoint_table_size; ++i) {
        struct ht_codepoint_entry entry = ht->entries[i];

        if (ht_codepoint_map_entry_empty(&entry)) continue;

        ht_codepoint_map_entry_free(entry);
    }
    free(ht->entries);
}

void ff_get_ortho_projection(float left, float right, float bottom,
                             float top, float near, float far,
                             float dest[][4]) {
    GLfloat rl, tb, fn;

    memcpy(dest, kmat4_zero_init, sizeof(kmat4_zero_init));

    rl = 1.0f / (right - left);
    tb = 1.0f / (top - bottom);
    fn = -1.0f / (far - near);

    dest[0][0] = 2.0f * rl;
    dest[1][1] = 2.0f * tb;
    dest[2][2] = 2.0f * fn;
    dest[3][0] = -(right + left) * rl;
    dest[3][1] = -(top + bottom) * tb;
    dest[3][2] = (far + near) * fn;
    dest[3][3] = 1.0f;
}

void ff_initialize(const char *sl_version) {
    unsigned eni = 1;
    char *c;
    c = (char *)&eni;
    g_system_endianess = *c == 1 ? endian_le : endian_be;

    if (g_system_endianess == endian_le) {
        g_utf8_to_utf32 = iconv_open("UTF-32LE", "UTF-8");
        g_utf32_to_utf8 = iconv_open("UTF-8", "UTF-32LE");
    } else {
        g_utf8_to_utf32 = iconv_open("UTF-32BE", "UTF-8BE");
        g_utf32_to_utf8 = iconv_open("UTF-8BE", "UTF-32BE");
    }

    assert(g_utf8_to_utf32 != (iconv_t)-1);
    assert(g_utf32_to_utf8 != (iconv_t)-1);

    const FT_Error error = FT_Init_FreeType(&g_ft_library);
    assert("Failed to initialize freetype2" && !error);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_max_texture_size);

    unsigned vertex_shader, geometry_shader, fragment_shader;
    bool err = compile_shader(g_msdf_vertex, GL_VERTEX_SHADER,
                              &vertex_shader, sl_version);
    assert("Failed to compile msdf vertex shader" && err);
    err = compile_shader(g_msdf_fragment, GL_FRAGMENT_SHADER,
                         &fragment_shader, sl_version);
    assert("Failed to compile msdf fragment shader" && err);
    err = (g_gen_shader = glCreateProgram());
    assert("Failed to generate shader program");

    glAttachShader(g_gen_shader, vertex_shader);
    glAttachShader(g_gen_shader, fragment_shader);
    glLinkProgram(g_gen_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    int link_status = 0;
    glGetProgramiv(g_gen_shader, GL_LINK_STATUS, &link_status);
    assert("Failed to link g_gen_shader" && link_status);

    g_uniforms.atlas_projection =
        glGetUniformLocation(g_gen_shader, "projection");
    g_uniforms.texture_offset =
        glGetUniformLocation(g_gen_shader, "offset");
    g_uniforms.glyph_height =
        glGetUniformLocation(g_gen_shader, "glyph_height");
    g_uniforms.offset = glGetUniformLocation(g_gen_shader, "offset");
    g_uniforms.translate =
        glGetUniformLocation(g_gen_shader, "translate");
    g_uniforms.scale = glGetUniformLocation(g_gen_shader, "scale");
    g_uniforms.range = glGetUniformLocation(g_gen_shader, "range");
    g_uniforms.meta_offset =
        glGetUniformLocation(g_gen_shader, "meta_offset");
    g_uniforms.point_offset =
        glGetUniformLocation(g_gen_shader, "point_offset");
    g_uniforms.metadata =
        glGetUniformLocation(g_gen_shader, "metadata");
    g_uniforms.point_data =
        glGetUniformLocation(g_gen_shader, "point_data");

    compile_shader(g_font_vertex, GL_VERTEX_SHADER, &vertex_shader,
                   sl_version);
    err = compile_shader(g_font_geometry, GL_GEOMETRY_SHADER,
                         &geometry_shader, sl_version);
    assert("Failed to compile geometry shader" && err);
    err = compile_shader(g_font_fragment, GL_FRAGMENT_SHADER,
                         &fragment_shader, sl_version);
    assert("Failed to compile font fragment shader" && err);
    err = (g_render_shader = glCreateProgram());
    assert("Faile to create g_render_shader" && err);

    glAttachShader(g_render_shader, vertex_shader);
    glAttachShader(g_render_shader, geometry_shader);
    glAttachShader(g_render_shader, fragment_shader);
    glLinkProgram(g_render_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    glGetProgramiv(g_render_shader, GL_LINK_STATUS, &link_status);
    assert("Failed to link g_render_shader" && link_status);

    g_uniforms.window_projection =
        glGetUniformLocation(g_render_shader, "projection");
    g_uniforms.font_atlas_projection =
        glGetUniformLocation(g_render_shader, "font_projection");
    g_uniforms.index =
        glGetUniformLocation(g_render_shader, "font_index");
    g_uniforms.atlas =
        glGetUniformLocation(g_render_shader, "font_atlas");
    g_uniforms.padding =
        glGetUniformLocation(g_render_shader, "padding");
    g_uniforms.dpi = glGetUniformLocation(g_render_shader, "dpi");
    g_uniforms.units_per_em =
        glGetUniformLocation(g_render_shader, "units_per_em");

    assert(g_uniforms.window_projection != -1);
    assert(g_uniforms.font_atlas_projection != -1);
    assert(g_uniforms.index != -1);
    assert(g_uniforms.atlas != -1);
    assert(g_uniforms.padding != -1);
    assert(g_uniforms.offset != -1);
    assert(g_uniforms.dpi != -1);
    assert(g_uniforms.units_per_em != -1);
    assert(g_uniforms.atlas_projection != -1);
    assert(g_uniforms.texture_offset != -1);
    assert(g_uniforms.translate != -1);
    assert(g_uniforms.scale != -1);
    assert(g_uniforms.range != -1);
    assert(g_uniforms.glyph_height != -1);
    assert(g_uniforms.meta_offset != -1);
    assert(g_uniforms.point_offset != -1);
    assert(g_uniforms.metadata != -1);
    assert(g_uniforms.point_data != -1);

    g_dpi[0] = 72.0;
    g_dpi[1] = 72.0;

    glGenVertexArrays(1, &g_bbox_vao);
    glGenBuffers(1, &g_bbox_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_bbox_vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), 0,
                 GL_STREAM_READ);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    g_fonts = ht_fpack_map_create();
    ff_new_load_font_from_memory(g_default_font, g_default_font_len,
                                 ff_default_font_config());
}

struct ff_font_config ff_default_font_config(void) {
    return (struct ff_font_config){
        .scale = 2.0f,
        .range = 2.2f,
        .texture_width = 1024,
        .texture_padding = 4,
    };
}

ff_font_id_t ff_new_load_font_from_memory(const unsigned char *bytes,
                                          size_t size,
                                          struct ff_font_config cfg) {
    ff_font_id_t handle = g_max_handle;
    ht_fpack_map_set(&g_fonts, handle,
                     (struct ff_font_texture_pack){0});
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, handle);
    fpack->font.font_path = 0;
    fpack->font.scale = cfg.scale;
    fpack->font.range = cfg.range;
    fpack->font.character_index.codepoint_map =
        ht_codepoint_map_create();

    fpack->atlas.texture_width = cfg.texture_width;
    fpack->atlas.padding = cfg.texture_padding;

    glGenBuffers(1, &fpack->atlas.index_buffer);
    glGenTextures(1, &fpack->atlas.index_texture);
    glGenTextures(1, &fpack->atlas.atlas_texture);
    glGenFramebuffers(1, &fpack->atlas.atlas_framebuffer);

    int err = !FT_New_Memory_Face(g_ft_library, bytes, size, 0,
                                  &fpack->font.face);
    assert(
        "Failed to create a new font face from memory"
        "invalid" &&
        err);
    FT_Select_Charmap(fpack->font.face, ft_encoding_unicode);
    fpack->font.vertical_advance =
        (float)(fpack->font.face->ascender -
                fpack->font.face->descender);

    glGenBuffers(1, &fpack->font.meta_input_buffer);
    glGenBuffers(1, &fpack->font.point_input_buffer);
    glGenTextures(1, &fpack->font.meta_input_texture);
    glGenTextures(1, &fpack->font.point_input_texture);

    gen_extended_ascii(handle);
    g_max_handle += 1;
    return handle;
}

ff_font_id_t ff_load_font(const char *path,
                          struct ff_font_config cfg) {
    ff_font_id_t handle = g_max_handle;
    ht_fpack_map_set(&g_fonts, handle,
                     (struct ff_font_texture_pack){0});
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, handle);
    fpack->font.font_path = path;
    fpack->font.scale = cfg.scale;
    fpack->font.range = cfg.range;
    fpack->font.character_index.codepoint_map =
        ht_codepoint_map_create();

    fpack->atlas.texture_width = cfg.texture_width;
    fpack->atlas.texture_height = g_max_texture_size;
    fpack->atlas.padding = cfg.texture_padding;
    glGenBuffers(1, &fpack->atlas.index_buffer);
    glGenTextures(1, &fpack->atlas.index_texture);
    glGenTextures(1, &fpack->atlas.atlas_texture);
    glGenFramebuffers(1, &fpack->atlas.atlas_framebuffer);

    int err = !FT_New_Face(g_ft_library, path, 0, &fpack->font.face);
    assert(
        "Failed to create a new font face, font path is probably "
        "invalid" &&
        err);
    FT_Select_Charmap(fpack->font.face, ft_encoding_unicode);
    fpack->font.vertical_advance =
        (float)(fpack->font.face->ascender -
                fpack->font.face->descender);

    glGenBuffers(1, &fpack->font.meta_input_buffer);
    glGenBuffers(1, &fpack->font.point_input_buffer);
    glGenTextures(1, &fpack->font.meta_input_texture);
    glGenTextures(1, &fpack->font.point_input_texture);

    gen_extended_ascii(handle);
    g_max_handle += 1;
    return handle;
}

void ff_unload_font(const ff_font_id_t font) {
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, font);
    if (fpack == NULL) return;
    FT_Done_Face(fpack->font.face);
    glDeleteBuffers(1, &fpack->font.meta_input_buffer);
    glDeleteBuffers(1, &fpack->font.point_input_buffer);
    glDeleteBuffers(1, &fpack->font.meta_input_texture);
    glDeleteBuffers(1, &fpack->font.point_input_texture);
    glDeleteBuffers(1, &fpack->atlas.index_buffer);
    glDeleteTextures(1, &fpack->atlas.index_texture);
    glDeleteTextures(1, &fpack->atlas.atlas_texture);
    glDeleteFramebuffers(1, &fpack->atlas.atlas_framebuffer);
    ht_codepoint_map_free(&fpack->font.character_index.codepoint_map);
}

struct ff_map_item *ff_map_get(struct ff_map *m, c32_t codepoint) {
    if (codepoint < 0xff && codepoint >= 0)
        return &m->extended_ascii[codepoint];
    struct ff_map_item *item =
        ht_codepoint_map_get(&m->codepoint_map, codepoint);

    if (item) return item;
    return NULL;
}

struct ff_map_item *ff_map_insert(struct ff_map *m,
                                  const c32_t codepoint) {
    if (codepoint < 0xff && codepoint >= 0) {
        m->extended_ascii[codepoint].codepoint = codepoint;
        m->extended_ascii[codepoint].codepoint_index =
            codepoint;  // TODO
        return &m->extended_ascii[codepoint];
    }

    ht_codepoint_map_set(&m->codepoint_map, codepoint,
                         (struct ff_map_item){0});
    struct ff_map_item *item =
        ht_codepoint_map_get(&m->codepoint_map, codepoint);
    return item;
}

int ff_gen_glyphs(const ff_font_id_t font, const c32_t *codepoints,
                  const ulong codepoints_count) {
    GLint original_viewport[4];
    glGetIntegerv(GL_VIEWPORT, original_viewport);

    int retval = -2;
    int nrender = codepoints_count;

    if (nrender <= 0) return -1;

    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, font);
    assert(fpack != NULL);

    size_t *meta_sizes = NULL, *point_sizes = NULL;
    struct ff_index_entry_t *atlas_index = NULL;
    void *point_data = NULL, *metadata = NULL;

    /* We will start with a square texture. */
    int new_texture_height = g_max_texture_size;
    int new_index_size =
        fpack->atlas.nallocated ? fpack->atlas.nallocated : 1;

    /* Calculate the amount of memory needed on the GPU.*/
    meta_sizes = calloc(nrender, sizeof(size_t));
    assert(meta_sizes);
    point_sizes = calloc(nrender, sizeof(size_t));
    assert(point_sizes);

    /* Amount of new memory needed for the index. */
    size_t index_size = nrender * sizeof(struct ff_index_entry_t);
    atlas_index = (struct ff_index_entry_t *)calloc(1, index_size);
    assert(atlas_index);

    size_t meta_size_sum = 0, point_size_sum = 0;
    for (size_t i = 0; (int)i < (int)nrender; ++i) {
        int index = codepoints[i];
        glyph_buffer_size(fpack->font.face, index, &meta_sizes[i],
                          &point_sizes[i]);

        meta_size_sum += meta_sizes[i];
        point_size_sum += point_sizes[i];
    }

    /* Allocate the calculated amount. */
    point_data = calloc(point_size_sum, 1);
    assert(point_data);
    metadata = calloc(meta_size_sum, 1);
    assert(metadata);

    /* Serialize the glyphs into RAM. */
    char *meta_ptr = (char *)metadata;
    char *point_ptr = (char *)point_data;
    for (size_t i = 0; (int)i < (int)nrender; ++i) {
        float buffer_width, buffer_height;

        int index = codepoints[i];
        int res = ff_serializer_serialize_glyph(
            fpack->font.face, index, meta_ptr, (float *)point_ptr);
        if (res) {
            printf("failed to thingify %d\n", index);
            continue;
        }

        struct ff_map_item *m =
            ff_map_insert(&fpack->font.character_index, index);
        m->codepoint_index = fpack->atlas.nglyphs + i;
        m->advance[0] =
            (float)fpack->font.face->glyph->metrics.horiAdvance;
        m->advance[1] =
            (float)fpack->font.face->glyph->metrics.vertAdvance;

        buffer_width = fpack->font.face->glyph->metrics.width /
                           g_serializer_scale +
                       fpack->font.range;
        buffer_height = fpack->font.face->glyph->metrics.height /
                            g_serializer_scale +
                        fpack->font.range;
        buffer_width *= fpack->font.scale;
        buffer_height *= fpack->font.scale;

        meta_ptr += meta_sizes[i];
        point_ptr += point_sizes[i];

        if (fpack->atlas.offset_x + buffer_width >
            fpack->atlas.texture_width) {
            fpack->atlas.offset_y +=
                (fpack->atlas.y_increment + fpack->atlas.padding);
            fpack->atlas.offset_x = 1;
            fpack->atlas.y_increment = 0;
        }
        fpack->atlas.y_increment =
            (size_t)buffer_height > fpack->atlas.y_increment
                ? (size_t)buffer_height
                : fpack->atlas.y_increment;

        atlas_index[i].offset_x = (GLfloat)fpack->atlas.offset_x;
        atlas_index[i].offset_y = (GLfloat)fpack->atlas.offset_y;
        atlas_index[i].size_x = buffer_width;
        atlas_index[i].size_y = buffer_height;
        atlas_index[i].bearing_x =
            (GLfloat)fpack->font.face->glyph->metrics.horiBearingX;
        atlas_index[i].bearing_y =
            (GLfloat)fpack->font.face->glyph->metrics.horiBearingY;
        atlas_index[i].glyph_width =
            (GLfloat)fpack->font.face->glyph->metrics.width;
        atlas_index[i].glyph_height =
            (GLfloat)fpack->font.face->glyph->metrics.height;

        fpack->atlas.offset_x +=
            (size_t)buffer_width + fpack->atlas.padding;

        while ((fpack->atlas.offset_y + buffer_height) >
               new_texture_height) {
            new_texture_height *= 2;
        }
        assert(new_texture_height <= g_max_texture_size);
        while ((int)(fpack->atlas.nglyphs + i) >= new_index_size) {
            new_index_size *= 2;
        }
    }

    /* Allocate and fill the buffers on GPU. */
    glBindBuffer(GL_ARRAY_BUFFER, fpack->font.meta_input_buffer);
    glBufferData(GL_ARRAY_BUFFER, meta_size_sum, metadata,
                 GL_DYNAMIC_READ);

    glBindBuffer(GL_ARRAY_BUFFER, fpack->font.point_input_buffer);
    glBufferData(GL_ARRAY_BUFFER, point_size_sum, point_data,
                 GL_DYNAMIC_READ);

    if ((int)fpack->atlas.nallocated == new_index_size) {
        glBindBuffer(GL_ARRAY_BUFFER, fpack->atlas.index_buffer);
    } else {
        GLuint new_buffer;
        glGenBuffers(1, &new_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, new_buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(struct ff_index_entry_t) * new_index_size,
                     0, GL_DYNAMIC_READ);
        assert(glGetError() != GL_OUT_OF_MEMORY);
        if (fpack->atlas.nglyphs) {
            glBindBuffer(GL_COPY_READ_BUFFER,
                         fpack->atlas.index_buffer);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER,
                                0, 0,
                                fpack->atlas.nglyphs *
                                    sizeof(struct ff_index_entry_t));
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }
        fpack->atlas.nallocated = new_index_size;
        glDeleteBuffers(1, &fpack->atlas.index_buffer);
        fpack->atlas.index_buffer = new_buffer;
    }
    glBufferSubData(
        GL_ARRAY_BUFFER,
        sizeof(struct ff_index_entry_t) * fpack->atlas.nglyphs,
        index_size, atlas_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Link sampler textures to the buffers. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->font.meta_input_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI,
                fpack->font.meta_input_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->font.point_input_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F,
                fpack->font.point_input_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->atlas.index_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F,
                fpack->atlas.index_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);

    /* Generate the atlas texture and bind it as the framebuffer. */
    if (fpack->atlas.texture_height == new_texture_height) {
        /* No need to extend the texture. */
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                          fpack->atlas.atlas_framebuffer);
        glBindTexture(GL_TEXTURE_2D, fpack->atlas.atlas_texture);
        glViewport(0, 0, fpack->atlas.texture_width,
                   fpack->atlas.texture_height);
    } else {
        GLuint new_texture;
        GLuint new_framebuffer;
        glGenTextures(1, &new_texture);
        glGenFramebuffers(1, &new_framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, new_framebuffer);

        glBindTexture(GL_TEXTURE_2D, new_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
                     fpack->atlas.texture_width, new_texture_height,
                     0, GL_RGBA, GL_FLOAT, NULL);

        assert(glGetError() != GL_OUT_OF_MEMORY);

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               new_texture, 0);
        glViewport(0, 0, fpack->atlas.texture_width,
                   new_texture_height);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        if (fpack->atlas.texture_height) {
            /* Old texture had data -> copy. */
            glBindFramebuffer(GL_READ_FRAMEBUFFER,
                              fpack->atlas.atlas_framebuffer);
            glBlitFramebuffer(0, 0, fpack->atlas.texture_width,
                              fpack->atlas.texture_height, 0, 0,
                              fpack->atlas.texture_width,
                              fpack->atlas.texture_height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        fpack->atlas.texture_height = new_texture_height;
        glDeleteTextures(1, &fpack->atlas.atlas_texture);
        fpack->atlas.atlas_texture = new_texture;
        glDeleteFramebuffers(1, &fpack->atlas.atlas_framebuffer);
        fpack->atlas.atlas_framebuffer = new_framebuffer;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    GLfloat framebuffer_projection[4][4];
    ff_get_ortho_projection(0, (GLfloat)fpack->atlas.texture_width, 0,
                            (GLfloat)fpack->atlas.texture_height,
                            -1.0, 1.0, framebuffer_projection);
    ff_get_ortho_projection(-(GLfloat)fpack->atlas.texture_width,
                            (GLfloat)fpack->atlas.texture_width,
                            -(GLfloat)fpack->atlas.texture_height,
                            (GLfloat)fpack->atlas.texture_height,
                            -1.0, 1.0, fpack->atlas.projection);
    glUseProgram(g_gen_shader);
    glUniform1i(g_uniforms.metadata, 0);
    glUniform1i(g_uniforms.point_data, 1);

    glUniformMatrix4fv(g_uniforms.atlas_projection, 1, GL_FALSE,
                       (GLfloat *)framebuffer_projection);

    glUniform2f(g_uniforms.scale, fpack->font.scale,
                fpack->font.scale);
    glUniform1f(g_uniforms.range, fpack->font.range);
    glUniform1i(g_uniforms.meta_offset, 0);
    glUniform1i(g_uniforms.point_offset, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr, "msdfgl: framebuffer incomplete: %x\n",
                glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->font.meta_input_texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->font.point_input_texture);

    glBindVertexArray(g_bbox_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_bbox_vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);

    int meta_offset = 0;
    int point_offset = 0;
    for (int i = 0; i < nrender; ++i) {
        struct ff_index_entry_t g = atlas_index[i];
        float w = g.size_x;
        float h = g.size_y;
        GLfloat bounding_box[] = {0, 0, w, 0, 0, h, 0, h, w, 0, w, h};
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bounding_box),
                        bounding_box);

        glUniform2f(
            g_uniforms.translate,
            -g.bearing_x / g_serializer_scale +
                fpack->font.range / 2.0f,
            (g.glyph_height - g.bearing_y) / g_serializer_scale +
                fpack->font.range / 2.0f);

        glUniform2f(g_uniforms.texture_offset, g.offset_x,
                    g.offset_y);
        glUniform1i(g_uniforms.meta_offset, meta_offset);
        glUniform1i(g_uniforms.point_offset,
                    point_offset / (2 * sizeof(GLfloat)));
        glUniform1f(g_uniforms.glyph_height, g.size_y);

        /* No need for draw call if there are no contours */
        if (((unsigned char *)metadata)[meta_offset])
            glDrawArrays(GL_TRIANGLES, 0, 6);

        meta_offset += meta_sizes[i];
        point_offset += point_sizes[i];
    }

    glDisableVertexAttribArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    fpack->atlas.nglyphs += nrender;
    retval = nrender;

    free(meta_sizes);
    free(point_sizes);
    free(atlas_index);
    free(point_data);
    free(metadata);

    glViewport(original_viewport[0], original_viewport[1],
               original_viewport[2], original_viewport[3]);

    return retval;
}

void ff_draw(ff_font_id_t font, const struct ff_glyph *glyphs,
             ulong glyphs_len, const float *projection) {
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, font);

    GLuint glyph_buffer;
    GLuint vao;
    glGenBuffers(1, &glyph_buffer);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, glyph_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 glyphs_len * sizeof(struct ff_glyph), &glyphs[0],
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    void *position_offset =
        (void *)offsetof(struct ff_glyph, position.x);
    void *color_offset = (void *)offsetof(struct ff_glyph, color);
    void *codepoint_offset =
        (void *)offsetof(struct ff_glyph, codepoint);
    void *size_offset = (void *)offsetof(struct ff_glyph, size);
    void *offset_offset =
        (void *)offsetof(struct ff_glyph, characteristics.offset);
    void *skew_offset =
        (void *)offsetof(struct ff_glyph, characteristics.skew);
    void *strength_offset =
        (void *)offsetof(struct ff_glyph, characteristics.strength);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct ff_glyph), position_offset);
    glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE,
                           sizeof(struct ff_glyph), color_offset);
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(struct ff_glyph),
                           codepoint_offset);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct ff_glyph), size_offset);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct ff_glyph), offset_offset);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct ff_glyph), skew_offset);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct ff_glyph), strength_offset);

    /* Enable gamma correction if user didn't enabled it */
    bool is_srgb_enabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    bool srgb_enabled_by_fn = !is_srgb_enabled;
    if (!is_srgb_enabled) glEnable(GL_FRAMEBUFFER_SRGB);

    glUseProgram(g_render_shader);
    /* Bind atlas texture and index buffer. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fpack->atlas.atlas_texture);
    glUniform1i(g_uniforms.atlas, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, fpack->atlas.index_texture);
    glUniform1i(g_uniforms.index, 1);

    glUniformMatrix4fv(g_uniforms.font_atlas_projection, 1, GL_FALSE,
                       (GLfloat *)fpack->atlas.projection);

    glUniformMatrix4fv(g_uniforms.window_projection, 1, GL_FALSE,
                       projection);
    glUniform1f(
        g_uniforms.padding,
        (GLfloat)(fpack->font.range / 2.0 * g_serializer_scale));
    glUniform1f(g_uniforms.units_per_em,
                (GLfloat)fpack->font.face->units_per_EM);
    glUniform2fv(g_uniforms.dpi, 1, g_dpi);

    /* Render the glyphs. */
    glDrawArrays(GL_POINTS, 0, glyphs_len);

    /* Clean up. */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    /* if the user didn't enabled it, disable it */
    if (srgb_enabled_by_fn) glDisable(GL_FRAMEBUFFER_SRGB);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &glyph_buffer);
    glDeleteVertexArrays(1, &vao);
}

struct ff_characteristics ff_get_default_characteristics() {
    return (struct ff_characteristics){
        .offset = 0.f, .skew = 0.f, .strength = .5f};
}

int ff_get_default_print_flags() {
    return ff_print_options_enable_kerning;
}

size_t ff_utf8_to_utf32(c32_t *dest, const char *src,
                        const ulong src_len) {
    // return size_t-1 if it fails, otherwise return the destination length
    char *in = (char *)src;
    size_t in_left = src_len;
    char *out = (char *)dest;
    size_t out_left = src_len * 4;
    size_t res =
        iconv(g_utf8_to_utf32, &in, &in_left, &out, &out_left);

    if (res == (size_t)-1) return res;
    return ((src_len) * 4 - out_left) / 4;
}

size_t ff_utf32_to_utf8(char *dest, const c32_t *src,
                        const ulong src_len) {
    // return size_t-1 if it fails, otherwise return the destination length
    char *in = (char *)src;
    size_t in_left = src_len * 4;
    char *out = dest;
    size_t out_left = src_len;
    size_t res =
        iconv(g_utf32_to_utf8, &in, &in_left, &out, &out_left);

    if (res == (size_t)-1) return res;
    return ((src_len) * 4 - out_left) / 4;
}

struct ff_dimensions ff_print_utf8(struct ff_glyphs_vector *vec,
                                   const char *str, size_t str_len,
                                   struct ff_print print, float pos_x,
                                   float pos_y) {
    c32_t str32[str_len];
    ff_utf8_to_utf32(str32, str, str_len);
    return ff_print_utf32(vec, str32, str_len, print, pos_x, pos_y);
}

struct ff_dimensions ff_print_utf32(struct ff_glyphs_vector *vec,
                                    const c32_t *str, size_t str_len,
                                    struct ff_print print,
                                    float pos_x, float pos_y) {
    struct ff_dimensions result = {0};
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, print.typography.font);
    float pos0_x = pos_x;
    float pos0_y = pos_y + print.typography.size;

    for (size_t i = 0; i < str_len; i++) {
        c32_t codepoint = str[i];

        struct ff_map_item *idx =
            ff_map_get(&fpack->font.character_index, codepoint);
        if (!idx) {
            ff_gen_glyphs(print.typography.font, &codepoint, 1);
            idx = ff_map_get(&fpack->font.character_index, codepoint);
            if (!idx) {
                codepoint = 0x25a1;
                idx = ff_map_get(&fpack->font.character_index,
                                 codepoint);
                assert(idx);
            }
        }

        FT_Vector kerning = {0};
        const bool kerning_is_viable =
            (print.options & ff_print_options_enable_kerning) &&
            FT_HAS_KERNING(fpack->font.face) && (i > 0);
        if (kerning_is_viable) {
            c32_t previous_character = str[i - 1];
            FT_Get_Kerning(
                fpack->font.face,
                FT_Get_Char_Index(fpack->font.face,
                                  previous_character),
                FT_Get_Char_Index(fpack->font.face, codepoint),
                FT_KERNING_UNSCALED, &kerning);
        }

        ff_glyphs_vector_push(vec, (struct ff_glyph){0});

        bool is_space = codepoint == L' ';
        bool is_tab = codepoint == L'\t';
        if (!is_space && !is_tab) {
            struct ff_glyph *new_glyph = &vec->data[vec->size - 1];
            new_glyph->position.x = pos0_x;
            new_glyph->position.y = pos0_y;
            new_glyph->color = print.typography.color;
            new_glyph->codepoint = idx->codepoint_index;
            new_glyph->size = print.typography.size;
            new_glyph->characteristics = print.characteristics;
        }

        float y_adv = (idx->advance[1] + kerning.y) *
                      (print.typography.size * g_dpi[0] / 72.0f) /
                      fpack->font.face->units_per_EM;
        float x_adv = (idx->advance[0] + kerning.x) *
                      (print.typography.size * g_dpi[0] / 72.0f) /
                      fpack->font.face->units_per_EM;
        if (is_tab) x_adv *= 4;

        if (print.options & ff_print_options_print_vertically) {
            pos0_y += y_adv;
            result.width = fmaxf(result.width, x_adv);
        } else {
            pos0_x += x_adv;
            result.height = fmaxf(result.height, x_adv);
        }
    }

    return result;
}

static struct ff_dimensions ff_measure(const ff_font_id_t font,
                                       const c32_t *str,
                                       size_t str_len,
                                       const float size,
                                       const bool with_kerning) {
    struct ff_font_texture_pack *fpack =
        ht_fpack_map_get(&g_fonts, font);

    struct ff_dimensions result = {0};

    for (size_t i = 0; i < str_len; i++) {
        c32_t codepoint = str[i];

        struct ff_map_item *idx =
            ff_map_get(&fpack->font.character_index, codepoint);
        if (!idx) {
            ff_gen_glyphs(font, &codepoint, 1);
            idx = ff_map_get(&fpack->font.character_index, codepoint);
            assert(idx);
        }

        FT_Vector kerning = {0};
        const bool should_get_kerning =
            with_kerning && FT_HAS_KERNING(fpack->font.face) &&
            (i > 0);
        if (should_get_kerning) {
            c32_t previous_character = str[i - 1];
            FT_Get_Kerning(
                fpack->font.face,
                FT_Get_Char_Index(fpack->font.face,
                                  previous_character),
                FT_Get_Char_Index(fpack->font.face, codepoint),
                FT_KERNING_UNSCALED, &kerning);
        }

        float height = (idx->advance[1] + kerning.y) *
                       (size * g_dpi[1] / 72.0f) /
                       fpack->font.face->units_per_EM;
        result.height = fmax(result.height, height);
        float x_adv = (idx->advance[0] + kerning.x) *
                      (size * g_dpi[0] / 72.0f) /
                      fpack->font.face->units_per_EM;
        if (codepoint == L'\t') x_adv *= 4;
        result.width += x_adv;
    }

    return result;
}

struct ff_dimensions ff_measure_utf32(const ff_font_id_t font,
                                      const c32_t *str,
                                      size_t str_len,
                                      const float size,
                                      const bool with_kerning) {
    return ff_measure(font, str, str_len, size, with_kerning);
}

struct ff_dimensions ff_measure_utf8(const ff_font_id_t font,
                                     const char *str,
                                     const size_t str_len,
                                     const float size,
                                     const bool with_kerning) {
    c32_t str32[str_len];
    ff_utf8_to_utf32(str32, str, str_len);

    return ff_measure(font, str32, str_len, size, with_kerning);
}

void ff_set_glyphs_pos(struct ff_glyph *glyphs, size_t count, float x,
                       float y) {
    if (!count) return;
    float advances_x[count];
    float advances_y[count];
    float prev_adv_x = glyphs[0].position.x;
    float prev_adv_y = glyphs[0].position.y;

    for (size_t i = 0; i < count; i += 1) {
        float tmp_prev_x = glyphs[i].position.x;
        float tmp_prev_y = glyphs[i].position.y;
        advances_x[i] = glyphs[i].position.x - prev_adv_x;
        advances_y[i] = glyphs[i].position.y - prev_adv_y;
        prev_adv_x = tmp_prev_x;
        prev_adv_y = tmp_prev_y;
    }

    float prev_x = x;
    for (size_t i = 0; i < count; i += 1) {
        float corrected_y = glyphs[i].size + y;
        glyphs[i].position.x = prev_x + advances_x[i];
        glyphs[i].position.y = corrected_y + advances_y[i];
        prev_x = glyphs[i].position.x;
    }
}

void ff_terminate() {
    for (ulong i = 0; i < g_max_handle; i += 1) ff_unload_font(i);
    ht_fpack_map_free(&g_fonts);
    FT_Done_FreeType(g_ft_library);
    iconv_close(g_utf32_to_utf8);
    iconv_close(g_utf8_to_utf32);
}
