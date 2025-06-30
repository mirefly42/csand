#include "font8x8_basic.h"
#include "math.h"
#include "nuklear_config.h"
#include "platform.h"
#include "renderer.h"
#include "vec2.h"
#include <GLES2/gl2.h>
#include <stddef.h>

#define FONT_GLYPH_WIDTH 8
#define FONT_GLYPH_HEIGHT 8
#define FONT_GLYPHS_COUNT 128
#define FONT_ATLAS_WIDTH FONT_GLYPH_WIDTH
#define FONT_ATLAS_HEIGHT (FONT_GLYPH_HEIGHT * FONT_GLYPHS_COUNT)

typedef struct CsandNuklearVertex {
    float pos[2];
    float uv[2];
    unsigned char color[4];
} CsandNuklearVertex;

typedef struct CsandRenderer {
    struct nk_user_font nk_font;
    CsandVec2Us world_size;
    CsandVec2Us framebuffer_size;
    CsandVec2Us viewport_offset;
    CsandVec2Us viewport_size;
    GLuint world_vbo;
    GLuint world_program;
    GLuint nuklear_vbo;
    GLuint nuklear_program;
    struct nk_context nk_ctx;
    struct nk_convert_config nk_convert_config;
    struct nk_draw_command nuklear_ctx_buffer_data[8 * 1024];
    struct nk_draw_command nuklear_command_buffer_data[1024];
    CsandNuklearVertex nuklear_vertex_buffer_data[64 * 1024];
    nk_draw_index nuklear_element_buffer_data[256 * 1024];
    struct nk_buffer cmds, vertices, elements;
} CsandRenderer;

CsandRenderer csand_renderer = {0};
static const char vertex_shader_src[] = {
#include "shader.vert.embed.h"
    '\0'
};
#define VERTEX_SHADER_SRC_LENGTH (sizeof(vertex_shader_src) - 1)

static const char fragment_shader_src[] = {
#include "shader.frag.embed.h"
    '\0'
};
#define FRAGMENT_SHADER_SRC_LENGTH (sizeof(fragment_shader_src) - 1)

static const char csand_nuklear_vert_src[] = {
#include "nuklear.vert.embed.h"
    '\0'
};
#define CSAND_NUKLEAR_VERT_SRC_LENGTH (sizeof(csand_nuklear_vert_src) - 1)

static const char csand_nuklear_frag_src[] = {
#include "nuklear.frag.embed.h"
    '\0'
};
#define CSAND_NUKLEAR_FRAG_SRC_LENGTH (sizeof(csand_nuklear_frag_src) - 1)

static GLuint csandLoadShaderProgram(
    const char *program_name,
    const char *vertex_shader_name, const char *vertex_shader_src, size_t vertex_shader_length,
    const char *fragment_shader_name, const char *fragment_shader_src, size_t fragment_shader_length
);

static GLuint csandLoadShader(const char *name, const char *src, size_t size, GLenum type);
static void csandUpdateWorldSize(CsandVec2Us world_size);

#define CSAND_RENDER_TEXTURE GL_TEXTURE0
#define CSAND_PALETTE_TEXTURE GL_TEXTURE1
#define CSAND_FONT_TEXTURE GL_TEXTURE2

static float csandNuklearTextWidth(nk_handle handle, float h, const char *text, int length) {
    (void)handle; (void)h; (void)text;

    return length * FONT_GLYPH_WIDTH;
}

static void csandNuklearQueryFontGlyph(nk_handle handle, float font_height, struct nk_user_font_glyph *glyph, nk_rune codepoint, nk_rune next_codepoint) {
    (void)handle; (void)font_height; (void)next_codepoint;

    glyph->width = FONT_GLYPH_WIDTH;
    glyph->height = FONT_GLYPH_HEIGHT;
    glyph->xadvance = FONT_GLYPH_WIDTH;
    glyph->offset = nk_vec2(0, 0);
    glyph->uv[0] = nk_vec2(0, codepoint * FONT_GLYPH_HEIGHT / (float)FONT_ATLAS_HEIGHT);
    glyph->uv[1] = nk_vec2(1, glyph->uv[0].y + FONT_GLYPH_HEIGHT / (float)FONT_ATLAS_HEIGHT);
}

