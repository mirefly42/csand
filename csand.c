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
    MAT_FIRE_GAS,
    MAT_FIRE_POWDER,
    MAT_FIRE_LIQUID,
    MAT_SMOKE,
    MAT_WOOD,
    MAT_COAL,
    MAT_OIL,
    MAT_HYDROGEN_GAS,
    MAT_HYDROGEN_LIQUID,
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
    uint16_t decay_prob;
    uint16_t ignition_prob;
    unsigned char decay_mat;
} CsandMaterialProperties;

/* Maps probability from 0-1 float to 0-65535 uint16_t */
#define NPROB(x) ((uint16_t)((x) * 0xFFFF))

static const CsandMaterialProperties materials[MATERIALS_COUNT] = {
    [MAT_AIR]             = {"air",             1000,    MAT_KIND_FLUID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_WALL]            = {"wall",            2500000, MAT_KIND_SOLID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_SAND]            = {"sand",            1500000, MAT_KIND_POWDER, NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_WATER]           = {"water",           1000000, MAT_KIND_FLUID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_FIRE_GAS]        = {"fire gas",        50,      MAT_KIND_FLUID,  NPROB(0.1),   NPROB(0),    MAT_AIR},
    [MAT_FIRE_POWDER]     = {"fire powder",     600000,  MAT_KIND_POWDER, NPROB(0.05),  NPROB(0),    MAT_SMOKE},
    [MAT_FIRE_LIQUID]     = {"fire liquid",     50000,   MAT_KIND_FLUID,  NPROB(0.06),  NPROB(0),    MAT_AIR},
    [MAT_SMOKE]           = {"smoke",           750,     MAT_KIND_FLUID,  NPROB(0.002), NPROB(0),    MAT_AIR},
    [MAT_WOOD]            = {"wood",            900000,  MAT_KIND_SOLID,  NPROB(0),     NPROB(0.50), MAT_AIR},
    [MAT_COAL]            = {"coal",            1500000, MAT_KIND_POWDER, NPROB(0),     NPROB(0.45), MAT_AIR},
    [MAT_OIL]             = {"oil",             750000,  MAT_KIND_FLUID,  NPROB(0),     NPROB(0.4),  MAT_AIR},
    [MAT_HYDROGEN_GAS]    = {"hydrogen gas",    100,     MAT_KIND_FLUID,  NPROB(0),     NPROB(1),    MAT_AIR},
    [MAT_HYDROGEN_LIQUID] = {"hydrogen liquid", 70800,   MAT_KIND_FLUID,  3,            NPROB(0.25), MAT_HYDROGEN_GAS},
};

static const CsandRgba palette[MATERIALS_COUNT] = {
    [MAT_AIR]             = {0x00, 0x00, 0x00, 0xFF},
    [MAT_WALL]            = {0xFF, 0x00, 0xFF, 0xFF},
    [MAT_SAND]            = {0xFF, 0xFF, 0x00, 0xFF},
    [MAT_WATER]           = {0x00, 0x00, 0xFF, 0xFF},
    [MAT_FIRE_GAS]        = {0xFF, 0x79, 0x00, 0xFF},
    [MAT_FIRE_POWDER]     = {0xFF, 0x65, 0x00, 0xFF},
    [MAT_FIRE_LIQUID]     = {0xFF, 0x70, 0x00, 0xFF},
    [MAT_SMOKE]           = {0x4B, 0x4B, 0x4B, 0xFF},
    [MAT_WOOD]            = {0xDC, 0xC8, 0x7D, 0xFF},
    [MAT_COAL]            = {0x2D, 0x2D, 0x2D, 0xFF},
    [MAT_OIL]             = {0x3B, 0x36, 0x1F, 0xFF},
    [MAT_HYDROGEN_GAS]    = {0x4C, 0x78, 0xA5, 0xFF},
    [MAT_HYDROGEN_LIQUID] = {0x57, 0x8F, 0xC8, 0xFF},
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
static inline bool csandChance(uint16_t prob);
static inline bool csandMatIsFire(unsigned char mat);
static void tryIgnite(CsandRenderBuffer buf, unsigned int x, unsigned int y);

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
        case CSAND_INPUT_SELECT_MAT4:
            draw_mat = MAT_FIRE_GAS;
            break;
        case CSAND_INPUT_SELECT_MAT5:
            draw_mat = MAT_WOOD;
            break;
        case CSAND_INPUT_SELECT_MAT6:
            draw_mat = MAT_COAL;
            break;
        case CSAND_INPUT_SELECT_MAT7:
            draw_mat = MAT_OIL;
            break;
        case CSAND_INPUT_SELECT_MAT8:
            draw_mat = MAT_HYDROGEN_GAS;
            break;
        case CSAND_INPUT_SELECT_MAT9:
            draw_mat = MAT_HYDROGEN_LIQUID;
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

            if (csandChance(mat_props.decay_prob)) {
                *csandGetMat(buf, x, y) = mat_props.decay_mat | MAT_UPDATED_BIT;
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

            if (csandMatIsFire(mat)) {
                tryIgnite(buf, sx, sy);
            } else if (csandMatIsFire(swap_mat)) {
                tryIgnite(buf, x, y);
            }

            if (mat_props.kind != MAT_KIND_SOLID && swap_mat_props.kind != MAT_KIND_SOLID && swap_mat_props.density < mat_props.density) {
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

static inline bool csandChance(uint16_t prob) {
    for (;;) {
        uint16_t x = csandRand();
        if (x < 0xFFFF) {
            return x < prob;
        }
    }
}

static inline bool csandMatIsFire(unsigned char mat) {
    return mat == MAT_FIRE_GAS || mat == MAT_FIRE_POWDER || mat == MAT_FIRE_LIQUID;
}

static void tryIgnite(CsandRenderBuffer buf, unsigned int x, unsigned int y) {
    unsigned char mat = *csandGetMat(buf, x, y);
    CsandMaterialProperties mat_props = materials[mat];

    if (!csandChance(mat_props.ignition_prob)) {
        return;
    }

    for (int dy = 2; dy >= -2; dy--) {
        for (int dx = -2; dx <= 2; dx++) {
            if (!(dx == 0 && dy == 0) && csandInBounds(buf, x + dx, y + dy) && *csandGetMat(buf, x + dx, y + dy) == MAT_AIR) {
                switch (mat_props.kind) {
                    case MAT_KIND_SOLID:
                    case MAT_KIND_POWDER:
                        mat = csandRand() & 1 ? MAT_FIRE_GAS : MAT_FIRE_POWDER;
                        break;
                    case MAT_KIND_FLUID:
                        mat = csandRand() & 1 ? MAT_FIRE_GAS : MAT_FIRE_LIQUID;
                        break;
                }

                *csandGetMat(buf, x, y) = mat;
                return;
            }
        }
    }
}
