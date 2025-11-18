#include "nuklear_config.h"
#include "platform.h"
#include "random.h"
#include "renderer.h"
#include "rgba.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
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
    MAT_UPDATED_BIT = 0x80
};

#define MATERIALS_COUNT MAT_UPDATED_BIT

#define WIDTH 128
#define HEIGHT 72

#define TARGET_TICK_DELAY (1.0/60.0 - 0.001)

static double next_tick_time = 0.0;
static unsigned int pause = 0;
static unsigned long speed = 1;
static unsigned char draw_mat = MAT_SAND;
static unsigned char data[WIDTH * HEIGHT] = {0};
static bool input_next_frame = false;
static bool any_nuklear_item_active = false;
static bool developer_menu_enabled = false;
static struct nk_rect developer_menu_bounds = {10, 10, 730, 540};
static float buttons_row_width = 0;
static bool buttons_shown = true;

typedef struct CsandRenderBuffer {
    unsigned char *data;
    unsigned short width;
    unsigned short height;
} CsandRenderBuffer;

static CsandRenderBuffer render_buf = {data, WIDTH, HEIGHT};

typedef enum {
    MAT_KIND_SOLID,
    MAT_KIND_POWDER,
    MAT_KIND_FLUID,
    MAT_KINDS_COUNT,
} CsandMaterialKind;

typedef struct CsandMaterialProperties {
    const char *name;
    uint32_t density;
    CsandMaterialKind kind;
    uint16_t decay_prob;
    uint16_t ignition_prob;
    unsigned char decay_mat;
} CsandMaterialProperties;

static CsandMaterialProperties materials[MATERIALS_COUNT] = {
    [MAT_AIR]             = {"air",             1000,    MAT_KIND_FLUID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_WALL]            = {"wall",            2500000, MAT_KIND_SOLID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_SAND]            = {"sand",            1500000, MAT_KIND_POWDER, NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_WATER]           = {"water",           1000000, MAT_KIND_FLUID,  NPROB(0),     NPROB(0),    MAT_AIR},
    [MAT_FIRE_GAS]        = {"fire gas",        50,      MAT_KIND_FLUID,  NPROB(0.1),   NPROB(0),    MAT_AIR},
    [MAT_FIRE_POWDER]     = {"fire powder",     600000,  MAT_KIND_POWDER, NPROB(0.05),  NPROB(0),    MAT_SMOKE},
    [MAT_FIRE_LIQUID]     = {"fire liquid",     50000,   MAT_KIND_FLUID,  NPROB(0.06),  NPROB(0),    MAT_AIR},
    [MAT_SMOKE]           = {"smoke",           750,     MAT_KIND_FLUID,  NPROB(0.002), NPROB(0),    MAT_AIR},
    [MAT_WOOD]            = {"wood",            900000,  MAT_KIND_SOLID,  NPROB(0),     NPROB(0.5),  MAT_AIR},
    [MAT_COAL]            = {"coal",            1500000, MAT_KIND_POWDER, NPROB(0),     NPROB(0.3),  MAT_AIR},
    [MAT_OIL]             = {"oil",             750000,  MAT_KIND_FLUID,  NPROB(0),     NPROB(0.25), MAT_AIR},
    [MAT_HYDROGEN_GAS]    = {"hydrogen gas",    100,     MAT_KIND_FLUID,  NPROB(0),     NPROB(1),    MAT_AIR},
    [MAT_HYDROGEN_LIQUID] = {"hydrogen liquid", 70800,   MAT_KIND_FLUID,  3,            NPROB(0.1),  MAT_HYDROGEN_GAS},
};