static void csandNuklearInit(void) {
    unsigned char font_atlas[FONT_ATLAS_HEIGHT][FONT_ATLAS_WIDTH];
    for (int y = 0; y < FONT_ATLAS_HEIGHT; y++) {
        for (int x = 0; x < FONT_ATLAS_WIDTH; x++) {
            font_atlas[y][x] = font8x8_basic[y / FONT_GLYPH_HEIGHT][y % FONT_GLYPH_HEIGHT] & (1 << x) ? 0xFF : 0x00;
        }
    }
    font_atlas[0][0] = 0xFF; // white pixel
    struct nk_draw_null_texture null_texture = {nk_handle_id(CSAND_FONT_TEXTURE), nk_vec2(0, 0)};

    GLuint font_texture;
    glGenTextures(1, &font_texture);

    glActiveTexture(CSAND_FONT_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, font_atlas);

    csand_renderer.nk_font.height = FONT_GLYPH_HEIGHT;
    csand_renderer.nk_font.width = csandNuklearTextWidth;
    csand_renderer.nk_font.query = csandNuklearQueryFontGlyph;

    if (!nk_init_fixed(
            &csand_renderer.nk_ctx,
            csand_renderer.nuklear_ctx_buffer_data,
            sizeof(csand_renderer.nuklear_ctx_buffer_data),
            &csand_renderer.nk_font
    )) {
        csandPlatformPrintErr("failed to initialize nuklear context\n");
    }

    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(CsandNuklearVertex, pos)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(CsandNuklearVertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(CsandNuklearVertex, color)},
        {NK_VERTEX_LAYOUT_END},
    };

    csand_renderer.nk_convert_config = (struct nk_convert_config){
        .global_alpha = 1.0f,
        .line_AA = NK_ANTI_ALIASING_ON,
        .shape_AA = NK_ANTI_ALIASING_ON,
        .circle_segment_count = 22,
        .arc_segment_count = 22,
        .curve_segment_count = 22,
        .tex_null = null_texture,
        .vertex_layout = vertex_layout,
        .vertex_size = sizeof(CsandNuklearVertex),
        .vertex_alignment = NK_ALIGNOF(CsandNuklearVertex),
    };

    GLuint ebo;
    glGenBuffers(1, &csand_renderer.nuklear_vbo);
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, csand_renderer.nuklear_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(csand_renderer.nuklear_vertex_buffer_data), NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(csand_renderer.nuklear_element_buffer_data), NULL, GL_STREAM_DRAW);

    nk_buffer_init_fixed(&csand_renderer.cmds, csand_renderer.nuklear_command_buffer_data, sizeof(csand_renderer.nuklear_command_buffer_data));
    nk_buffer_init_fixed(&csand_renderer.vertices, csand_renderer.nuklear_vertex_buffer_data, sizeof(csand_renderer.nuklear_vertex_buffer_data));
    nk_buffer_init_fixed(&csand_renderer.elements, csand_renderer.nuklear_element_buffer_data, sizeof(csand_renderer.nuklear_element_buffer_data));

    csand_renderer.nuklear_program = csandLoadShaderProgram(
        "nuklear",
        "nuklear.vert", csand_nuklear_vert_src, CSAND_NUKLEAR_VERT_SRC_LENGTH,
        "nuklear.frag", csand_nuklear_frag_src, CSAND_NUKLEAR_FRAG_SRC_LENGTH
    );

    glUseProgram(csand_renderer.nuklear_program);
    glUniform1i(glGetUniformLocation(csand_renderer.nuklear_program, "texture"), 2);
}

