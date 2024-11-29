#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <epoxy/gl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512
#define WIDTH 128
#define HEIGHT 64
#define SPEED_LIMIT 128

enum {
    MAT_AIR,
    MAT_SOLID,
    MAT_SAND,
    MAT_LIQUID,
    MAT_UPDATED_BIT = 0x80
};

static GLFWwindow *window = NULL;
static GLuint texture = 0;
static unsigned char buf[HEIGHT][WIDTH] = {0};
static unsigned int pause = 0;
static unsigned long speed = 1;
static unsigned char draw_mat = MAT_SAND;

static void csandInit(void);
static void csandGlfwErrorCallback(int code, const char *description);
static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static GLuint csandLoadShaderProgram(void);
static GLuint csandLoadShader(const char *path, GLenum type);
static char *csandReadTextFile(const char *path, long *out_size);
static void csandDraw(void);
static void csandSimulate(void);
static bool csandInBounds(int x, int y);
static void csandTerminate(void);

int main(void) {
    csandInit();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        csandDraw();

        glfwSwapBuffers(window);
    }

    csandTerminate();
}

static void csandInit(void) {
    srand(time(0));

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

    GLuint program = csandLoadShaderProgram();
    glUseProgram(program);

    GLint position_attr_loc = glGetAttribLocation(program, "position");
    glVertexAttribPointer(position_attr_loc, 2, GL_BYTE, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_attr_loc);


    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, WIDTH, HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    glUniform1i(glGetUniformLocation(program, "texture"), 0);
}

static void csandGlfwErrorCallback(int code, const char *description) {
    fprintf(stderr, "glfw error %d: %s\n", code, description);
}

static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE:
                pause = !pause;
                break;
            case GLFW_KEY_0:
                draw_mat = MAT_AIR;
                break;
            case GLFW_KEY_1:
                draw_mat = MAT_SOLID;
                break;
            case GLFW_KEY_2:
                draw_mat = MAT_SAND;
                break;
            case GLFW_KEY_3:
                draw_mat = MAT_LIQUID;
                break;
            case GLFW_KEY_EQUAL:
                if (speed < SPEED_LIMIT) {
                    speed *= 2;
                }
                printf("speed: %lu\n", speed);
                break;
            case GLFW_KEY_MINUS:
                if (speed > 1) {
                    speed /= 2;
                }
                printf("speed: %lu\n", speed);
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

static void csandDraw(void) {
    if (!pause) {
        for (unsigned long i = 0; i < speed; i++) {
            csandSimulate();
        }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}

static void csandSimulate(void) {
    for (unsigned int y = 0; y < HEIGHT; y++) {
        for (unsigned int x = 0; x < WIDTH; x++) {
            unsigned char mat = buf[y][x];

            switch (mat) {
                case MAT_SAND:
                    if (y <= 0) {
                        continue;
                    } else {
                        int dx = rand() % 3 - 1;
                        if (!csandInBounds(x + dx, y - 1)) continue;
                        unsigned char swap_mat = buf[y - 1][x + dx];

                        if (swap_mat == MAT_AIR || swap_mat == MAT_LIQUID) {
                            buf[y][x] = swap_mat;
                            buf[y - 1][x + dx] = mat | MAT_UPDATED_BIT;
                        }
                    }
                    break;
                case MAT_LIQUID: {
                    int dx = rand() % 3 - 1;
                    int dy = - (rand() % 2);
                    if (!csandInBounds(x + dx, y + dy)) continue;

                    if (buf[y + dy][x + dx] == MAT_AIR) {
                        buf[y][x] = MAT_AIR;
                        buf[y + dy][x + dx] = mat | MAT_UPDATED_BIT;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double fx, fy;
        glfwGetCursorPos(window, &fx, &fy);

        int x = (int)fx * WIDTH / WINDOW_WIDTH;
        int y = HEIGHT - 1 - (int)fy * HEIGHT / WINDOW_HEIGHT;

        if (csandInBounds(x, y)) {
            buf[y][x] = draw_mat | MAT_UPDATED_BIT;
        }
    }

    for (unsigned int i = 0; i < WIDTH*HEIGHT; i++) {
        ((unsigned char *)buf)[i] &= ~MAT_UPDATED_BIT;
    }
}

static bool csandInBounds(int x, int y) {
    return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

static void csandTerminate(void) {
    glfwTerminate();
}