static CsandRgba palette[MATERIALS_COUNT] = {
    [MAT_AIR]             = {0x00, 0x00, 0x00, 0x87},
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

static const unsigned char selectable_materials[] = {
    MAT_AIR,
    MAT_WALL,
    MAT_SAND,
    MAT_WATER,
    MAT_FIRE_GAS,
    MAT_WOOD,
    MAT_COAL,
    MAT_OIL,
    MAT_HYDROGEN_GAS,
    MAT_HYDROGEN_LIQUID,
};

#define CSAND_STATIC_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

static bool csandKeyCallback(CsandKey key, CsandAction action, CsandModSet mods);
static void csandCharCallback(uint32_t codepoint);
static void csandMouseButtonCallback(CsandMouseButton button, unsigned int pressed) {
    CsandVec2Us pos = csandPlatformGetCursorPos();
    struct nk_context *nk_ctx = csandRendererNuklearContext();
    switch (button) {
        case CSAND_MOUSE_BUTTON_LEFT:
            nk_input_button(nk_ctx, NK_BUTTON_LEFT, pos.x, pos.y, pressed);
            break;
        case CSAND_MOUSE_BUTTON_RIGHT:
            nk_input_button(nk_ctx, NK_BUTTON_RIGHT, pos.x, pos.y, pressed);
            break;
    }
}

static void csandMouseMotionCallback(double x, double y) {
    nk_input_motion(csandRendererNuklearContext(), x, y);
}

static void csandMouseScrollCallback(double x, double y) {
    nk_input_scroll(csandRendererNuklearContext(), nk_vec2(x, y));
}

static void csandRenderCallback(double time);
static void csandSimulate(CsandRenderBuffer buf);
static inline unsigned char *csandGetMat(CsandRenderBuffer buf, unsigned int x, unsigned int y);
static bool csandInBounds(CsandRenderBuffer buf, int x, int y);
static inline bool csandMatIsFire(unsigned char mat);
static void tryIgnite(CsandRenderBuffer buf, unsigned int x, unsigned int y);

static void csandDoubleSimulationSpeed(void) {
    if (speed < SPEED_LIMIT) {
        speed *= 2;
    }
}

static void csandHalveSimulationSpeed(void) {
    if (speed > 1) {
        speed /= 2;
    }
}

static void csandSimulateSingleFrame(void) {
    input_next_frame = true;
    pause = true;
}

int main(void) {
    csandPlatformInit();
    csandRendererInit((CsandVec2Us){WIDTH, HEIGHT}, csandPlatformGetFramebufferSize(), palette, MATERIALS_COUNT);
    csandRendererSetGlow(true);
    csandPlatformSetKeyCallback(csandKeyCallback);
    csandPlatformSetCharCallback(csandCharCallback);
    csandPlatformSetMouseButtonCallback(csandMouseButtonCallback);
    csandPlatformSetMouseMotionCallback(csandMouseMotionCallback);
    csandPlatformSetMouseScrollCallback(csandMouseScrollCallback);
    csandPlatformSetRenderCallback(csandRenderCallback);
    csandPlatformSetFramebufferSizeCallback(csandRendererUpdateViewport);
    nk_input_begin(csandRendererNuklearContext());
    csandPlatformRun();
}

static void csandSendKeyStateToNuklear(CsandKey key, CsandAction action, CsandModSet mods) {
    struct nk_context *nk_ctx = csandRendererNuklearContext();
    switch (key) {
        case CSAND_KEY_LEFT_SHIFT:
        case CSAND_KEY_RIGHT_SHIFT:
            nk_input_key(nk_ctx, NK_KEY_SHIFT, action);
            break;
        case CSAND_KEY_LEFT_CONTROL:
        case CSAND_KEY_RIGHT_CONTROL:
            nk_input_key(nk_ctx, NK_KEY_CTRL, action);
            break;
        case CSAND_KEY_DELETE:
            nk_input_key(nk_ctx, NK_KEY_DEL, action);
            break;
        case CSAND_KEY_ENTER:
            nk_input_key(nk_ctx, NK_KEY_ENTER, action);
            break;
        case CSAND_KEY_TAB:
            nk_input_key(nk_ctx, NK_KEY_TAB, action);
            break;
        case CSAND_KEY_BACKSPACE:
            nk_input_key(nk_ctx, NK_KEY_BACKSPACE, action);
            break;
        case CSAND_KEY_C:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_COPY, action);
            }
            break;
        case CSAND_KEY_X:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_CUT, action);
            }
            break;
        case CSAND_KEY_V:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_PASTE, action);
            }
            break;
        case CSAND_KEY_UP:
            nk_input_key(nk_ctx, NK_KEY_UP, action);
            break;
        case CSAND_KEY_DOWN:
            nk_input_key(nk_ctx, NK_KEY_DOWN, action);
            break;
        case CSAND_KEY_LEFT:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_WORD_LEFT, action);
            } else {
                nk_input_key(nk_ctx, NK_KEY_LEFT, action);
            }
            break;
        case CSAND_KEY_RIGHT:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_WORD_RIGHT, action);
            } else {
                nk_input_key(nk_ctx, NK_KEY_RIGHT, action);
            }
            break;
        case CSAND_KEY_B:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_LINE_START, action);
            }
            break;
        case CSAND_KEY_E:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_LINE_END, action);
            }
            break;
        case CSAND_KEY_HOME:
            nk_input_key(nk_ctx, NK_KEY_TEXT_START, action);
            nk_input_key(nk_ctx, NK_KEY_SCROLL_START, action);
            break;
        case CSAND_KEY_END:
            nk_input_key(nk_ctx, NK_KEY_TEXT_END, action);
            nk_input_key(nk_ctx, NK_KEY_SCROLL_END, action);
            break;
        case CSAND_KEY_Z:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_UNDO, action);
            }
            break;
        case CSAND_KEY_R:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_REDO, action);
            }
            break;
        case CSAND_KEY_A:
            if (mods == CSAND_MOD_CONTROL) {
                nk_input_key(nk_ctx, NK_KEY_TEXT_SELECT_ALL, action);
            }
        default:
            break;
    }
}

