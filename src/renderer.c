#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "renderer.h"
#include "text.h"
#include "png_renderer.h"
#include "jpg.h"
#include "gif.h"

static const char *get_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');

    if (!dot || dot == filename)
        return NULL;

    return dot + 1;
}

int renderer_render(FBConfig *fb)
{
    if (!fb || !fb->input_file) {
        fprintf(stderr, "No input file specified\n");
        return -1;
    }

    const char *extension =
        get_extension(fb->input_file);

    if (!extension) {
        fprintf(
            stderr,
            "Cannot determine file type: %s\n",
            fb->input_file
        );
        return -1;
    }

    if (strcasecmp(extension, "txt") == 0) {
        return text_render(
            fb,
            fb->input_file
        );
    }

    if (strcasecmp(extension, "png") == 0) {
        return png_render(
            fb,
            fb->input_file
        );
    }

    if (strcasecmp(extension, "jpg") == 0 ||
        strcasecmp(extension, "jpeg") == 0) {

        return jpg_render(
            fb,
            fb->input_file
        );
    }

    if (strcasecmp(extension, "gif") == 0) {
        return gif_render(
            fb,
            fb->input_file
        );
    }

    fprintf(
        stderr,
        "Unsupported input format: .%s\n",
        extension
    );

    return -1;
}
