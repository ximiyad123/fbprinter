#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "fbprinter.h"

int fb_open(FBConfig *fb);
void fb_close(FBConfig *fb);

void fb_clear(FBConfig *fb);

void fb_put_pixel(
    FBConfig *fb,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a
);

#endif
