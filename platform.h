#ifndef CSAND_PLATFORM_H
#define CSAND_PLATFORM_H

#include "vec2.h"
#include <stdint.h>

typedef enum {
    CSAND_KEY_SPACE = 32,
    CSAND_KEY_APOSTROPHE = 39,
    CSAND_KEY_COMMA = 44,
    CSAND_KEY_MINUS = 45,
    CSAND_KEY_PERIOD = 46,
    CSAND_KEY_SLASH = 47,
    CSAND_KEY_0 = 48,
    CSAND_KEY_1 = 49,
    CSAND_KEY_2 = 50,
    CSAND_KEY_3 = 51,
    CSAND_KEY_4 = 52,
    CSAND_KEY_5 = 53,
    CSAND_KEY_6 = 54,
    CSAND_KEY_7 = 55,
    CSAND_KEY_8 = 56,
    CSAND_KEY_9 = 57,
    CSAND_KEY_SEMICOLON = 59,
    CSAND_KEY_EQUAL = 61,
    CSAND_KEY_A = 65,
    CSAND_KEY_B = 66,
    CSAND_KEY_C = 67,
    CSAND_KEY_D = 68,
    CSAND_KEY_E = 69,
    CSAND_KEY_F = 70,
    CSAND_KEY_G = 71,
    CSAND_KEY_H = 72,
    CSAND_KEY_I = 73,
    CSAND_KEY_J = 74,
    CSAND_KEY_K = 75,
    CSAND_KEY_L = 76,
    CSAND_KEY_M = 77,
    CSAND_KEY_N = 78,
    CSAND_KEY_O = 79,
    CSAND_KEY_P = 80,
    CSAND_KEY_Q = 81,
    CSAND_KEY_R = 82,
    CSAND_KEY_S = 83,
    CSAND_KEY_T = 84,
    CSAND_KEY_U = 85,
    CSAND_KEY_V = 86,
    CSAND_KEY_W = 87,
    CSAND_KEY_X = 88,
    CSAND_KEY_Y = 89,
    CSAND_KEY_Z = 90,
    CSAND_KEY_LEFT_BRACKET = 91,
    CSAND_KEY_BACKSLASH = 92,
    CSAND_KEY_RIGHT_BRACKET = 93,
    CSAND_KEY_GRAVE_ACCENT = 96,
    CSAND_KEY_WORLD_1 = 161,
    CSAND_KEY_WORLD_2 = 162,
    CSAND_KEY_ESCAPE = 256,
    CSAND_KEY_ENTER = 257,
    CSAND_KEY_TAB = 258,
    CSAND_KEY_BACKSPACE = 259,
    CSAND_KEY_INSERT = 260,
    CSAND_KEY_DELETE = 261,
    CSAND_KEY_RIGHT = 262,
    CSAND_KEY_LEFT = 263,
    CSAND_KEY_DOWN = 264,
    CSAND_KEY_UP = 265,
    CSAND_KEY_PAGE_UP = 266,
    CSAND_KEY_PAGE_DOWN = 267,
    CSAND_KEY_HOME = 268,
    CSAND_KEY_END = 269,
    CSAND_KEY_CAPS_LOCK = 280,
    CSAND_KEY_SCROLL_LOCK = 281,
    CSAND_KEY_NUM_LOCK = 282,
    CSAND_KEY_PRINT_SCREEN = 283,
    CSAND_KEY_PAUSE = 284,
    CSAND_KEY_F1 = 290,
    CSAND_KEY_F2 = 291,
    CSAND_KEY_F3 = 292,
    CSAND_KEY_F4 = 293,
    CSAND_KEY_F5 = 294,
    CSAND_KEY_F6 = 295,
    CSAND_KEY_F7 = 296,
    CSAND_KEY_F8 = 297,
    CSAND_KEY_F9 = 298,
    CSAND_KEY_F10 = 299,
    CSAND_KEY_F11 = 300,
    CSAND_KEY_F12 = 301,
    CSAND_KEY_F13 = 302,
    CSAND_KEY_F14 = 303,
    CSAND_KEY_F15 = 304,
    CSAND_KEY_F16 = 305,
    CSAND_KEY_F17 = 306,
    CSAND_KEY_F18 = 307,
    CSAND_KEY_F19 = 308,
    CSAND_KEY_F20 = 309,
    CSAND_KEY_F21 = 310,
    CSAND_KEY_F22 = 311,
    CSAND_KEY_F23 = 312,
    CSAND_KEY_F24 = 313,
    CSAND_KEY_F25 = 314,
    CSAND_KEY_KP_0 = 320,
    CSAND_KEY_KP_1 = 321,
    CSAND_KEY_KP_2 = 322,
    CSAND_KEY_KP_3 = 323,
    CSAND_KEY_KP_4 = 324,
    CSAND_KEY_KP_5 = 325,
    CSAND_KEY_KP_6 = 326,
    CSAND_KEY_KP_7 = 327,
    CSAND_KEY_KP_8 = 328,
    CSAND_KEY_KP_9 = 329,
    CSAND_KEY_KP_DECIMAL = 330,
    CSAND_KEY_KP_DIVIDE = 331,
    CSAND_KEY_KP_MULTIPLY = 332,
    CSAND_KEY_KP_SUBTRACT = 333,
    CSAND_KEY_KP_ADD = 334,
    CSAND_KEY_KP_ENTER = 335,
    CSAND_KEY_KP_EQUAL = 336,
    CSAND_KEY_LEFT_SHIFT = 340,
    CSAND_KEY_LEFT_CONTROL = 341,
    CSAND_KEY_LEFT_ALT = 342,
    CSAND_KEY_LEFT_SUPER = 343,
    CSAND_KEY_RIGHT_SHIFT = 344,
    CSAND_KEY_RIGHT_CONTROL = 345,
    CSAND_KEY_RIGHT_ALT = 346,
    CSAND_KEY_RIGHT_SUPER = 347,
    CSAND_KEY_MENU = 348,
} CsandKey;

