#include "math.h"
#include "platform.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct CsandPlatform {
    GLFWwindow *window;
    CsandRenderCallback render_callback;
    CsandInputCallback input_callback;
    CsandFramebufferSizeCallback framebuffer_size_callback;
} CsandPlatform;

static CsandPlatform platform = {0};

static void csandGlfwErrorCallback(int code, const char *msg) {
    fprintf(stderr, "glfw error %d: %s\n", code, msg);
}

static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (!platform.input_callback) {
        return;
    }

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE:
                platform.input_callback(CSAND_INPUT_PAUSE_TOGGLE);
                break;
            case GLFW_KEY_EQUAL:
                platform.input_callback(CSAND_INPUT_SPEED_INCREASE);
                break;
            case GLFW_KEY_MINUS:
                platform.input_callback(CSAND_INPUT_SPEED_DECREASE);
                break;
            case GLFW_KEY_PERIOD:
                platform.input_callback(CSAND_INPUT_NEXT_FRAME);
                break;
            default:
                if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                    platform.input_callback(key - GLFW_KEY_0 + CSAND_INPUT_SELECT_MAT0);
                }
                break;
        }
    }
}

static void csandGlfwFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    (void)window;

    if (platform.framebuffer_size_callback) {
        platform.framebuffer_size_callback((CsandVec2Us){
            csandUiMin(width, USHRT_MAX),
            csandUiMin(height, USHRT_MAX),
        });
    }
}

void csandPlatformInit(void) {
    glfwSetErrorCallback(csandGlfwErrorCallback);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    platform.window = glfwCreateWindow(1024, 512, "csand", NULL, NULL);

    glfwMakeContextCurrent(platform.window);

    glfwSetKeyCallback(platform.window, csandGlfwKeyCallback);
    glfwSetFramebufferSizeCallback(platform.window, csandGlfwFramebufferSizeCallback);
}

void csandPlatformSetRenderCallback(CsandRenderCallback callback) {
    platform.render_callback = callback;
}

void csandPlatformSetInputCallback(CsandInputCallback callback) {
    platform.input_callback = callback;
}

void csandPlatformSetFramebufferSizeCallback(CsandFramebufferSizeCallback callback) {
    platform.framebuffer_size_callback = callback;
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

    return glfwGetMouseButton(platform.window, glfw_button) == GLFW_PRESS;
}

CsandVec2Us csandPlatformGetCursorPos(void) {
    double fx, fy;
    glfwGetCursorPos(platform.window, &fx, &fy);

    return (CsandVec2Us){
        csandDClamp(fx, 0, USHRT_MAX),
        csandDClamp(fy, 0, USHRT_MAX),
    };
}

CsandVec2Us csandPlatformGetWindowSize(void) {
    int width, height;
    glfwGetWindowSize(platform.window, &width, &height);

    return (CsandVec2Us){
        csandUiMin(width, USHRT_MAX),
        csandUiMin(height, USHRT_MAX),
    };
}

CsandVec2Us csandPlatformGetFramebufferSize(void) {
    int width, height;
    glfwGetFramebufferSize(platform.window, &width, &height);

    return (CsandVec2Us){
        csandUiMin(width, USHRT_MAX),
        csandUiMin(height, USHRT_MAX),
    };
}

void csandPlatformRun(void) {
    while (!glfwWindowShouldClose(platform.window)) {
        glfwPollEvents();

        if (platform.render_callback) {
            platform.render_callback();
        }

        glfwSwapBuffers(platform.window);
    }
}

void csandPlatformPrintErr(const char *str) {
    fwrite(str, 1, strlen(str), stderr);
}