void csandRendererInit(CsandVec2Us world_size, CsandVec2Us framebuffer_size, const CsandRgba *colors, uint8_t colors_count) {
    glGenBuffers(1, &csand_renderer.world_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, csand_renderer.world_vbo);
    GLbyte vbo_data[3*2] = {
        -3, -1,
         1, -1,
         1,  3,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_data), vbo_data, GL_STATIC_DRAW);

    csand_renderer.world_program = csandLoadShaderProgram(
        "world",
        "shader.vert", vertex_shader_src, VERTEX_SHADER_SRC_LENGTH,
        "shader.frag", fragment_shader_src, FRAGMENT_SHADER_SRC_LENGTH
    );
    glUseProgram(csand_renderer.world_program);

    GLuint render_texture;
    glGenTextures(1, &render_texture);

    glActiveTexture(CSAND_RENDER_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, render_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUniform1i(glGetUniformLocation(csand_renderer.world_program, "texture"), 0);

    GLuint palette_texture;
    glGenTextures(1, &palette_texture);

    glActiveTexture(CSAND_PALETTE_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, palette_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    csandRendererSetPalette(colors, colors_count);

    csandNuklearInit();
    csandUpdateWorldSize(world_size);
    csandRendererUpdateViewport(framebuffer_size);
}

void csandRendererSetPalette(const CsandRgba *colors, uint8_t colors_count) {
    glActiveTexture(CSAND_PALETTE_TEXTURE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, colors_count, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);
    glUseProgram(csand_renderer.world_program);
    glUniform1i(glGetUniformLocation(csand_renderer.world_program, "palette"), 1);
    glUniform1i(glGetUniformLocation(csand_renderer.world_program, "palette_size"), colors_count);
}

static void csandRendererRenderNuklear(void) {
    nk_convert(
        &csand_renderer.nk_ctx,
        &csand_renderer.cmds,
        &csand_renderer.vertices,
        &csand_renderer.elements,
        &csand_renderer.nk_convert_config
    );

    glBindBuffer(GL_ARRAY_BUFFER, csand_renderer.nuklear_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, csand_renderer.vertices.size, nk_buffer_memory_const(&csand_renderer.vertices));
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, csand_renderer.elements.size, nk_buffer_memory_const(&csand_renderer.elements));

    GLint position_attr_loc = glGetAttribLocation(csand_renderer.nuklear_program, "position");
    glVertexAttribPointer(position_attr_loc, 2, GL_FLOAT, GL_FALSE, sizeof(CsandNuklearVertex), (void *)offsetof(CsandNuklearVertex, pos));
    glEnableVertexAttribArray(position_attr_loc);

    GLint uv_attr_loc = glGetAttribLocation(csand_renderer.nuklear_program, "uv");
    glVertexAttribPointer(uv_attr_loc, 2, GL_FLOAT, GL_FALSE, sizeof(CsandNuklearVertex), (void *)offsetof(CsandNuklearVertex, uv));
    glEnableVertexAttribArray(uv_attr_loc);

    GLint color_attr_loc = glGetAttribLocation(csand_renderer.nuklear_program, "color");
    glVertexAttribPointer(color_attr_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CsandNuklearVertex), (void *)offsetof(CsandNuklearVertex, color));
    glEnableVertexAttribArray(color_attr_loc);

    glUseProgram(csand_renderer.nuklear_program);
    glActiveTexture(CSAND_FONT_TEXTURE);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

    glViewport(0, 0, csand_renderer.framebuffer_size.x, csand_renderer.framebuffer_size.y);
    glUniform2f(glGetUniformLocation(csand_renderer.nuklear_program, "framebuffer_size"), csand_renderer.framebuffer_size.x, csand_renderer.framebuffer_size.y);

    const struct nk_draw_command *cmd = NULL;
    unsigned int drawn_elements = 0;
    nk_draw_foreach(cmd, &csand_renderer.nk_ctx, &csand_renderer.cmds) {
        if (cmd->elem_count == 0) {
            continue;
        }

        glScissor(cmd->clip_rect.x, csand_renderer.framebuffer_size.y - 1 - cmd->clip_rect.y - cmd->clip_rect.h, cmd->clip_rect.w, cmd->clip_rect.h);
        glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT, (const void *)(drawn_elements * sizeof(nk_draw_index)));
        drawn_elements += cmd->elem_count;
    }

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    nk_buffer_clear(&csand_renderer.elements);
    nk_buffer_clear(&csand_renderer.vertices);
    nk_buffer_clear(&csand_renderer.cmds);
}