typedef enum {
    CSAND_MOD_NONE,
    CSAND_MOD_SHIFT = 0x1,
    CSAND_MOD_CONTROL = 0x2,
    CSAND_MOD_ALT = 0x4,
} CsandMod;

typedef CsandMod CsandModSet;

typedef enum {
    CSAND_ACTION_RELEASE,
    CSAND_ACTION_PRESS,
} CsandAction;

typedef enum {
    CSAND_MOUSE_BUTTON_LEFT,
    CSAND_MOUSE_BUTTON_RIGHT,
} CsandMouseButton;

typedef void (*CsandRenderCallback)(double time);
typedef void (*CsandKeyCallback)(CsandKey key, CsandAction action, CsandModSet mods);
typedef void (*CsandCharCallback)(uint32_t codepoint);
typedef void (*CsandMouseButtonCallback)(CsandMouseButton button, unsigned int pressed);
typedef void (*CsandMouseMotionCallback)(double x, double y);
typedef void (*CsandMouseScrollCallback)(double x, double y);
typedef void (*CsandFramebufferSizeCallback)(CsandVec2Us size);

void csandPlatformInit(void);
void csandPlatformSetRenderCallback(CsandRenderCallback callback);
void csandPlatformSetKeyCallback(CsandKeyCallback callback);
void csandPlatformSetCharCallback(CsandCharCallback callback);
void csandPlatformSetMouseButtonCallback(CsandMouseButtonCallback callback);
void csandPlatformSetMouseMotionCallback(CsandMouseMotionCallback callback);
void csandPlatformSetMouseScrollCallback(CsandMouseScrollCallback callback);
void csandPlatformSetFramebufferSizeCallback(CsandFramebufferSizeCallback callback);
unsigned int csandPlatformIsMouseButtonPressed(CsandMouseButton button);
CsandVec2Us csandPlatformGetCursorPos(void);
CsandVec2Us csandPlatformGetWindowSize(void);
CsandVec2Us csandPlatformGetFramebufferSize(void);
void csandPlatformRun(void);
void csandPlatformPrintErr(const char *str);
void csandPlatformToggleFullscreen(void);

#endif
