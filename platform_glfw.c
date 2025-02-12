#include "platform.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512

#define WIDTH 128
#define HEIGHT 64

static GLFWwindow *window;

static void csandGlfwErrorCallback(int code, const char *msg);
static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static CsandRenderCallback render_callback = NULL;
static CsandInputCallback input_callback = NULL;

void csandPlatformInit(void) {
    glfwSetErrorCallback(csandGlfwErrorCallback);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "csand", NULL, NULL);

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, csandGlfwKeyCallback);
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
            case GLFW_KEY_EQUAL:
                input_callback(CSAND_INPUT_SPEED_INCREASE);
                break;
            case GLFW_KEY_MINUS:
                input_callback(CSAND_INPUT_SPEED_DECREASE);
                break;
            case GLFW_KEY_PERIOD:
                input_callback(CSAND_INPUT_NEXT_FRAME);
                break;
            default:
                if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                    input_callback(key - GLFW_KEY_0 + CSAND_INPUT_SELECT_MAT0);
                }
                break;
        }
    }
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
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (render_callback) {
            render_callback();
        }

        glfwSwapBuffers(window);
    }
}

void csandPlatformPrintErr(const char *str) {
    fwrite(str, 1, strlen(str), stderr);
}
