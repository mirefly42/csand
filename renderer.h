#ifndef CSAND_RENDERER_H
#define CSAND_RENDERER_H

#include "rgba.h"
#include "vec2.h"
#include <stdint.h>

void csandRendererInit(CsandVec2Us world_size, CsandVec2Us framebuffer_size, const CsandRgba *colors, uint8_t colors_count);
void csandRendererRender(unsigned char *data, unsigned short width, unsigned short height);
void csandRendererUpdateViewport(CsandVec2Us framebuffer_size);
CsandVec2Us csandRendererScreenSpaceToWorldSpace(CsandVec2Us vec);

#endif
