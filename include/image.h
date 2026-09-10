#ifndef IMAGE_H
#define IMAGE_H

#include "fbprinter.h"

void image_draw_rgba(
    FBConfig *fb,
    const uint8_t *pixels,
    int width,
    int height,
    int x,
    int y
);

#endif