static bool csandKeyCallback(CsandKey key, CsandAction action, CsandModSet mods) {
    csandSendKeyStateToNuklear(key, action, mods);

    if (action != CSAND_ACTION_PRESS || any_nuklear_item_active) return false;

    switch (key) {
        case CSAND_KEY_0:
            draw_mat = MAT_AIR;
            return true;
        case CSAND_KEY_1:
            draw_mat = MAT_WALL;
            return true;
        case CSAND_KEY_2:
            draw_mat = MAT_SAND;
            return true;
        case CSAND_KEY_3:
            draw_mat = MAT_WATER;
            return true;
        case CSAND_KEY_4:
            draw_mat = MAT_FIRE_GAS;
            return true;
        case CSAND_KEY_5:
            draw_mat = MAT_WOOD;
            return true;
        case CSAND_KEY_6:
            draw_mat = MAT_COAL;
            return true;
        case CSAND_KEY_7:
            draw_mat = MAT_OIL;
            return true;
        case CSAND_KEY_8:
            draw_mat = MAT_HYDROGEN_GAS;
            return true;
        case CSAND_KEY_9:
            draw_mat = MAT_HYDROGEN_LIQUID;
            return true;
        case CSAND_KEY_SPACE:
            pause = !pause;
            return true;
        case CSAND_KEY_G:
            csandRendererSetGlow(!csandRendererGetGlow());
            return true;
        case CSAND_KEY_EQUAL:
            csandDoubleSimulationSpeed();
            return true;
        case CSAND_KEY_MINUS:
            csandHalveSimulationSpeed();
            return true;
        case CSAND_KEY_PERIOD:
            csandSimulateSingleFrame();
            return true;
        case CSAND_KEY_F:
            csandPlatformToggleFullscreen();
            return true;
        case CSAND_KEY_TAB:
            developer_menu_enabled = !developer_menu_enabled;
            return true;
        default:
            break;
    }

    return false;
}

static void csandCharCallback(uint32_t codepoint) {
    nk_input_unicode(csandRendererNuklearContext(), codepoint);
}

