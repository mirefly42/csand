#ifndef CSAND_RENDERER_H
#define CSAND_RENDERER_H

#include "rgba.h"
#include <stdint.h>

void csandRendererInit(const CsandRgba *colors, uint8_t colors_count);
void csandRendererRender(unsigned char *data, unsigned short width, unsigned short height);

#endif
