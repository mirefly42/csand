#include "renderer.h"
#include "vec2.h"
#include "math.h"
#include "platform.h"
#include <GLES2/gl2.h>
#include <stddef.h>

typedef struct CsandRenderer {
    CsandVec2Us world_size;
    CsandVec2Us viewport_offset;
    CsandVec2Us viewport_size;
    GLuint program;
} CsandRenderer;

CsandRenderer csand_renderer;
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

static GLuint csandLoadShaderProgram(void);
static GLuint csandLoadShader(const char *name, const char *src, size_t size, GLenum type);
static void csandUpdateWorldSize(CsandVec2Us world_size);

#define CSAND_RENDER_TEXTURE GL_TEXTURE0
#define CSAND_PALETTE_TEXTURE GL_TEXTURE1

void csandRendererInit(CsandVec2Us world_size, CsandVec2Us framebuffer_size, const CsandRgba *colors, uint8_t colors_count) {
    csand_renderer = (CsandRenderer){0};

    glEnable(GL_CULL_FACE);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLbyte vbo_data[3*2] = {
        -3, -1,
         1, -1,
         1,  3,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_data), vbo_data, GL_STATIC_DRAW);

    csand_renderer.program = csandLoadShaderProgram();
    glUseProgram(csand_renderer.program);

    GLint position_attr_loc = glGetAttribLocation(csand_renderer.program, "position");
    glVertexAttribPointer(position_attr_loc, 2, GL_BYTE, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_attr_loc);

    GLuint render_texture;
    glGenTextures(1, &render_texture);

    glActiveTexture(CSAND_RENDER_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, render_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUniform1i(glGetUniformLocation(csand_renderer.program, "texture"), 0);

    GLuint palette_texture;
    glGenTextures(1, &palette_texture);

    glActiveTexture(CSAND_PALETTE_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, palette_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, colors_count, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);

    glUniform1i(glGetUniformLocation(csand_renderer.program, "palette"), 1);
    glUniform1i(glGetUniformLocation(csand_renderer.program, "palette_size"), colors_count);

    csandUpdateWorldSize(world_size);
    csandRendererUpdateViewport(framebuffer_size);
}

void csandRendererRender(unsigned char *data, unsigned short width, unsigned short height) {
    glClearColor(0.06, 0.12, 0.17, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (width != csand_renderer.world_size.x || height != csand_renderer.world_size.y) {
        csandUpdateWorldSize((CsandVec2Us){width, height});
    }

    glActiveTexture(CSAND_RENDER_TEXTURE);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void csandRendererUpdateViewport(CsandVec2Us framebuffer_size) {
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

    glViewport(
        csand_renderer.viewport_offset.x,
        csand_renderer.viewport_offset.y,
        csand_renderer.viewport_size.x,
        csand_renderer.viewport_size.y
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

static GLuint csandLoadShaderProgram(void) {
    GLuint vertex_shader = csandLoadShader("shader.vert", vertex_shader_src, VERTEX_SHADER_SRC_LENGTH, GL_VERTEX_SHADER);
    GLuint fragment_shader = csandLoadShader("shader.frag", fragment_shader_src, FRAGMENT_SHADER_SRC_LENGTH, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        char info_log[1024];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        csandPlatformPrintErr("failed to link shader program: ");
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
