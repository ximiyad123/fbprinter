#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "fbprinter.h"
#include "framebuffer.h"
#include "renderer.h"
#include "ini_parser.h"

static void print_usage(const char *program)
{
    printf(
        "Usage:\n"
        "  %s <address,width,height> <file> [options]\n"
        "  %s --config=<file> [options]\n"
        "\n"
        "Options:\n"
        "  --config=<file>       Load configuration file\n"
        "  --scale=<n>            Text scale\n"
        "  --text-x=<n>           Text X position\n"
        "  --text-y=<n>           Text Y position\n"
        "  --image-x=<n>          Image X position\n"
        "  --image-y=<n>          Image Y position\n"
        "  --keep-leftover        Preserve existing framebuffer\n"
        "  --help                 Show this help\n"
        "\n"
        "Framebuffer format:\n"
        "  address,width,height\n"
        "\n"
        "Examples:\n"
        "  sudo %s 0xb8000000,1220,2712 boot.txt\n"
        "  sudo %s 0xb8000000,1220,2712 image.png --image-x=100\n"
        "  sudo %s --config=/etc/fbprinter.ini\n",
        program,
        program,
        program,
        program,
        program
    );
}

static int parse_framebuffer(
    FBConfig *config,
    const char *value)
{
    if (!config || !value)
        return -1;

    char *copy = strdup(value);

    if (!copy)
        return -1;

    char *saveptr = NULL;

    char *address_str =
        strtok_r(copy, ",", &saveptr);

    char *width_str =
        strtok_r(NULL, ",", &saveptr);

    char *height_str =
        strtok_r(NULL, ",", &saveptr);

    if (!address_str ||
        !width_str ||
        !height_str) {

        fprintf(
            stderr,
            "Invalid framebuffer specification: %s\n",
            value
        );

        free(copy);
        return -1;
    }

    errno = 0;

    unsigned long long address =
        strtoull(
            address_str,
            NULL,
            0
        );

    if (errno != 0) {
        fprintf(
            stderr,
            "Invalid framebuffer address: %s\n",
            address_str
        );

        free(copy);
        return -1;
    }

    errno = 0;

    unsigned long width =
        strtoul(
            width_str,
            NULL,
            0
        );

    if (errno != 0 || width == 0) {
        fprintf(
            stderr,
            "Invalid framebuffer width: %s\n",
            width_str
        );

        free(copy);
        return -1;
    }

    errno = 0;

    unsigned long height =
        strtoul(
            height_str,
            NULL,
            0
        );

    if (errno != 0 || height == 0) {
        fprintf(
            stderr,
            "Invalid framebuffer height: %s\n",
            height_str
        );

        free(copy);
        return -1;
    }

    config->address =
        (uintptr_t)address;

    config->width =
        (uint32_t)width;

    config->height =
        (uint32_t)height;

    free(copy);

    return 0;
}

static int parse_int_option(
    const char *value,
    int *output)
{
    if (!value || !output)
        return -1;

    char *end = NULL;

    errno = 0;

    long number =
        strtol(
            value,
            &end,
            0
        );

    if (errno != 0 ||
        end == value ||
        *end != '\0') {
        return -1;
    }

    *output = (int)number;

    return 0;
}

static int set_option(
    FBConfig *config,
    const char *argument)
{
    const char *value;

    if (strncmp(
            argument,
            "--scale=",
            8) == 0) {

        value = argument + 8;

        return parse_int_option(
            value,
            &config->scale
        );
    }

    if (strncmp(
            argument,
            "--text-x=",
            9) == 0) {

        value = argument + 9;

        return parse_int_option(
            value,
            &config->text_x
        );
    }

    if (strncmp(
            argument,
            "--text-y=",
            9) == 0) {

        value = argument + 9;

        return parse_int_option(
            value,
            &config->text_y
        );
    }

    if (strncmp(
            argument,
            "--image-x=",
            10) == 0) {

        value = argument + 10;

        return parse_int_option(
            value,
            &config->image_x
        );
    }

    if (strncmp(
            argument,
            "--image-y=",
            10) == 0) {

        value = argument + 10;

        return parse_int_option(
            value,
            &config->image_y
        );
    }

    if (strcmp(
            argument,
            "--keep-leftover") == 0) {

        config->keep_leftover = 1;
        return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    FBConfig config = {
        .address = 0,
        .width = 0,
        .height = 0,

        .scale = 1,

        .text_x = 0,
        .text_y = 0,

        .image_x = 0,
        .image_y = 0,

        .keep_leftover = 0,

        .input_file = NULL,

        .mem_fd = -1,
        .mapped_base = NULL,
        .fb_ptr = NULL,
        .map_size = 0
    };

    const char *config_file = NULL;

    /*
     * First pass:
     * find --config before processing the rest.
     */
    for (int i = 1; i < argc; ++i) {

        if (strncmp(
                argv[i],
                "--config=",
                9) == 0) {

            config_file =
                argv[i] + 9;
        }
    }

    /*
     * Load configuration first.
     */
    if (config_file) {

        if (ini_load(
                &config,
                config_file) != 0) {

            return EXIT_FAILURE;
        }
    }

    /*
     * Command-line arguments override config.ini.
     */
    for (int i = 1; i < argc; ++i) {

        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            free(config.input_file);
            return EXIT_SUCCESS;
        }

        if (strncmp(
                arg,
                "--config=",
                9) == 0) {
            continue;
        }

        /*
         * framebuffer specification
         */
        if (arg[0] != '-' &&
            strchr(arg, ',') != NULL) {

            if (parse_framebuffer(
                    &config,
                    arg) != 0) {

                free(config.input_file);
                return EXIT_FAILURE;
            }

            continue;
        }

        /*
         * Input filename.
         */
        if (arg[0] != '-') {

            free(config.input_file);

            config.input_file =
                strdup(arg);

            if (!config.input_file) {
                fprintf(
                    stderr,
                    "Out of memory\n"
                );

                return EXIT_FAILURE;
            }

            continue;
        }

        /*
         * Other options.
         */
        int result =
            set_option(
                &config,
                arg
            );

        if (result < 0) {
            fprintf(
                stderr,
                "Invalid option: %s\n",
                arg
            );

            free(config.input_file);
            return EXIT_FAILURE;
        }

        if (result > 0) {
            fprintf(
                stderr,
                "Unknown option: %s\n",
                arg
            );

            free(config.input_file);
            return EXIT_FAILURE;
        }
    }

    /*
     * Validate configuration.
     */
    if (config.address == 0 ||
        config.width == 0 ||
        config.height == 0) {

        fprintf(
            stderr,
            "Framebuffer configuration is incomplete\n"
        );

        print_usage(argv[0]);

        free(config.input_file);

        return EXIT_FAILURE;
    }

    if (!config.input_file) {

        fprintf(
            stderr,
            "No input file specified\n"
        );

        print_usage(argv[0]);

        return EXIT_FAILURE;
    }

    if (config.scale < 1) {
        fprintf(
            stderr,
            "Scale must be at least 1\n"
        );

        free(config.input_file);

        return EXIT_FAILURE;
    }

    /*
     * Open physical framebuffer.
     */
    if (fb_open(&config) != 0) {
        free(config.input_file);
        return EXIT_FAILURE;
    }

    /*
     * Unless --keep-leftover was specified,
     * clear the existing framebuffer first.
     */
    if (!config.keep_leftover)
        fb_clear(&config);

    /*
     * Render the requested file.
     */
    int result =
        renderer_render(&config);

    /*
     * Always clean up the mmap and /dev/mem.
     */
    fb_close(&config);

    free(config.input_file);

    if (result != 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
