#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "framebuffer.h"

int fb_open(FBConfig *fb)
{
    if (!fb || fb->address == 0 ||
        fb->width == 0 || fb->height == 0) {
        fprintf(stderr, "Invalid framebuffer configuration\n");
        return -1;
    }

    fb->mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fb->mem_fd < 0) {
        perror("open /dev/mem");
        return -1;
    }

    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size <= 0) {
        fprintf(stderr, "Could not determine page size\n");
        close(fb->mem_fd);
        fb->mem_fd = -1;
        return -1;
    }

    uintptr_t page_mask =
        (uintptr_t)page_size - 1;

    uintptr_t aligned_address =
        fb->address & ~page_mask;

    size_t offset =
        fb->address - aligned_address;

    size_t framebuffer_size =
        (size_t)fb->width *
        (size_t)fb->height *
        sizeof(uint32_t);

    fb->map_size =
        offset + framebuffer_size;

    fb->mapped_base = mmap(
        NULL,
        fb->map_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fb->mem_fd,
        (off_t)aligned_address
    );

    if (fb->mapped_base == MAP_FAILED) {
        perror("mmap framebuffer");

        close(fb->mem_fd);

        fb->mem_fd = -1;
        fb->mapped_base = NULL;

        return -1;
    }

    fb->fb_ptr =
        (uint32_t *)(
            (uint8_t *)fb->mapped_base +
            offset
        );

    return 0;
}

void fb_close(FBConfig *fb)
{
    if (!fb)
        return;

    if (fb->mapped_base &&
        fb->mapped_base != MAP_FAILED) {
        munmap(
            fb->mapped_base,
            fb->map_size
        );
    }

    if (fb->mem_fd >= 0)
        close(fb->mem_fd);

    fb->mapped_base = NULL;
    fb->fb_ptr = NULL;
    fb->mem_fd = -1;
}

void fb_clear(FBConfig *fb)
{
    if (!fb || !fb->fb_ptr)
        return;

    memset(
        fb->fb_ptr,
        0,
        (size_t)fb->width *
        (size_t)fb->height *
        sizeof(uint32_t)
    );
}

void fb_put_pixel(
    FBConfig *fb,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a)
{
    if (!fb || !fb->fb_ptr)
        return;

    if (x < 0 || y < 0)
        return;

    if ((uint32_t)x >= fb->width ||
        (uint32_t)y >= fb->height)
        return;

    uint32_t *pixel =
        &fb->fb_ptr[
            (size_t)y * fb->width + x
        ];

    /*
     * Fully transparent:
     * don't touch the existing framebuffer.
     */
    if (a == 0)
        return;

    /*
     * Fully opaque.
     */
    if (a == 255) {
        *pixel =
            0xFF000000u |
            ((uint32_t)r << 16) |
            ((uint32_t)g << 8) |
            b;

        return;
    }

    /*
     * Alpha blend against the existing pixel.
     */
    uint32_t old = *pixel;

    uint8_t old_r =
        (old >> 16) & 0xff;

    uint8_t old_g =
        (old >> 8) & 0xff;

    uint8_t old_b =
        old & 0xff;

    uint32_t inv_alpha =
        255u - a;

    uint8_t new_r =
        (uint8_t)(
            ((uint32_t)r * a +
             (uint32_t)old_r * inv_alpha) / 255
        );

    uint8_t new_g =
        (uint8_t)(
            ((uint32_t)g * a +
             (uint32_t)old_g * inv_alpha) / 255
        );

    uint8_t new_b =
        (uint8_t)(
            ((uint32_t)b * a +
             (uint32_t)old_b * inv_alpha) / 255
        );

    *pixel =
        0xFF000000u |
        ((uint32_t)new_r << 16) |
        ((uint32_t)new_g << 8) |
        new_b;
}
