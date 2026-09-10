#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "framebuffer.h"
#include "font.h"

static void draw_character(
    FBConfig *fb,
    char c,
    int x,
    int y,
    int scale)
{
    int index = font_index(c);

    if (index < 0)
        return;

    const unsigned char *glyph =
        letters[index];

    for (int gy = 0; gy < FONTH; ++gy) {
        for (int gx = 0; gx < FONTW; ++gx) {

            /*
             * Adjust this if your font.h stores
             * pixels in a different format.
             */
            int byte = gy * FONTW + gx;

            if (!glyph[byte])
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
    FBConfig *fb,
    const char *text)
{
    int x = fb->text_x;
    int y = fb->text_y;

    int scale = fb->scale;

    if (scale < 1)
        scale = 1;

    int start_x = x;

    while (*text) {

        char c = *text++;

        if (c == '\n') {
            x = start_x;
            y += FONTH * scale;
            continue;
        }

        if (c == '\r')
            continue;

        /*
         * Basic tab handling.
         */
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
    FBConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen text file");
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
        fprintf(stderr,
                "Failed to allocate text buffer\n");
        fclose(file);
        return -1;
    }

    size_t read_size =
        fread(buffer, 1, (size_t)size, file);

    fclose(file);

    if (read_size != (size_t)size) {
        fprintf(stderr,
                "Failed to read text file\n");
        free(buffer);
        return -1;
    }

    buffer[size] = '\0';

    draw_text(fb, buffer);

    free(buffer);

    return 0;
}
