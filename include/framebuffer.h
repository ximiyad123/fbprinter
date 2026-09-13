#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "fbprinter.h"

int fb_open(FBPrinterConfig *config);
void fb_close(FBPrinterConfig *config);

void fb_clear(FBPrinterConfig *config);

void fb_put_pixel(
    FBPrinterConfig *config,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a
);

#endif
