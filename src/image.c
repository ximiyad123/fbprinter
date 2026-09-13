#include "image.h"
#include "framebuffer.h"

void image_draw_rgba(
    FBPrinterConfig *fb,
    const uint8_t *pixels,
    int width,
    int height,
    int x,
    int y)
{
    if (!fb || !pixels)
        return;

    int scale = fb->img_size;

    if (scale < 1)
        scale = 1;

    for (int py = 0; py < height; ++py) {

        for (int px = 0; px < width; ++px) {

            const uint8_t *pixel =
                pixels +
                ((size_t)py * width + px) * 4;

            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];
            uint8_t a = pixel[3];

            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {

                    fb_put_pixel(
                        fb,
                        x + px * scale + sx,
                        y + py * scale + sy,
                        r,
                        g,
                        b,
                        a
                    );
                }
            }
        }
    }
}
