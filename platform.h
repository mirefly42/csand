#ifndef CSAND_PLATFORM_H
#define CSAND_PLATFORM_H

#include "rgba.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    CSAND_INPUT_SELECT_MAT0,
    CSAND_INPUT_SELECT_MAT1,
    CSAND_INPUT_SELECT_MAT2,
    CSAND_INPUT_SELECT_MAT3,
    CSAND_INPUT_PAUSE_TOGGLE,
    CSAND_INPUT_SPEED_INCREASE,
    CSAND_INPUT_SPEED_DECREASE,
} CsandInput;

typedef enum {
    CSAND_MOUSE_BUTTON_LEFT,
    CSAND_MOUSE_BUTTON_RIGHT,
} CsandMouseButton;

typedef void (*CsandRenderCallback)(unsigned char *data, unsigned short width, unsigned short height);
typedef void (*CsandInputCallback)(CsandInput input);

void csandPlatformInit(void);
void csandPlatformSetPalette(const CsandRgba *colors, uint8_t colors_count);
void csandPlatformSetRenderCallback(CsandRenderCallback callback);
void csandPlatformSetInputCallback(CsandInputCallback callback);
unsigned int csandPlatformIsMouseButtonPressed(CsandMouseButton button);
void csandPlatformGetCursorPos(unsigned short *x, unsigned short *y);
void csandPlatformRun(void);

#endif
