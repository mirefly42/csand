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
    CsandKeyCallback key_callback;
    CsandCharCallback char_callback;
    CsandMouseButtonCallback mouse_button_callback;
    CsandMouseMotionCallback mouse_motion_callback;
    CsandMouseScrollCallback mouse_scroll_callback;
    CsandFramebufferSizeCallback framebuffer_size_callback;
    CsandVec2I windowed_mode_pos;
    CsandVec2I windowed_mode_size;
} CsandPlatform;

static CsandPlatform platform = {0};

static void csandGlfwErrorCallback(int code, const char *msg) {
    fprintf(stderr, "glfw error %d: %s\n", code, msg);
}

void csandPlatformToggleFullscreen(void) {
    if (glfwGetWindowMonitor(platform.window)) {
        glfwSetWindowMonitor(
            platform.window,
            NULL,
            platform.windowed_mode_pos.x,
            platform.windowed_mode_pos.y,
            platform.windowed_mode_size.x,
            platform.windowed_mode_size.y,
            GLFW_DONT_CARE
        );
    } else {
        glfwGetWindowPos(
            platform.window,
            &platform.windowed_mode_pos.x,
            &platform.windowed_mode_pos.y
        );

        glfwGetWindowSize(
            platform.window,
            &platform.windowed_mode_size.x,
            &platform.windowed_mode_size.y
        );

        GLFWmonitor *mon = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(
            platform.window,
            mon,
            0, 0,
            mode->width, mode->height,
            GLFW_DONT_CARE
        );
    }
}

static void csandGlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (!platform.key_callback) {
        return;
    }

    switch (action) {
        case GLFW_RELEASE:
        case GLFW_PRESS:
            platform.key_callback(key, action, mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT));
            break;
    }
}

static void csandGlfwCharCallback(GLFWwindow *window, unsigned int codepoint) {
    (void)window;
    if (platform.char_callback) {
        platform.char_callback(codepoint);
    }
}

static void csandGlfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    (void)window;
    (void)mods;

    if (
        platform.mouse_button_callback &&
        (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) &&
        (action == GLFW_RELEASE || action == GLFW_PRESS)
    ) {
        platform.mouse_button_callback(button, action);
    }
}

static void csandGlfwCursorPosCallback(GLFWwindow *window, double x, double y) {
    (void)window;

    if (platform.mouse_motion_callback) {
        platform.mouse_motion_callback(x, y);
    }
}

static void csandGlfwScrollCallback(GLFWwindow *window, double x, double y) {
    (void)window;

    if (platform.mouse_scroll_callback) {
        platform.mouse_scroll_callback(x, y);
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

    platform.windowed_mode_size = (CsandVec2I){1024, 512};
    platform.window = glfwCreateWindow(
        platform.windowed_mode_size.x, platform.windowed_mode_size.y,
        "csand",
        NULL,
        NULL
    );

    glfwMakeContextCurrent(platform.window);

    glfwSetKeyCallback(platform.window, csandGlfwKeyCallback);
    glfwSetCharCallback(platform.window, csandGlfwCharCallback);
    glfwSetMouseButtonCallback(platform.window, csandGlfwMouseButtonCallback);
    glfwSetCursorPosCallback(platform.window, csandGlfwCursorPosCallback);
    glfwSetScrollCallback(platform.window, csandGlfwScrollCallback);
    glfwSetFramebufferSizeCallback(platform.window, csandGlfwFramebufferSizeCallback);
}

void csandPlatformSetRenderCallback(CsandRenderCallback callback) {
    platform.render_callback = callback;
}

void csandPlatformSetKeyCallback(CsandKeyCallback callback) {
    platform.key_callback = callback;
}

void csandPlatformSetCharCallback(CsandCharCallback callback) {
    platform.char_callback = callback;
}

void csandPlatformSetMouseButtonCallback(CsandMouseButtonCallback callback) {
    platform.mouse_button_callback = callback;
}

void csandPlatformSetMouseMotionCallback(CsandMouseMotionCallback callback) {
    platform.mouse_motion_callback = callback;
}

void csandPlatformSetMouseScrollCallback(CsandMouseScrollCallback callback) {
    platform.mouse_scroll_callback = callback;
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
            platform.render_callback(glfwGetTime());
        }

        glfwSwapBuffers(platform.window);
    }
}

void csandPlatformPrintErr(const char *str) {
    fwrite(str, 1, strlen(str), stderr);
}
