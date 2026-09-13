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
    /*
     * font.h stores printable ASCII characters starting at:
     *
     * letters[0]  = ' '
     * letters[1]  = '!'
     * ...
     * letters[33] = 'A'
     * ...
     * letters[65] = 'a'
     *
     * Therefore the ASCII character must be converted to
     * the corresponding font table index.
     */
    if (c < 32 || c > 126)
        return;

    int index = font_index((char)c);

    for (int gy = 0; gy < FONTH; ++gy) {

        unsigned char row = letters[index][gy];

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

/*
 * Draw a solid black background strip for one line of text.
 */
static void draw_text_background(
    FBPrinterConfig *fb,
    int x,
    int y,
    int width,
    int height)
{
    for (int py = 0; py < height; ++py) {

        for (int px = 0; px < width; ++px) {

            fb_put_pixel(
                fb,
                x + px,
                y + py,
                0,
                0,
                0,
                255
            );
        }
    }
}

/*
 * Find the width of a single line.
 */
static int line_width(
    const char *text,
    int scale)
{
    int width = 0;

    while (*text && *text != '\n') {

        unsigned char c =
            (unsigned char)*text++;

        if (c == '\r')
            continue;

        if (c == '\t') {
            width += FONTW * scale * 4;
            continue;
        }

        width += FONTW * scale;
    }

    return width;
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

        /*
         * Draw the black background for the current line
         * before drawing any characters.
         */
        int width = line_width(text, scale);

        draw_text_background(
            fb,
            x,
            y,
            width,
            FONTH * scale
        );

        /*
         * Draw characters until the end of this line.
         */
        while (*text && *text != '\n') {

            unsigned char c =
                (unsigned char)*text++;

            /*
             * Ignore carriage return.
             */
            if (c == '\r')
                continue;

            /*
             * Simple tab handling.
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

            /*
             * Advance to the next character.
             */
            x += FONTW * scale;
        }

        /*
         * Move to the next line.
         */
        if (*text == '\n') {
            text++;

            x = start_x;
            y += FONTH * scale;
        }
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

    /*
     * Determine file size.
     */
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

    /*
     * Allocate space for the text plus the terminating NUL.
     */
    char *buffer =
        malloc((size_t)size + 1);

    if (!buffer) {
        fclose(file);
        return -1;
    }

    /*
     * Read the entire file.
     */
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

    /*
     * Render the text.
     */
    draw_text(fb, buffer);

    free(buffer);

    return 0;
}
