#include "platform.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <epoxy/gl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512

#define WIDTH 128
#define HEIGHT 64

static GLFWwindow *window;
static GLuint texture = 0;
static GLuint program = 0;

static void csandGlfwErrorCallback(int code, const char *msg);
static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static GLuint csandLoadShaderProgram(void);
static GLuint csandLoadShader(const char *path, GLenum type);
static char *csandReadTextFile(const char *path, long *out_size);
static CsandRenderCallback render_callback = NULL;
static CsandInputCallback input_callback = NULL;

void csandPlatformInit(void) {
    glfwSetErrorCallback(csandGlfwErrorCallback);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "csand", NULL, NULL);

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, csandGlfwKeyCallback);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLbyte vbo_data[3*2] = {
        -3, -1,
         1, -1,
         1,  3,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_data), vbo_data, GL_STATIC_DRAW);

    program = csandLoadShaderProgram();
    glUseProgram(program);

    GLint position_attr_loc = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attr_loc, 2, GL_BYTE, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_attr_loc);


    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, WIDTH, HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    glUniform1i(glGetUniformLocation(program, "texture"), 0);
}

static void csandGlfwErrorCallback(int code, const char *msg) {
    fprintf(stderr, "glfw error %d: %s\n", code, msg);
}

static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (!input_callback) {
        return;
    }

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE:
                input_callback(CSAND_INPUT_PAUSE_TOGGLE);
                break;
            case GLFW_KEY_0:
                input_callback(CSAND_INPUT_SELECT_MAT0);
                break;
            case GLFW_KEY_1:
                input_callback(CSAND_INPUT_SELECT_MAT1);
                break;
            case GLFW_KEY_2:
                input_callback(CSAND_INPUT_SELECT_MAT2);
                break;
            case GLFW_KEY_3:
                input_callback(CSAND_INPUT_SELECT_MAT3);
                break;
            case GLFW_KEY_EQUAL:
                input_callback(CSAND_INPUT_SPEED_INCREASE);
                break;
            case GLFW_KEY_MINUS:
                input_callback(CSAND_INPUT_SPEED_DECREASE);
                break;
            default:
                break;
        }
    }
}

static GLuint csandLoadShaderProgram(void) {
    GLuint vertex_shader = csandLoadShader("shader.vert", GL_VERTEX_SHADER);
    GLuint fragment_shader = csandLoadShader("shader.frag", GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        char info_log[1024];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "failed to link shader program: %s\n", info_log);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    return program;
}

static GLuint csandLoadShader(const char *path, GLenum type) {
    long size = 0;
    char *code = csandReadTextFile(path, &size);
    if (!code) {
        return 0;
    }

    GLenum shader = glCreateShader(type);

    GLint length[1] = {size};
    glShaderSource(shader, 1, (const char **)&code, length);
    free(code);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char info_log[1024];

        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "failed to compile shader '%s': %s\n", path, info_log);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static char *csandReadTextFile(const char *path, long *out_size) {
    *out_size = 0;

    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "failed to open file '%s': %s\n", path, strerror(errno));
        goto fail0;
    }

    if (fseek(file, 0, SEEK_END) < 0) {
        perror("fseek");
        goto fail1;
    }

    long size = ftell(file);
    if (size < 0) {
        perror("ftell");
        goto fail1;
    }

    rewind(file);

    char *buf = malloc(size);
    if (!buf) {
        perror("malloc");
        goto fail1;
    }

    if (fread(buf, size, 1, file) != 1) {
        perror("fread");
        goto fail2;
    }

    fclose(file);
    *out_size = size;
    return buf;

fail2:
    free(buf);
fail1:
    fclose(file);
fail0:
    return NULL;
}

void csandPlatformSetPalette(const CsandRgba *colors, uint8_t colors_count) {
    GLuint palette;
    glGenTextures(1, &palette);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, palette);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, colors_count, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);

    glUniform1i(glGetUniformLocation(program, "palette"), 1);
    glUniform1i(glGetUniformLocation(program, "palette_size"), colors_count);

    glActiveTexture(GL_TEXTURE0);
}

void csandPlatformSetRenderCallback(CsandRenderCallback callback) {
    render_callback = callback;
}

void csandPlatformSetInputCallback(CsandInputCallback callback) {
    input_callback = callback;
}

unsigned int csandPlatformIsMouseButtonPressed(CsandMouseButton button) {
    int glfw_button;
    switch (button) {
        case CSAND_MOUSE_BUTTON_LEFT:
            glfw_button = GLFW_MOUSE_BUTTON_LEFT;
            break;
        case CSAND_MOUSE_BUTTON_RIGHT:
            glfw_button = GLFW_MOUSE_BUTTON_RIGHT;
            break;
        default:
            return 0;
    }

    return glfwGetMouseButton(window, glfw_button) == GLFW_PRESS;
}

void csandPlatformGetCursorPos(unsigned short *out_x, unsigned short *out_y) {
        double fx, fy;
        glfwGetCursorPos(window, &fx, &fy);

        int x = (int)fx * WIDTH / WINDOW_WIDTH;
        int y = HEIGHT - 1 - (int)fy * HEIGHT / WINDOW_HEIGHT;

        if (x < 0) x = 0;
        if (x >= WIDTH) x = WIDTH - 1;

        if (y < 0) y = 0;
        if (y >= HEIGHT) y = HEIGHT - 1;

        *out_x = x;
        *out_y = y;
}

void csandPlatformRun(void) {
    unsigned char data[WIDTH * HEIGHT] = {0};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (render_callback) {
            render_callback(data, WIDTH, HEIGHT);
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }
}
