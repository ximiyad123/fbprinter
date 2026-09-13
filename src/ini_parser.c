#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ini_parser.h"

static char *trim(char *str)
{
    while (isspace((unsigned char)*str))
        str++;

    char *end = str + strlen(str);

    while (end > str &&
           isspace((unsigned char)*(end - 1))) {
        end--;
    }

    *end = '\0';

    return str;
}

static int parse_bool(const char *value)
{
    if (!value)
        return 0;

    if (strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 ||
        strcmp(value, "1") == 0 ||
        strcasecmp(value, "on") == 0) {
        return 1;
    }

    return 0;
}

static void parse_value(
    FBPrinterConfig *config,
    const char *section,
    const char *key,
    const char *value)
{
    if (strcmp(section, "framebuffer") == 0) {

        if (strcmp(key, "address") == 0) {
            config->address =
                (uintptr_t)strtoull(value, NULL, 0);
        }
        else if (strcmp(key, "width") == 0) {
            config->width =
                (uint32_t)strtoul(value, NULL, 0);
        }
        else if (strcmp(key, "height") == 0) {
            config->height =
                (uint32_t)strtoul(value, NULL, 0);
        }

    }
    else if (strcmp(section, "renderer") == 0) {

        /*
         * Backwards-compatible scale option.
         *
         * New configs can use text_size and img_size
         * independently.
         */
        if (strcmp(key, "scale") == 0) {
            int scale =
                (int)strtol(value, NULL, 0);

            config->text_size = scale;
            config->img_size = scale;
        }
        else if (strcmp(key, "text_size") == 0) {
            config->text_size =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "img_size") == 0) {
            config->img_size =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "text_x") == 0) {
            config->text_x =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "text_y") == 0) {
            config->text_y =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "image_x") == 0) {
            config->image_x =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "image_y") == 0) {
            config->image_y =
                (int)strtol(value, NULL, 0);
        }
        else if (strcmp(key, "keep_leftover") == 0) {
            config->keep_leftover =
                parse_bool(value);
        }

    }
    else if (strcmp(section, "input") == 0) {

        if (strcmp(key, "file") == 0) {
            free(config->input_file);

            config->input_file =
                strdup(value);
        }
    }
}

int ini_load(
    FBPrinterConfig *config,
    const char *filename)
{
    if (!config || !filename)
        return -1;

    FILE *file = fopen(filename, "r");

    if (!file) {
        perror("fopen config");
        return -1;
    }

    char line[1024];
    char section[128] = "";

    unsigned long line_number = 0;

    while (fgets(line, sizeof(line), file)) {

        line_number++;

        char *text = trim(line);

        /*
         * Ignore empty lines.
         */
        if (*text == '\0')
            continue;

        /*
         * Ignore comments.
         */
        if (*text == '#' ||
            *text == ';')
            continue;

        /*
         * Section:
         *
         * [framebuffer]
         */
        if (*text == '[') {

            char *end =
                strchr(text, ']');

            if (!end) {
                fprintf(
                    stderr,
                    "%s:%lu: invalid section\n",
                    filename,
                    line_number
                );

                fclose(file);
                return -1;
            }

            *end = '\0';

            strncpy(
                section,
                text + 1,
                sizeof(section) - 1
            );

            section[
                sizeof(section) - 1
            ] = '\0';

            continue;
        }

        /*
         * key=value
         */
        char *equals =
            strchr(text, '=');

        if (!equals) {
            fprintf(
                stderr,
                "%s:%lu: expected key=value\n",
                filename,
                line_number
            );

            fclose(file);
            return -1;
        }

        *equals = '\0';

        char *key =
            trim(text);

        char *value =
            trim(equals + 1);

        if (*key == '\0')
            continue;

        parse_value(
            config,
            section,
            key,
            value
        );
    }

    fclose(file);

    return 0;
}
