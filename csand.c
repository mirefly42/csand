#include "platform.h"
#include "rgba.h"
#include <stdbool.h>
#include <stdint.h>

#define SPEED_LIMIT 128

enum {
    MAT_AIR,
    MAT_WALL,
    MAT_SAND,
    MAT_WATER,
    MATERIALS_COUNT,
    MAT_UPDATED_BIT = 0x80
};

static unsigned int pause = 0;
static unsigned long speed = 1;
static unsigned char draw_mat = MAT_SAND;

typedef enum {
    MAT_KIND_SOLID,
    MAT_KIND_POWDER,
    MAT_KIND_FLUID,
} CsandMaterialKind;

typedef struct CsandMaterialProperties {
    const char *name;
    uint32_t density;
    CsandMaterialKind kind;
} CsandMaterialProperties;

static const CsandMaterialProperties materials[MATERIALS_COUNT] = {
    [MAT_AIR] = {"air", 1000, MAT_KIND_FLUID},
    [MAT_WALL] = {"wall", 2500000, MAT_KIND_SOLID},
    [MAT_SAND] = {"sand", 1500000, MAT_KIND_POWDER},
    [MAT_WATER] = {"water", 1000000, MAT_KIND_FLUID},
};

static const CsandRgba palette[MATERIALS_COUNT] = {
    [MAT_AIR] = {0x00, 0x00, 0x00, 0xFF},
    [MAT_WALL] = {0xFF, 0x00, 0xFF, 0xFF},
    [MAT_SAND] = {0xFF, 0xFF, 0x00, 0xFF},
    [MAT_WATER] = {0x00, 0x00, 0xFF, 0xFF},
};

typedef struct CsandRenderBuffer {
    unsigned char *data;
    unsigned short width;
    unsigned short height;
} CsandRenderBuffer;

static void csandInputCallback(CsandInput input);
static void csandRenderCallback(unsigned char *data, unsigned short width, unsigned short height);
static void csandSimulate(CsandRenderBuffer buf);
static inline unsigned char *csandGetMat(CsandRenderBuffer buf, unsigned int x, unsigned int y);
static uint32_t csandRand(void);
static bool csandInBounds(CsandRenderBuffer buf, int x, int y);

int main(void) {
    csandPlatformInit();
    csandPlatformSetPalette(palette, MATERIALS_COUNT);
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
            draw_mat = MAT_WALL;
            break;
        case CSAND_INPUT_SELECT_MAT2:
            draw_mat = MAT_SAND;
            break;
        case CSAND_INPUT_SELECT_MAT3:
            draw_mat = MAT_WATER;
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

            if (mat & MAT_UPDATED_BIT) {
                continue;
            }
            CsandMaterialProperties mat_props = materials[mat];

            if (mat_props.kind == MAT_KIND_SOLID) {
                continue;
            }

            int dx = csandRand() % 3 - 1;
            int dy = mat_props.kind == MAT_KIND_POWDER ? -1 : -(csandRand() & 1);

            if (!csandInBounds(buf, x + dx, y + dy)) {
                continue;
            }

            unsigned int sx = x + dx;
            unsigned int sy = y + dy;
            unsigned char swap_mat = *csandGetMat(buf, sx, sy);
            if (swap_mat & MAT_UPDATED_BIT) {
                continue;
            }

            CsandMaterialProperties swap_mat_props = materials[swap_mat];

            if (swap_mat_props.kind != MAT_KIND_SOLID && swap_mat_props.density < mat_props.density) {
                *csandGetMat(buf, x, y) = swap_mat;
                *csandGetMat(buf, sx, sy) = mat | MAT_UPDATED_BIT;
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

static uint32_t csandRand(void)
{
    static uint64_t seed = 1;
    seed = 6364136223846793005*seed + 1;
    return seed >> 32;
}

static bool csandInBounds(CsandRenderBuffer buf, int x, int y) {
    return x >= 0 && x < buf.width && y >= 0 && y < buf.height;
}

