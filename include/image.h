#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#include "fbprinter.h"


void image_draw_rgba(
    FBPrinterConfig *fb,
    const uint8_t *pixels,
    int width,
    int height,
    int x,
    int y
);


/*
 * Draw using FBPrinterConfig's generic image
 * positioning mode.
 */
void image_draw_rgba_positioned(
    FBPrinterConfig *fb,
    const uint8_t *pixels,
    int width,
    int height
);

#endif
