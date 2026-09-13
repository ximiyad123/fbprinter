
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "fbprinter.h"
#include "ini_parser.h"

static void usage(const char *program)
{
    printf(
        "Usage:\n"
        "  %s <address,width,height> <file> [options]\n"
        "  %s --config=<file> [options]\n"
        "\n"
        "Options:\n"
        "  --config=<file>       Load configuration\n"
        "  --text-size=<1-99>    Text scale\n"
        "  --img-size=<1-99>     Image scale\n"
        "  --text-x=<n>          Text X position\n"
        "  --text-y=<n>          Text Y position\n"
        "  --image-x=<n>         Image X position\n"
        "  --image-y=<n>         Image Y position\n"
        "  --keep-leftover       Preserve framebuffer contents\n"
        "  --help                Show this help\n",
        program,
        program
    );
}

static int parse_framebuffer(
    FBPrinterConfig *config,
    const char *value)
{
    char *copy = strdup(value);

    if (!copy)
        return -1;

    char *saveptr = NULL;

    char *a = strtok_r(copy, ",", &saveptr);
    char *w = strtok_r(NULL, ",", &saveptr);
    char *h = strtok_r(NULL, ",", &saveptr);

    if (!a || !w || !h) {
        free(copy);
        return -1;
    }

    errno = 0;

    unsigned long long address =
        strtoull(a, NULL, 0);

    unsigned long width =
        strtoul(w, NULL, 0);

    unsigned long height =
        strtoul(h, NULL, 0);

    if (errno != 0 ||
        address == 0 ||
        width == 0 ||
        height == 0 ||
        width > UINT32_MAX ||
        height > UINT32_MAX) {

        free(copy);
        return -1;
    }

    config->address = (uintptr_t)address;
    config->width = (uint32_t)width;
    config->height = (uint32_t)height;

    free(copy);

    return 0;
}

static int parse_int(
    const char *value,
    int *output)
{
    char *end = NULL;

    errno = 0;

    long value_long =
        strtol(value, &end, 0);

    if (errno != 0 ||
        end == value ||
        *end != '\0' ||
        value_long < INT32_MIN ||
        value_long > INT32_MAX) {

        return -1;
    }

    *output = (int)value_long;

    return 0;
}

static int option(
    FBPrinterConfig *config,
    const char *arg)
{
    const char *value;

    if (strncmp(arg, "--text-size=", 12) == 0) {

        value = arg + 12;

        int n;

        if (parse_int(value, &n) != 0 ||
            n < 1 ||
            n > 99)
            return -1;

        config->text_size = n;
        return 0;
    }

    if (strncmp(arg, "--img-size=", 11) == 0) {

        value = arg + 11;

        int n;

        if (parse_int(value, &n) != 0 ||
            n < 1 ||
            n > 99)
            return -1;

        config->img_size = n;
        return 0;
    }

    if (strncmp(arg, "--text-x=", 9) == 0)
        return parse_int(arg + 9, &config->text_x);

    if (strncmp(arg, "--text-y=", 9) == 0)
        return parse_int(arg + 9, &config->text_y);

    if (strncmp(arg, "--image-x=", 10) == 0)
        return parse_int(arg + 10, &config->image_x);

    if (strncmp(arg, "--image-y=", 10) == 0)
        return parse_int(arg + 10, &config->image_y);

    if (strcmp(arg, "--keep-leftover") == 0) {
        config->keep_leftover = 1;
        return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    FBPrinterConfig config;

    fbprinter_config_init(&config);

    const char *config_file = NULL;

    for (int i = 1; i < argc; ++i) {

        if (strncmp(argv[i], "--config=", 9) == 0)
            config_file = argv[i] + 9;
    }

    if (config_file) {

        if (ini_load(
                &config,
                config_file) != 0)
            return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {

        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            usage(argv[0]);
            free(config.input_file);
            return EXIT_SUCCESS;
        }

        if (strncmp(arg, "--config=", 9) == 0)
            continue;

        if (arg[0] != '-' &&
            strchr(arg, ',') != NULL) {

            if (parse_framebuffer(
                    &config,
                    arg) != 0) {

                fprintf(
                    stderr,
                    "Invalid framebuffer: %s\n",
                    arg
                );

                free(config.input_file);
                return EXIT_FAILURE;
            }

            continue;
        }

        if (arg[0] != '-') {

            free(config.input_file);

            config.input_file =
                strdup(arg);

            if (!config.input_file) {
                free(config.input_file);
                return EXIT_FAILURE;
            }

            continue;
        }

        int result =
            option(&config, arg);

        if (result != 0) {

            fprintf(
                stderr,
                "Invalid/unknown option: %s\n",
                arg
            );

            free(config.input_file);
            return EXIT_FAILURE;
        }
    }

    if (!config.address ||
        !config.width ||
        !config.height ||
        !config.input_file) {

        usage(argv[0]);

        free(config.input_file);

        return EXIT_FAILURE;
    }

    if (fbprinter_open(&config) != 0) {

        free(config.input_file);

        return EXIT_FAILURE;
    }

    if (!config.keep_leftover)
        fbprinter_clear(&config);

    int result =
        fbprinter_render(
            &config,
            config.input_file
        );

    fbprinter_close(&config);

    free(config.input_file);

    return result == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