void csandRendererRender(unsigned char *data, unsigned short width, unsigned short height) {
    glClearColor(0.06, 0.12, 0.17, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (width != csand_renderer.world_size.x || height != csand_renderer.world_size.y) {
        csandUpdateWorldSize((CsandVec2Us){width, height});
    }

    glBindBuffer(GL_ARRAY_BUFFER, csand_renderer.world_vbo);
    GLint position_attr_loc = glGetAttribLocation(csand_renderer.world_program, "position");
    glVertexAttribPointer(position_attr_loc, 2, GL_BYTE, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_attr_loc);

    glActiveTexture(CSAND_RENDER_TEXTURE);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

    glUseProgram(csand_renderer.world_program);

    glViewport(
        csand_renderer.viewport_offset.x,
        csand_renderer.viewport_offset.y,
        csand_renderer.viewport_size.x,
        csand_renderer.viewport_size.y
    );

    glEnable(GL_CULL_FACE);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisable(GL_CULL_FACE);

    csandRendererRenderNuklear();
}

void csandRendererUpdateViewport(CsandVec2Us framebuffer_size) {
    csand_renderer.framebuffer_size = framebuffer_size;
    float scale_x = framebuffer_size.x / (float)csand_renderer.world_size.x;
    float scale_y = framebuffer_size.y / (float)csand_renderer.world_size.y;

    CsandVec2F viewport_size_float = csandVec2FMulScalar(
        CSAND_VEC2_CONVERT(CsandVec2F, csand_renderer.world_size),
        csandFMin(scale_x, scale_y)
    );

    csand_renderer.viewport_size = CSAND_VEC2_CONVERT(CsandVec2Us, viewport_size_float);
    csand_renderer.viewport_offset = csandVec2UsSub(
        csandVec2UsDivScalar(framebuffer_size, 2),
        csandVec2UsDivScalar(csand_renderer.viewport_size, 2)
    );
}

CsandVec2Us csandRendererScreenSpaceToWorldSpace(CsandVec2Us vec) {
    CsandVec2L result = csandVec2LClamp(
        csandVec2LDiv(
            csandVec2LMul(
                csandVec2LSub(
                    (CsandVec2L){
                        vec.x,
                        csandPlatformGetWindowSize().y - 1 - vec.y,
                    },
                    CSAND_VEC2_CONVERT(CsandVec2L, csand_renderer.viewport_offset)
                ),
                CSAND_VEC2_CONVERT(CsandVec2L, csand_renderer.world_size)
            ),
            CSAND_VEC2_CONVERT(CsandVec2L, csand_renderer.viewport_size)
        ),
        (CsandVec2L){0},
        csandVec2LSubScalar(CSAND_VEC2_CONVERT(CsandVec2L, csand_renderer.world_size), 1)
    );

    return CSAND_VEC2_CONVERT(CsandVec2Us, result);
}

struct nk_context *csandRendererNuklearContext(void) {
    return &csand_renderer.nk_ctx;
}

static GLuint csandLoadShaderProgram(
    const char *program_name,
    const char *vertex_shader_name, const char *vertex_shader_src, size_t vertex_shader_length,
    const char *fragment_shader_name, const char *fragment_shader_src, size_t fragment_shader_length
) {
    GLuint vertex_shader = csandLoadShader(vertex_shader_name, vertex_shader_src, vertex_shader_length, GL_VERTEX_SHADER);
    GLuint fragment_shader = csandLoadShader(fragment_shader_name, fragment_shader_src, fragment_shader_length, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        char info_log[1024];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        csandPlatformPrintErr("failed to link shader program '");
        csandPlatformPrintErr(program_name);
        csandPlatformPrintErr("': ");
        csandPlatformPrintErr(info_log);
        csandPlatformPrintErr("\n");
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    return program;
}

static GLuint csandLoadShader(const char *name, const char *src, size_t size, GLenum type) {
    GLenum shader = glCreateShader(type);

    GLint length[1] = {size};
    glShaderSource(shader, 1, (const char **)&src, length);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char info_log[1024];

        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
        csandPlatformPrintErr("failed to compile shader '");
        csandPlatformPrintErr(name);
        csandPlatformPrintErr("': ");
        csandPlatformPrintErr(info_log);
        csandPlatformPrintErr("\n");

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static void csandUpdateWorldSize(CsandVec2Us world_size) {
    glActiveTexture(CSAND_RENDER_TEXTURE);

    csand_renderer.world_size = world_size;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, world_size.x, world_size.y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
}
