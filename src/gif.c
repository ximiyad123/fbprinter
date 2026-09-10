#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <gif_lib.h>

#include "gif.h"
#include "framebuffer.h"

#define GIF_MAX_FPS       15
#define GIF_MIN_FRAME_MS  (1000 / GIF_MAX_FPS)

static int gif_get_delay(const SavedImage *frame)
{
    if (!frame)
        return GIF_MIN_FRAME_MS;

    for (int i = 0;
         i < frame->ExtensionBlockCount;
         ++i) {

        const ExtensionBlock *ext =
            &frame->ExtensionBlocks[i];

        if (ext->Function == GRAPHICS_EXT_FUNC_CODE &&
            ext->ByteCount >= 4) {

            /*
             * Graphics Control Extension:
             *
             * Byte 0: packed fields
             * Byte 1-2: delay in 1/100 seconds
             * Byte 3: transparent color index
             */

            int delay_cs =
                ext->Bytes[1] |
                (ext->Bytes[2] << 8);

            int delay_ms =
                delay_cs * 10;

            /*
             * Some GIFs specify 0 delay.
             * Don't allow that to turn into an
             * uncontrolled busy loop.
             */
            if (delay_ms <= 0)
                delay_ms = GIF_MIN_FRAME_MS;

            /*
             * Hard cap: never play faster than 15 FPS.
             */
            if (delay_ms < GIF_MIN_FRAME_MS)
                delay_ms = GIF_MIN_FRAME_MS;

            return delay_ms;
        }
    }

    return GIF_MIN_FRAME_MS;
}

static int gif_get_transparent_index(
    const SavedImage *frame)
{
    if (!frame)
        return -1;

    for (int i = 0;
         i < frame->ExtensionBlockCount;
         ++i) {

        const ExtensionBlock *ext =
            &frame->ExtensionBlocks[i];

        if (ext->Function == GRAPHICS_EXT_FUNC_CODE &&
            ext->ByteCount >= 4) {

            int transparent =
                ext->Bytes[0] & 0x01;

            if (transparent)
                return ext->Bytes[3];
        }
    }

    return -1;
}

static void gif_draw_frame(
    FBConfig *fb,
    const SavedImage *frame,
    int screen_width,
    int screen_height)
{
    if (!fb || !frame)
        return;

    const ColorMapObject *color_map =
        frame->ImageDesc.ColorMap;

    if (!color_map)
        color_map = NULL;

    /*
     * GIFs can have a local color map.
     * If none exists, the global map is used
     * by the caller.
     */

    int transparent_index =
        gif_get_transparent_index(frame);

    int left =
        frame->ImageDesc.Left;

    int top =
        frame->ImageDesc.Top;

    int width =
        frame->ImageDesc.Width;

    int height =
        frame->ImageDesc.Height;

    if (width <= 0 || height <= 0)
        return;

    for (int y = 0; y < height; ++y) {

        int screen_y =
            fb->image_y + top + y;

        if (screen_y < 0 ||
            screen_y >= screen_height)
            continue;

        for (int x = 0; x < width; ++x) {

            int screen_x =
                fb->image_x + left + x;

            if (screen_x < 0 ||
                screen_x >= screen_width)
                continue;

            size_t index =
                (size_t)y * width + x;

            int color_index =
                frame->RasterBits[index];

            /*
             * Transparent GIF pixel:
             * leave the framebuffer untouched.
             */
            if (color_index == transparent_index)
                continue;

            if (!color_map)
                continue;

            if (color_index < 0 ||
                color_index >= color_map->ColorCount)
                continue;

            GifColorType color =
                color_map->Colors[color_index];

            fb_put_pixel(
                fb,
                screen_x,
                screen_y,
                color.Red,
                color.Green,
                color.Blue,
                255
            );
        }
    }
}

int gif_render(
    FBConfig *fb,
    const char *filename)
{
    if (!fb || !filename)
        return -1;

    int error = 0;

    GifFileType *gif =
        DGifOpenFileName(
            filename,
            &error
        );

    if (!gif) {
        fprintf(
            stderr,
            "Failed to open GIF: %s\n",
            filename
        );
        return -1;
    }

    if (DGifSlurp(gif) == GIF_ERROR) {
        fprintf(
            stderr,
            "Failed to decode GIF: %s\n",
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
     * Save the global color map.
     */
    const ColorMapObject *global_map =
        gif->SColorMap;

    for (int frame_index = 0;
         frame_index < gif->ImageCount;
         ++frame_index) {

        SavedImage *frame =
            &gif->SavedImages[frame_index];

        /*
         * gif_draw_frame() expects the frame's
         * local color map. If there isn't one,
         * temporarily use the global map.
         */
        ColorMapObject *old_map =
            frame->ImageDesc.ColorMap;

        if (!frame->ImageDesc.ColorMap)
            frame->ImageDesc.ColorMap =
                (ColorMapObject *)global_map;

        gif_draw_frame(
            fb,
            frame,
            (int)fb->width,
            (int)fb->height
        );

        /*
         * Restore the original local map pointer.
         */
        frame->ImageDesc.ColorMap =
            old_map;

        int delay_ms =
            gif_get_delay(frame);

        /*
         * Sleep between frames.
         */
        usleep(
            (useconds_t)delay_ms * 1000
        );
    }

    DGifCloseFile(
        gif,
        &error
    );

    return 0;
}