static void csandDrawDeveloperMenu(void) {
    struct nk_context *nk_ctx = csandRendererNuklearContext();

    nk_flags win_flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
        NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;

    if (nk_begin(nk_ctx, "Developer menu", developer_menu_bounds, win_flags)) {
        developer_menu_bounds = nk_window_get_bounds(nk_ctx);

        float id_width = 25;
        float name_width = 120;
        float density_width = 90;
        float other_width = 80;
        float row_height = 25;
        int cols = 8;
        nk_flags align = NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE;
        nk_layout_row_begin(nk_ctx, NK_STATIC, row_height, cols);
        {
            nk_layout_row_push(nk_ctx, id_width);
            nk_label(nk_ctx, "ID", align);
            nk_layout_row_push(nk_ctx, name_width);
            nk_label(nk_ctx, "COLOR", align);
            nk_label(nk_ctx, "MATERIAL", align);
            nk_layout_row_push(nk_ctx, density_width);
            nk_label(nk_ctx, "DENSITY", align);
            nk_layout_row_push(nk_ctx, other_width);
            nk_label(nk_ctx, "KIND", align);
            nk_label(nk_ctx, "DECAY PROBABILITY", align);
            nk_label(nk_ctx, "IGNITION PROBABILITY", align);
            nk_label(nk_ctx, "DECAY MATERIAL", align);
        }
        nk_layout_row_end(nk_ctx);

        bool palette_changed = false;

        for (unsigned int i = 0; i < MATERIALS_COUNT; i++) {
            CsandMaterialProperties *props = &materials[i];

            nk_layout_row_begin(nk_ctx, NK_STATIC, row_height, cols);

            nk_layout_row_push(nk_ctx, id_width);
            nk_labelf(nk_ctx, align, "%u", i);

            nk_layout_row_push(nk_ctx, name_width);

            CsandRgba *color = palette + i;
            if (nk_combo_begin_color(nk_ctx, (struct nk_color){color->r, color->g, color->b, 0xFF}, nk_vec2(300, 300))) {
                nk_layout_row_dynamic(nk_ctx, row_height, 4);
                color->r = nk_propertyi(nk_ctx, "#R", 0x00, color->r, 0xFF, 1, 1);
                color->g = nk_propertyi(nk_ctx, "#G", 0x00, color->g, 0xFF, 1, 1);
                color->b = nk_propertyi(nk_ctx, "#B", 0x00, color->b, 0xFF, 1, 1);
                color->a = nk_propertyi(nk_ctx, "#A", 0x00, color->a, 0xFF, 1, 1);
                palette_changed = true;
                nk_combo_end(nk_ctx);
            }

            const char *name = props->name;
            if (name == NULL) {
                name = "unnamed";
            }

            if (nk_button_label(nk_ctx, name)) {
                draw_mat = i;
            }

            nk_layout_row_push(nk_ctx, density_width);
            props->density = nk_propertyi(nk_ctx, "##density", 0, props->density, INT_MAX, 25, 500);

            nk_layout_row_push(nk_ctx, other_width);
            static const char *kinds[] = {
                [MAT_KIND_SOLID] = "solid",
                [MAT_KIND_POWDER] = "powder",
                [MAT_KIND_FLUID] = "fluid"
            };

            props->kind = nk_combo(nk_ctx, kinds, sizeof(kinds) / sizeof(kinds[0]), props->kind, row_height, nk_vec2(other_width, row_height*(MAT_KINDS_COUNT + 1)));
            props->decay_prob = nk_propertyi(nk_ctx, "##decay_prob", 0, props->decay_prob, UINT16_MAX, 1, 1);
            props->ignition_prob = nk_propertyi(nk_ctx, "##ignition_prob", 0, props->ignition_prob, UINT16_MAX, 1, 1);
            props->decay_mat = nk_propertyi(nk_ctx, "##decay_mat", 0, props->decay_mat, MATERIALS_COUNT - 1, 1, 0.5);

            nk_layout_row_end(nk_ctx);
        }

        if (palette_changed) {
            csandRendererSetPalette(palette, MATERIALS_COUNT);
        }
    }
    nk_end(nk_ctx);
}

static bool csandRowTemplateAutoSizedButton(struct nk_context *nk_ctx, const char *text) {
    const struct nk_user_font *font = nk_ctx->style.font;
    float width = font->width(font->userdata, font->height, text, nk_strlen(text)) + 18;
    nk_layout_row_template_push_static(nk_ctx, width);
    return nk_button_label(nk_ctx, text);
}

static void csandDrawButtons(void) {
    struct nk_context *nk_ctx = csandRendererNuklearContext();

    float row_height = 40;
    nk_style_push_style_item(nk_ctx, &nk_ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));
    if (nk_begin(nk_ctx, "Buttons", nk_rect(0, 0, csandFMin(buttons_row_width, csandPlatformGetFramebufferSize().x) + 14, row_height + 18), NK_WINDOW_BACKGROUND)) {
        nk_layout_row_template_begin(nk_ctx, row_height);

        if (csandRowTemplateAutoSizedButton(nk_ctx, buttons_shown ? "<" : ">")) {
            buttons_shown = !buttons_shown;
        }

        if (buttons_shown) {
            for (size_t i = 0; i < CSAND_STATIC_ARRAY_LENGTH(selectable_materials); i++) {
                unsigned char mat = selectable_materials[i];
                if (csandRowTemplateAutoSizedButton(nk_ctx, materials[mat].name)) {
                    draw_mat = mat;
                }
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "fullscreen")) {
                csandPlatformToggleFullscreen();
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "pause")) {
                pause = !pause;
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "speed+")) {
                csandDoubleSimulationSpeed();
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "speed-")) {
                csandHalveSimulationSpeed();
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "next frame")) {
                csandSimulateSingleFrame();
            }

            if (csandRowTemplateAutoSizedButton(nk_ctx, "dev menu")) {
                developer_menu_enabled = !developer_menu_enabled;
            }
        }
        struct nk_panel *panel = nk_window_get_panel(nk_ctx);
        buttons_row_width = panel->max_x;
        nk_layout_row_template_end(nk_ctx);
    }
    nk_end(nk_ctx);
    nk_style_pop_style_item(nk_ctx);
}

