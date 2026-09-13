#ifndef FBPRINTER_H
#define FBPRINTER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FBPrinterConfig {
    uintptr_t address;

    uint32_t width;
    uint32_t height;

    int text_size;
    int img_size;

    int text_x;
    int text_y;

    int image_x;
    int image_y;

    int keep_leftover;

    char *input_file;
    /*
     * Internal state.
     *
     * Applications using the library should not modify these.
     */
    int mem_fd;
    void *mapped_base;
    uint32_t *fb_ptr;
    size_t map_size;
} FBPrinterConfig;


/*
 * Library-level API.
 */

/* Set safe defaults. */
void fbprinter_config_init(FBPrinterConfig *config);

/* Open/map the physical framebuffer. */
int fbprinter_open(FBPrinterConfig *config);

/* Unmap/close the framebuffer. */
void fbprinter_close(FBPrinterConfig *config);

/* Clear framebuffer unless keep_leftover is desired. */
void fbprinter_clear(FBPrinterConfig *config);

/* Render a file according to its type. */
int fbprinter_render(
    FBPrinterConfig *config,
    const char *filename
);


/*
 * Configuration helpers.
 */

void fbprinter_set_text_size(
    FBPrinterConfig *config,
    int size
);

void fbprinter_set_image_size(
    FBPrinterConfig *config,
    int size
);

void fbprinter_set_text_position(
    FBPrinterConfig *config,
    int x,
    int y
);

void fbprinter_set_image_position(
    FBPrinterConfig *config,
    int x,
    int y
);

void fbprinter_set_keep_leftover(
    FBPrinterConfig *config,
    int enabled
);


/*
 * Low-level framebuffer API.
 */

void fbprinter_put_pixel(
    FBPrinterConfig *config,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a
);

#ifdef __cplusplus
}
#endif

#endif
