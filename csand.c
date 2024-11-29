#include "platform.h"
#include <stdbool.h>
#include <stdlib.h>

#define SPEED_LIMIT 128

enum {
    MAT_AIR,
    MAT_SOLID,
    MAT_SAND,
    MAT_LIQUID,
    MAT_UPDATED_BIT = 0x80
};

static unsigned int pause = 0;
static unsigned long speed = 1;
static unsigned char draw_mat = MAT_SAND;

typedef struct CsandRenderBuffer {
    unsigned char *data;
    unsigned short width;
    unsigned short height;
} CsandRenderBuffer;

static void csandInputCallback(CsandInput input);
static void csandRenderCallback(unsigned char *data, unsigned short width, unsigned short height);
static void csandSimulate(CsandRenderBuffer buf);
static inline unsigned char *csandGetMat(CsandRenderBuffer buf, unsigned int x, unsigned int y);
static bool csandInBounds(CsandRenderBuffer buf, int x, int y);

int main(void) {
    csandPlatformInit();
    csandPlatformSetInputCallback(csandInputCallback);
    csandPlatformSetRenderCallback(csandRenderCallback);
    csandPlatformRun();
}

static void csandInputCallback(CsandInput input) {
    switch (input) {
        case CSAND_INPUT_SELECT_MAT0:
            draw_mat = MAT_AIR;
            break;
        case CSAND_INPUT_SELECT_MAT1:
            draw_mat = MAT_SOLID;
            break;
        case CSAND_INPUT_SELECT_MAT2:
            draw_mat = MAT_SAND;
            break;
        case CSAND_INPUT_SELECT_MAT3:
            draw_mat = MAT_LIQUID;
            break;
        case CSAND_INPUT_PAUSE_TOGGLE:
            pause = !pause;
            break;
        case CSAND_INPUT_SPEED_INCREASE:
            if (speed < SPEED_LIMIT) {
                speed *= 2;
            }
            break;
        case CSAND_INPUT_SPEED_DECREASE:
            if (speed > 1) {
                speed /= 2;
            }
            break;
    }
}

static void csandRenderCallback(unsigned char *data, unsigned short width, unsigned short height) {
    if (!pause) {
        CsandRenderBuffer render_buf = {data, width, height};
        for (unsigned long i = 0; i < speed; i++) {
            csandSimulate(render_buf);
        }
    }
}

static void csandSimulate(CsandRenderBuffer buf) {
    for (unsigned int y = 0; y < buf.height; y++) {
        for (unsigned int x = 0; x < buf.width; x++) {
            unsigned char mat = *csandGetMat(buf, x, y);

            switch (mat) {
                case MAT_SAND:
                    if (y <= 0) {
                        continue;
                    } else {
                        int dx = rand() % 3 - 1;
                        if (!csandInBounds(buf, x + dx, y - 1)) continue;
                        unsigned char swap_mat = *csandGetMat(buf, x + dx, y - 1);

                        if (swap_mat == MAT_AIR || swap_mat == MAT_LIQUID) {
                            *csandGetMat(buf, x, y) = swap_mat;
                            *csandGetMat(buf, x + dx, y - 1) = mat | MAT_UPDATED_BIT;
                        }
                    }
                    break;
                case MAT_LIQUID: {
                    int dx = rand() % 3 - 1;
                    int dy = - (rand() % 2);
                    if (!csandInBounds(buf, x + dx, y + dy)) continue;

                    if (*csandGetMat(buf, x + dx, y + dy) == MAT_AIR) {
                        *csandGetMat(buf, x, y) = MAT_AIR;
                        *csandGetMat(buf, x + dx, y + dy) = mat | MAT_UPDATED_BIT;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (csandPlatformIsMouseButtonPressed(CSAND_MOUSE_BUTTON_LEFT)) {
        unsigned short x, y;
        csandPlatformGetCursorPos(&x, &y);
        *csandGetMat(buf, x, y) = draw_mat | MAT_UPDATED_BIT;
    }

    for (unsigned int i = 0; i < buf.width * buf.height; i++) {
        buf.data[i] &= ~MAT_UPDATED_BIT;
    }
}

static inline unsigned char *csandGetMat(CsandRenderBuffer buf, unsigned int x, unsigned int y) {
    return buf.data + buf.width * y + x;
}

static bool csandInBounds(CsandRenderBuffer buf, int x, int y) {
    return x >= 0 && x < buf.width && y >= 0 && y < buf.height;
}

