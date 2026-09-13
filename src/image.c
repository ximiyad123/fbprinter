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


/*
 * Draw an RGBA image using the configured
 * generic positioning mode.
 */
void image_draw_rgba_positioned(
    FBPrinterConfig *fb,
    const uint8_t *pixels,
    int width,
    int height)
{
    if (!fb || !pixels)
        return;

    if (width <= 0 || height <= 0)
        return;

    int scale = fb->img_size;

    if (scale < 1)
        scale = 1;


    /*
     * Positioning must use the final displayed size,
     * because scaling changes the object's dimensions.
     */
    uint32_t displayed_width =
        (uint32_t)width *
        (uint32_t)scale;

    uint32_t displayed_height =
        (uint32_t)height *
        (uint32_t)scale;


    int x;
    int y;

    fbprinter_get_image_position(
        fb,
        displayed_width,
        displayed_height,
        &x,
        &y
    );


    image_draw_rgba(
        fb,
        pixels,
        width,
        height,
        x,
        y
    );
}
