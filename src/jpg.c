#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>

#include <jpeglib.h>

#include "jpg.h"
#include "image.h"

/*
 * libjpeg uses its own error handler and calls longjmp()
 * when a decoding error occurs.
 */
typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf jump_buffer;
} JPGError;

static void jpg_error_exit(
    j_common_ptr cinfo)
{
    JPGError *error =
        (JPGError *)cinfo->err;

    longjmp(
        error->jump_buffer,
        1
    );
}

int jpg_render(
    FBConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("fopen JPEG");
        return -1;
    }

    struct jpeg_decompress_struct jpeg;

    JPGError error;

    jpeg.err =
        jpeg_std_error(&error.pub);

    error.pub.error_exit =
        jpg_error_exit;

    if (setjmp(error.jump_buffer)) {
        fprintf(stderr,
                "JPEG decoding failed: %s\n",
                filename);

        jpeg_destroy_decompress(&jpeg);
        fclose(file);

        return -1;
    }

    jpeg_create_decompress(&jpeg);

    jpeg_stdio_src(
        &jpeg,
        file
    );

    /*
     * Read JPEG header.
     */
    int result =
        jpeg_read_header(
            &jpeg,
            TRUE
        );

    if (result != JPEG_HEADER_OK) {
        fprintf(stderr,
                "Invalid JPEG header: %s\n",
                filename);

        jpeg_destroy_decompress(&jpeg);
        fclose(file);

        return -1;
    }

    /*
     * Force RGB output.
     *
     * This avoids depending on whether the source
     * JPEG is grayscale, CMYK, etc.
     */
    jpeg.out_color_space =
        JCS_RGB;

    jpeg_start_decompress(&jpeg);

    int width =
        (int)jpeg.output_width;

    int height =
        (int)jpeg.output_height;

    int components =
        (int)jpeg.output_components;

    if (width <= 0 ||
        height <= 0 ||
        components != 3) {

        fprintf(stderr,
                "Unsupported JPEG output format\n");

        jpeg_finish_decompress(&jpeg);
        jpeg_destroy_decompress(&jpeg);
        fclose(file);

        return -1;
    }

    /*
     * Convert RGB → RGBA.
     */
    size_t pixel_count =
        (size_t)width *
        (size_t)height;

    uint8_t *pixels =
        malloc(pixel_count * 4);

    if (!pixels) {
        fprintf(stderr,
                "Failed to allocate JPEG buffer\n");

        jpeg_finish_decompress(&jpeg);
        jpeg_destroy_decompress(&jpeg);
        fclose(file);

        return -1;
    }

    /*
     * libjpeg gives us one scanline at a time.
     */
    size_t row_stride =
        (size_t)width * 3;

    uint8_t *row =
        malloc(row_stride);

    if (!row) {
        fprintf(stderr,
                "Failed to allocate JPEG row buffer\n");

        free(pixels);

        jpeg_finish_decompress(&jpeg);
        jpeg_destroy_decompress(&jpeg);
        fclose(file);

        return -1;
    }

    while (jpeg.output_scanline <
           jpeg.output_height) {

        JSAMPROW row_pointer =
            row;

        jpeg_read_scanlines(
            &jpeg,
            &row_pointer,
            1
        );

        int y =
            (int)jpeg.output_scanline - 1;

        for (int x = 0; x < width; ++x) {

            size_t src =
                (size_t)x * 3;

            size_t dst =
                ((size_t)y * width + x) * 4;

            pixels[dst + 0] =
                row[src + 0];

            pixels[dst + 1] =
                row[src + 1];

            pixels[dst + 2] =
                row[src + 2];

            /*
             * JPEG has no alpha channel.
             */
            pixels[dst + 3] =
                255;
        }
    }

    free(row);

    jpeg_finish_decompress(&jpeg);
    jpeg_destroy_decompress(&jpeg);

    fclose(file);

    /*
     * Send the decoded image through the common
     * image renderer.
     */
    image_draw_rgba(
        fb,
        pixels,
        width,
        height,
        fb->image_x,
        fb->image_y
    );

    free(pixels);

    return 0;
}
