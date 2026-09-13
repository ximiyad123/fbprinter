#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "fbprinter.h"
#include "framebuffer.h"
#include "renderer.h"


/*
 * =========================================================
 * Configuration
 * =========================================================
 */

void fbprinter_config_init(
    FBPrinterConfig *config)
{
    if (!config)
        return;

    memset(
        config,
        0,
        sizeof(*config)
    );


    /*
     * Framebuffer.
     */
    config->address = 0;

    config->width = 0;
    config->height = 0;


    /*
     * Text.
     */
    config->text_size = 1;

    config->text_x = 0;
    config->text_y = 0;

    config->text_position =
        FBPRINTER_POSITION_EXACT;


    /*
     * Images.
     */
    config->img_size = 1;

    config->image_x = 0;
    config->image_y = 0;

    config->image_position =
        FBPRINTER_POSITION_EXACT;


    /*
     * Preserve framebuffer by default: disabled.
     */
    config->keep_leftover = 0;


    /*
     * Input file.
     */
    config->input_file = NULL;


    /*
     * Internal framebuffer state.
     */
    config->mem_fd = -1;

    config->mapped_base = NULL;

    config->fb_ptr = NULL;

    config->map_size = 0;
}


/*
 * =========================================================
 * Framebuffer
 * =========================================================
 */

int fbprinter_open(
    FBPrinterConfig *config)
{
    if (!config)
        return -1;


    if (config->text_size < 1 ||
        config->text_size > 99) {
        return -1;
    }


    if (config->img_size < 1 ||
        config->img_size > 99) {
        return -1;
    }


    if (config->width == 0 ||
        config->height == 0) {
        return -1;
    }


    return fb_open(config);
}


void fbprinter_close(
    FBPrinterConfig *config)
{
    if (!config)
        return;

    fb_close(config);
}


void fbprinter_clear(
    FBPrinterConfig *config)
{
    if (!config)
        return;

    fb_clear(config);
}


/*
 * =========================================================
 * Pixel
 * =========================================================
 */

void fbprinter_put_pixel(
    FBPrinterConfig *config,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a)
{
    if (!config)
        return;

    fb_put_pixel(
        config,
        x,
        y,
        r,
        g,
        b,
        a
    );
}


/*
 * =========================================================
 * Generic position calculation
 * =========================================================
 */

void fbprinter_calculate_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    FBPrinterPosition position,
    int exact_x,
    int exact_y,
    int *out_x,
    int *out_y)
{
    if (!config ||
        !out_x ||
        !out_y) {
        return;
    }


    /*
     * Default to the exact coordinates.
     */
    *out_x = exact_x;
    *out_y = exact_y;


    switch (position) {

    case FBPRINTER_POSITION_EXACT:

        *out_x = exact_x;
        *out_y = exact_y;

        break;


    case FBPRINTER_POSITION_TOP_LEFT:

        *out_x = 0;
        *out_y = 0;

        break;


    case FBPRINTER_POSITION_TOP_RIGHT:

        *out_x =
            (int)config->width -
            (int)object_width;

        *out_y = 0;

        break;


    case FBPRINTER_POSITION_BOTTOM_LEFT:

        *out_x = 0;

        *out_y =
            (int)config->height -
            (int)object_height;

        break;


    case FBPRINTER_POSITION_BOTTOM_RIGHT:

        *out_x =
            (int)config->width -
            (int)object_width;

        *out_y =
            (int)config->height -
            (int)object_height;

        break;


    case FBPRINTER_POSITION_CENTER:

        *out_x =
            ((int)config->width -
             (int)object_width) / 2;

        *out_y =
            ((int)config->height -
             (int)object_height) / 2;

        break;


    default:

        *out_x = exact_x;
        *out_y = exact_y;

        break;
    }
}


/*
 * =========================================================
 * Text positioning
 * =========================================================
 */

void fbprinter_set_text_position(
    FBPrinterConfig *config,
    int x,
    int y)
{
    if (!config)
        return;


    config->text_x = x;
    config->text_y = y;


    /*
     * Setting exact coordinates selects EXACT mode.
     */
    config->text_position =
        FBPRINTER_POSITION_EXACT;
}


void fbprinter_set_text_position_mode(
    FBPrinterConfig *config,
    FBPrinterPosition position)
{
    if (!config)
        return;


    config->text_position = position;
}


void fbprinter_get_text_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    int *x,
    int *y)
{
    if (!config ||
        !x ||
        !y) {
        return;
    }


    fbprinter_calculate_position(
        config,
        object_width,
        object_height,
        config->text_position,
        config->text_x,
        config->text_y,
        x,
        y
    );
}


/*
 * =========================================================
 * Image positioning
 * =========================================================
 */

void fbprinter_set_image_position(
    FBPrinterConfig *config,
    int x,
    int y)
{
    if (!config)
        return;


    config->image_x = x;
    config->image_y = y;


    /*
     * Setting exact coordinates selects EXACT mode.
     */
    config->image_position =
        FBPRINTER_POSITION_EXACT;
}


void fbprinter_set_image_position_mode(
    FBPrinterConfig *config,
    FBPrinterPosition position)
{
    if (!config)
        return;


    config->image_position = position;
}


