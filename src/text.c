#include <stdio.h>
#include <stdlib.h>

#include "text.h"
#include "framebuffer.h"
#include "font.h"

static void draw_character(
    FBPrinterConfig *fb,
    unsigned char c,
    int x,
    int y,
    int scale)
{
    for (int gy = 0; gy < FONTH; ++gy) {

        unsigned char row = letters[c][gy];

        for (int gx = 0; gx < FONTW; ++gx) {

            if (!(row & (0x80 >> gx)))
                continue;

            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {

                    fb_put_pixel(
                        fb,
                        x + gx * scale + sx,
                        y + gy * scale + sy,
                        255,
                        255,
                        255,
                        255
                    );
                }
            }
        }
    }
}

static void draw_text(
    FBPrinterConfig *fb,
    const char *text)
{
    int x = fb->text_x;
    int y = fb->text_y;

    int scale = fb->text_size;

    if (scale < 1)
        scale = 1;

    int start_x = x;

    while (*text) {

        unsigned char c =
            (unsigned char)*text++;

        if (c == '\n') {
            x = start_x;
            y += FONTH * scale;
            continue;
        }

        if (c == '\r')
            continue;

        if (c == '\t') {
            x += FONTW * scale * 4;
            continue;
        }

        draw_character(
            fb,
            c,
            x,
            y,
            scale
        );

        x += FONTW * scale;
    }
}

int text_render(
    FBPrinterConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen");
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    long size = ftell(file);

    if (size < 0) {
        fclose(file);
        return -1;
    }

    rewind(file);

    char *buffer =
        malloc((size_t)size + 1);

    if (!buffer) {
        fclose(file);
        return -1;
    }

    size_t read_size =
        fread(
            buffer,
            1,
            (size_t)size,
            file
        );

    fclose(file);

    if (read_size != (size_t)size) {
        free(buffer);
        return -1;
    }

    buffer[size] = '\0';

    draw_text(fb, buffer);

    free(buffer);

    return 0;
}
