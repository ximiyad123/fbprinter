#ifndef FBPRINTER_H
#define FBPRINTER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uintptr_t address;

    uint32_t width;
    uint32_t height;

    int scale;

    int text_x;
    int text_y;

    int image_x;
    int image_y;

    int keep_leftover;

    char *input_file;

    /* Internal framebuffer state */
    int mem_fd;
    void *mapped_base;
    uint32_t *fb_ptr;
    size_t map_size;
} FBConfig;

#endif