void fbprinter_get_image_position(
    const FBPrinterConfig *config,
    uint32_t object_width,
    uint32_t object_height,
    int *x,
    int *y)
{
    if (!config ||
        !x ||
        !y) {
        return;
    }


    fbprinter_calculate_position(
        config,
        object_width,
        object_height,
        config->image_position,
        config->image_x,
        config->image_y,
        x,
        y
    );
}


/*
 * =========================================================
 * Generic buffer drawing
 * =========================================================
 *
 * Buffer format:
 *
 *     0xAARRGGBB
 */

int fbprinter_draw_buffer(
    FBPrinterConfig *config,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    int x,
    int y)
{
    if (!config ||
        !pixels) {
        return -1;
    }


    if (width == 0 ||
        height == 0) {
        return -1;
    }


    for (uint32_t py = 0;
         py < height;
         py++) {

        for (uint32_t px = 0;
             px < width;
             px++) {

            size_t index =
                (size_t)py *
                (size_t)width +
                (size_t)px;


            uint32_t pixel =
                pixels[index];


            uint8_t a =
                (uint8_t)(
                    (pixel >> 24) &
                    0xff
                );


            uint8_t r =
                (uint8_t)(
                    (pixel >> 16) &
                    0xff
                );


            uint8_t g =
                (uint8_t)(
                    (pixel >> 8) &
                    0xff
                );


            uint8_t b =
                (uint8_t)(
                    pixel &
                    0xff
                );


            fbprinter_put_pixel(
                config,
                x + (int)px,
                y + (int)py,
                r,
                g,
                b,
                a
            );
        }
    }


    return 0;
}


/*
 * =========================================================
 * Positioned buffer drawing
 * =========================================================
 */

int fbprinter_draw_buffer_positioned(
    FBPrinterConfig *config,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    FBPrinterPosition position,
    int x,
    int y)
{
    if (!config ||
        !pixels) {
        return -1;
    }


    if (width == 0 ||
        height == 0) {
        return -1;
    }


    int destination_x;
    int destination_y;


    fbprinter_calculate_position(
        config,
        width,
        height,
        position,
        x,
        y,
        &destination_x,
        &destination_y
    );


    return fbprinter_draw_buffer(
        config,
        pixels,
        width,
        height,
        destination_x,
        destination_y
    );
}


/*
 * =========================================================
 * High-level renderer
 * =========================================================
 */

int fbprinter_render(
    FBPrinterConfig *config,
    const char *filename)
{
    if (!config ||
        !filename) {
        return -1;
    }


    char *old_file =
        config->input_file;


    config->input_file =
        (char *)filename;


    int result =
        renderer_render(config);


    config->input_file =
        old_file;


    return result;
}


/*
 * =========================================================
 * Scaling
 * =========================================================
 */

void fbprinter_set_text_size(
    FBPrinterConfig *config,
    int size)
{
    if (!config)
        return;


    if (size < 1)
        size = 1;


    if (size > 99)
        size = 99;


    config->text_size = size;
}


void fbprinter_set_image_size(
    FBPrinterConfig *config,
    int size)
{
    if (!config)
        return;


    if (size < 1)
        size = 1;


    if (size > 99)
        size = 99;


    config->img_size = size;
}


/*
 * =========================================================
 * Framebuffer preservation
 * =========================================================
 */

void fbprinter_set_keep_leftover(
    FBPrinterConfig *config,
    int enabled)
{
    if (!config)
        return;


    config->keep_leftover =
        enabled ? 1 : 0;
}


/*
 * =========================================================
 * Position string conversion
 * =========================================================
 */

const char *fbprinter_position_to_string(
    FBPrinterPosition position)
{
    switch (position) {

    case FBPRINTER_POSITION_EXACT:
        return "exact";


    case FBPRINTER_POSITION_TOP_LEFT:
        return "top-left";


    case FBPRINTER_POSITION_TOP_RIGHT:
        return "top-right";


    case FBPRINTER_POSITION_BOTTOM_LEFT:
        return "bottom-left";


    case FBPRINTER_POSITION_BOTTOM_RIGHT:
        return "bottom-right";


    case FBPRINTER_POSITION_CENTER:
        return "center";


    default:
        return "exact";
    }
}


FBPrinterPosition fbprinter_position_from_string(
    const char *value)
{
    if (!value)
        return FBPRINTER_POSITION_EXACT;


    if (strcasecmp(value, "exact") == 0)
        return FBPRINTER_POSITION_EXACT;


    if (strcasecmp(value, "top-left") == 0 ||
        strcasecmp(value, "topleft") == 0)
        return FBPRINTER_POSITION_TOP_LEFT;


    if (strcasecmp(value, "top-right") == 0 ||
        strcasecmp(value, "topright") == 0)
        return FBPRINTER_POSITION_TOP_RIGHT;


    if (strcasecmp(value, "bottom-left") == 0 ||
        strcasecmp(value, "bottomleft") == 0)
        return FBPRINTER_POSITION_BOTTOM_LEFT;


    if (strcasecmp(value, "bottom-right") == 0 ||
        strcasecmp(value, "bottomright") == 0)
        return FBPRINTER_POSITION_BOTTOM_RIGHT;


    if (strcasecmp(value, "center") == 0 ||
        strcasecmp(value, "centre") == 0)
        return FBPRINTER_POSITION_CENTER;


    return FBPRINTER_POSITION_EXACT;
}
