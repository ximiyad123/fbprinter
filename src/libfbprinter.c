#include <stdlib.h>

#include "fbprinter.h"
#include "framebuffer.h"
#include "renderer.h"

void fbprinter_config_init(FBPrinterConfig *config)
{
    if (!config)
        return;

    config->address = 0;
    config->width = 0;
    config->height = 0;

    config->text_size = 1;
    config->img_size = 1;

    config->text_x = 0;
    config->text_y = 0;

    config->image_x = 0;
    config->image_y = 0;

    config->keep_leftover = 0;
    config->input_file = NULL;

    config->mem_fd = -1;
    config->mapped_base = NULL;
    config->fb_ptr = NULL;
    config->map_size = 0;
}

int fbprinter_open(FBPrinterConfig *config)
{
    if (!config)
        return -1;

    if (config->text_size < 1 ||
        config->text_size > 99)
        return -1;

    if (config->img_size < 1 ||
        config->img_size > 99)
        return -1;

    return fb_open(config);
}

void fbprinter_close(FBPrinterConfig *config)
{
    if (!config)
        return;

    fb_close(config);
}

void fbprinter_clear(FBPrinterConfig *config)
{
    if (!config)
        return;

    fb_clear(config);
}

int fbprinter_render(
    FBPrinterConfig *config,
    const char *filename)
{
    if (!config || !filename)
        return -1;

    /*
     * The renderer uses config->input_file internally,
     * so temporarily provide the requested filename.
     */
    char *old_file = config->input_file;

    config->input_file = (char *)filename;

    int result = renderer_render(config);

    config->input_file = old_file;

    return result;
}

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

void fbprinter_set_text_position(
    FBPrinterConfig *config,
    int x,
    int y)
{
    if (!config)
        return;

    config->text_x = x;
    config->text_y = y;
}

void fbprinter_set_image_position(
    FBPrinterConfig *config,
    int x,
    int y)
{
    if (!config)
        return;

    config->image_x = x;
    config->image_y = y;
}

void fbprinter_set_keep_leftover(
    FBPrinterConfig *config,
    int enabled)
{
    if (!config)
        return;

    config->keep_leftover = enabled ? 1 : 0;
}

void fbprinter_put_pixel(
    FBPrinterConfig *config,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a)
{
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
