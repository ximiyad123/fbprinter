#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include <gif_lib.h>

#include "gif.h"
#include "framebuffer.h"

#define GIF_MAX_FPS 15
#define GIF_MIN_FRAME_NS (1000000000LL / GIF_MAX_FPS)


static void sleep_until(const struct timespec *deadline)
{
    for (;;) {
        int result = clock_nanosleep(
            CLOCK_MONOTONIC,
            TIMER_ABSTIME,
            deadline,
            NULL
        );

        if (result == 0)
            return;

        if (result != EINTR)
            return;
    }
}


static void timespec_add_ns(
    struct timespec *time,
    int64_t nanoseconds)
{
    time->tv_sec += nanoseconds / 1000000000LL;
    time->tv_nsec += nanoseconds % 1000000000LL;

    if (time->tv_nsec >= 1000000000L) {
        time->tv_sec++;
        time->tv_nsec -= 1000000000L;
    }
}


/*
 * Find the Graphics Control Extension belonging to a GIF frame.
 *
 * giflib stores extensions as ExtensionBlock structures, while
 * DGifExtensionToGCB() expects the raw bytes from one block.
 */
static int get_graphics_control_block(
    SavedImage *frame,
    GraphicsControlBlock *gcb)
{
    if (!frame || !gcb)
        return 0;

    for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
        ExtensionBlock *ext = &frame->ExtensionBlocks[i];

        if (ext->Function != GRAPHICS_EXT_FUNC_CODE)
            continue;

        if (ext->ByteCount <= 0 || !ext->Bytes)
            continue;

        if (DGifExtensionToGCB(
                ext->ByteCount,
                ext->Bytes,
                gcb) == GIF_OK)
            return 1;
    }

    return 0;
}


static int64_t get_frame_delay_ns(SavedImage *frame)
{
    GraphicsControlBlock gcb;

    if (!get_graphics_control_block(frame, &gcb))
        return GIF_MIN_FRAME_NS;

    /*
     * GIF DelayTime is measured in 1/100 second units.
     */
    int64_t delay =
        (int64_t)gcb.DelayTime * 10000000LL;

    /*
     * Prevent zero-delay and excessively fast GIFs.
     */
    if (delay <= 0)
        delay = GIF_MIN_FRAME_NS;

    if (delay < GIF_MIN_FRAME_NS)
        delay = GIF_MIN_FRAME_NS;

    return delay;
}


static int get_transparent_index(SavedImage *frame)
{
    GraphicsControlBlock gcb;

    if (!get_graphics_control_block(frame, &gcb))
        return -1;

    if (gcb.TransparentColor == NO_TRANSPARENT_COLOR)
        return -1;

    return gcb.TransparentColor;
}


static void draw_frame(
    FBPrinterConfig *fb,
    GifFileType *gif,
    SavedImage *frame)
{
    if (!fb || !gif || !frame)
        return;

    ColorMapObject *color_map =
        frame->ImageDesc.ColorMap;

    if (!color_map)
        color_map = gif->SColorMap;

    if (!color_map)
        return;

    int width = frame->ImageDesc.Width;
    int height = frame->ImageDesc.Height;

    int left = frame->ImageDesc.Left;
    int top = frame->ImageDesc.Top;

    int scale = fb->img_size;

    if (scale < 1)
        scale = 1;

    int transparent_index =
        get_transparent_index(frame);

    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {

            size_t offset =
                (size_t)py * (size_t)width +
                (size_t)px;

            int color_index =
                frame->RasterBits[offset];

            if (color_index == transparent_index)
                continue;

            if (color_index < 0 ||
                color_index >= color_map->ColorCount)
                continue;

            GifColorType color =
                color_map->Colors[color_index];

            int dst_x =
                fb->image_x +
                (left + px) * scale;

            int dst_y =
                fb->image_y +
                (top + py) * scale;

            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {

                    fb_put_pixel(
                        fb,
                        dst_x + sx,
                        dst_y + sy,
                        color.Red,
                        color.Green,
                        color.Blue,
                        255
                    );
                }
            }
        }
    }
}


static void dispose_background(
    FBPrinterConfig *fb,
    SavedImage *frame)
{
    if (!fb || !frame)
        return;

    GraphicsControlBlock gcb;

    if (!get_graphics_control_block(frame, &gcb))
        return;

    if (gcb.DisposalMode != DISPOSE_BACKGROUND)
        return;

    /*
     * When keeping the old framebuffer contents, don't
     * erase the GIF frame background.
     */
    if (fb->keep_leftover)
        return;

    int scale = fb->img_size;

    if (scale < 1)
        scale = 1;

    int width = frame->ImageDesc.Width;
    int height = frame->ImageDesc.Height;

    int left = frame->ImageDesc.Left;
    int top = frame->ImageDesc.Top;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {

            int dst_x =
                fb->image_x +
                (left + px) * scale;

            int dst_y =
                fb->image_y +
                (top + py) * scale;

            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {

                    fb_put_pixel(
                        fb,
                        dst_x + sx,
                        dst_y + sy,
                        r,
                        g,
                        b,
                        255
                    );
                }
            }
        }
    }
}


int gif_render(
    FBPrinterConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    int error = 0;

    GifFileType *gif =
        DGifOpenFileName(filename, &error);

    if (!gif) {
        fprintf(
            stderr,
            "Cannot open GIF: %s\n",
            filename
        );

        return -1;
    }

    if (DGifSlurp(gif) != GIF_OK) {
        fprintf(
            stderr,
            "Cannot decode GIF: %s\n",
            filename
        );

        DGifCloseFile(gif, &error);

        return -1;
    }

    if (gif->ImageCount <= 0) {
        fprintf(
            stderr,
            "GIF contains no frames: %s\n",
            filename
        );

        DGifCloseFile(gif, &error);

        return -1;
    }


    /*
     * Absolute monotonic timing prevents rendering time
     * from accumulating into the GIF's frame delay.
     */
    struct timespec next_frame;

    if (clock_gettime(
            CLOCK_MONOTONIC,
            &next_frame) != 0) {

        perror("clock_gettime");

        DGifCloseFile(gif, &error);

        return -1;
    }


    for (int frame_index = 0;
         frame_index < gif->ImageCount;
         ++frame_index) {

        SavedImage *frame =
            &gif->SavedImages[frame_index];


        /*
         * Draw the current frame.
         */
        draw_frame(
            fb,
            gif,
            frame
        );


        /*
         * Get GIF-specified delay, clamped
         * to a maximum of 15 FPS.
         */
        int64_t delay =
            get_frame_delay_ns(frame);


        /*
         * Schedule the next frame relative to
         * the previous scheduled frame.
         */
        timespec_add_ns(
            &next_frame,
            delay
        );


        /*
         * Wait until the scheduled presentation
         * time instead of simply sleeping after
         * rendering.
         */
        sleep_until(
            &next_frame
        );


        /*
         * Handle DISPOSE_BACKGROUND after the
         * frame's presentation interval.
         */
        dispose_background(
            fb,
            frame
        );
    }


    if (DGifCloseFile(gif, &error) != GIF_OK) {
        fprintf(
            stderr,
            "Warning: failed to close GIF cleanly\n"
        );
    }

    return 0;
}
