#include "image.h"
#include "framebuffer.h"

void image_draw_rgba(
    FBConfig *fb,
    const uint8_t *pixels,
    int width,
    int height,
    int x,
    int y)
{
    if (!fb || !pixels)
        return;

    if (width <= 0 || height <= 0)
        return;

    for (int iy = 0; iy < height; ++iy) {
        for (int ix = 0; ix < width; ++ix) {

            const uint8_t *pixel =
                pixels +
                ((size_t)iy * width + ix) * 4;

            fb_put_pixel(
                fb,
                x + ix,
                y + iy,

                pixel[0],
                pixel[1],
                pixel[2],
                pixel[3]
            );
        }
    }
}