static void csandRenderCallback(double time) {
    struct nk_context *nk_ctx = csandRendererNuklearContext();
    nk_input_end(nk_ctx);

    csandDrawButtons();

    if (developer_menu_enabled) {
        csandDrawDeveloperMenu();
    }

    any_nuklear_item_active = nk_item_is_any_active(nk_ctx);

    bool draw = !any_nuklear_item_active && csandPlatformIsMouseButtonPressed(CSAND_MOUSE_BUTTON_LEFT);
    CsandVec2Us cur_pos = csandRendererScreenSpaceToWorldSpace(csandPlatformGetCursorPos());

    if (time >= next_tick_time && (!pause || input_next_frame)) {
        input_next_frame = false;
        for (unsigned long i = 0; i < speed; i++) {
            csandSimulate(render_buf);
            if (draw) {
                *csandGetMat(render_buf, cur_pos.x, cur_pos.y) = draw_mat;
            }
        }
        next_tick_time = time + TARGET_TICK_DELAY;
    } else if (draw) {
        *csandGetMat(render_buf, cur_pos.x, cur_pos.y) = draw_mat;
    }

    csandRendererRender(data, WIDTH, HEIGHT);
    nk_clear(nk_ctx);

    nk_input_begin(nk_ctx);
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
                swap_mat = *csandGetMat(buf, sx, sy) & (~MAT_UPDATED_BIT);
                swap_mat_props = materials[swap_mat];
            } else if (csandMatIsFire(swap_mat)) {
                tryIgnite(buf, x, y);
                mat = *csandGetMat(buf, x, y) & (~MAT_UPDATED_BIT);
                mat_props = materials[mat];
            }

            if (mat_props.kind != MAT_KIND_SOLID && swap_mat_props.kind != MAT_KIND_SOLID && swap_mat_props.density < mat_props.density) {
                *csandGetMat(buf, x, y) = swap_mat;
                *csandGetMat(buf, sx, sy) = mat | MAT_UPDATED_BIT;
            }
        }
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

static inline bool csandMatIsFire(unsigned char mat) {
    return mat == MAT_FIRE_GAS || mat == MAT_FIRE_POWDER || mat == MAT_FIRE_LIQUID;
}

static void tryIgnite(CsandRenderBuffer buf, unsigned int x, unsigned int y) {
    unsigned char mat = *csandGetMat(buf, x, y);
    CsandMaterialProperties mat_props = materials[mat];

    if (!csandChance(mat_props.ignition_prob)) {
        return;
    }

    const int air_range = 2;
    for (int dy = air_range; dy >= -air_range; dy--) {
        for (int dx = -air_range; dx <= air_range; dx++) {
            if (!(dx == 0 && dy == 0) && csandInBounds(buf, x + dx, y + dy) && *csandGetMat(buf, x + dx, y + dy) == MAT_AIR) {
                switch (mat_props.kind) {
                    case MAT_KIND_SOLID:
                    case MAT_KIND_POWDER:
                        mat = csandRand() & 1 ? MAT_FIRE_GAS : MAT_FIRE_POWDER;
                        break;
                    case MAT_KIND_FLUID:
                        mat = csandRand() & 1 ? MAT_FIRE_GAS : MAT_FIRE_LIQUID;
                        break;
                    case MAT_KINDS_COUNT:
                        break;
                }

                *csandGetMat(buf, x, y) = mat | MAT_UPDATED_BIT;
                return;
            }
        }
    }
}
