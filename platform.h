#ifndef CSAND_PLATFORM_H
#define CSAND_PLATFORM_H

#include "vec2.h"
#include <stdint.h>

typedef enum {
    CSAND_INPUT_SELECT_MAT0,
    CSAND_INPUT_SELECT_MAT1,
    CSAND_INPUT_SELECT_MAT2,
    CSAND_INPUT_SELECT_MAT3,
    CSAND_INPUT_SELECT_MAT4,
    CSAND_INPUT_SELECT_MAT5,
    CSAND_INPUT_SELECT_MAT6,
    CSAND_INPUT_SELECT_MAT7,
    CSAND_INPUT_SELECT_MAT8,
    CSAND_INPUT_SELECT_MAT9,
    CSAND_INPUT_PAUSE_TOGGLE,
    CSAND_INPUT_SPEED_INCREASE,
    CSAND_INPUT_SPEED_DECREASE,
    CSAND_INPUT_NEXT_FRAME,
} CsandInput;

typedef enum {
    CSAND_MOUSE_BUTTON_LEFT,
    CSAND_MOUSE_BUTTON_RIGHT,
} CsandMouseButton;

typedef void (*CsandRenderCallback)(double time);
typedef void (*CsandInputCallback)(CsandInput input);
typedef void (*CsandFramebufferSizeCallback)(CsandVec2Us size);

void csandPlatformInit(void);
void csandPlatformSetRenderCallback(CsandRenderCallback callback);
void csandPlatformSetInputCallback(CsandInputCallback callback);
void csandPlatformSetFramebufferSizeCallback(CsandFramebufferSizeCallback callback);
unsigned int csandPlatformIsMouseButtonPressed(CsandMouseButton button);
CsandVec2Us csandPlatformGetCursorPos(void);
CsandVec2Us csandPlatformGetWindowSize(void);
CsandVec2Us csandPlatformGetFramebufferSize(void);
void csandPlatformRun(void);
void csandPlatformPrintErr(const char *str);

#endif
