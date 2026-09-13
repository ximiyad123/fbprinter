#ifndef INI_PARSER_H
#define INI_PARSER_H

#include "fbprinter.h"

int ini_load(
    FBPrinterConfig *config,
    const char *filename
);

#endif
