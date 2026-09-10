#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <png.h>

#include "png_renderer.h"
#include "image.h"

int png_render(
    FBConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen PNG");
        return -1;
    }

    png_byte signature[8];

    if (fread(signature, 1, 8, file) != 8 ||
        png_sig_cmp(signature, 0, 8)) {
        fprintf(stderr, "Not a valid PNG file: %s\n", filename);
        fclose(file);
        return -1;
    }

    png_structp png =
        png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL,
            NULL,
            NULL
        );

    if (!png) {
        fprintf(stderr, "Failed to create PNG read structure\n");
        fclose(file);
        return -1;
    }

    png_infop info =
        png_create_info_struct(png);

    if (!info) {
        fprintf(stderr, "Failed to create PNG info structure\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(file);
        return -1;
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "PNG decoding failed: %s\n", filename);

        png_destroy_read_struct(
            &png,
            &info,
            NULL
        );

        fclose(file);
        return -1;
    }

    png_init_io(png, file);

    /*
     * We already consumed the PNG signature.
     */
    png_set_sig_bytes(png, 8);

    png_read_info(png, info);

    png_uint_32 width =
        png_get_image_width(png, info);

    png_uint_32 height =
        png_get_image_height(png, info);

    int color_type =
        png_get_color_type(png, info);

    int bit_depth =
        png_get_bit_depth(png, info);

    /*
     * Convert everything into:
     *
     * RGBA8888
     *
     * 8 bits per channel.
     */

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
        bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {

        png_set_filler(
            png,
            0xFF,
            PNG_FILLER_AFTER
        );
    }

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {

        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    png_size_t row_bytes =
        png_get_rowbytes(png, info);

    size_t buffer_size =
        (size_t)row_bytes * height;

    uint8_t *pixels =
        malloc(buffer_size);

    if (!pixels) {
        fprintf(stderr,
                "Failed to allocate PNG buffer\n");

        png_destroy_read_struct(
            &png,
            &info,
            NULL
        );

        fclose(file);
        return -1;
    }

    png_bytep *rows =
        malloc(sizeof(png_bytep) * height);

    if (!rows) {
        fprintf(stderr,
                "Failed to allocate PNG rows\n");

        free(pixels);

        png_destroy_read_struct(
            &png,
            &info,
            NULL
        );

        fclose(file);
        return -1;
    }

    for (png_uint_32 y = 0; y < height; ++y) {
        rows[y] =
            pixels +
            (size_t)y * row_bytes;
    }

    png_read_image(png, rows);

    png_read_end(png, NULL);

    free(rows);

    png_destroy_read_struct(
        &png,
        &info,
        NULL
    );

    fclose(file);

    /*
     * libpng has now produced RGBA pixels.
     */
    image_draw_rgba(
        fb,
        pixels,
        (int)width,
        (int)height,
        fb->image_x,
        fb->image_y
    );

    free(pixels);

    return 0;
}
