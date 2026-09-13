CC ?= gcc
AR ?= ar
RANLIB ?= ranlib

CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS += -Iinclude -D_GNU_SOURCE
LDFLAGS ?=

LDLIBS += -lpng -ljpeg -lgif -lz -lm

BUILD_DIR := build

DYNAMIC_DIR := $(BUILD_DIR)/dynamic
STATIC_DIR  := $(BUILD_DIR)/static
LIBRARY_DIR := $(BUILD_DIR)/library

OBJ_DIR := $(BUILD_DIR)/obj

# PIC static third-party libraries built by Dockerfile
PIC_DEPS := /opt/fbprinter-deps/lib

LIB_SOURCES := \
	src/libfbprinter.c \
	src/framebuffer.c \
	src/renderer.c \
	src/image.c \
	src/text.c \
	src/png.c \
	src/jpg.c \
	src/gif.c \
	src/ini_parser.c

LIB_OBJECTS := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIB_SOURCES))

CLI_OBJECT := $(OBJ_DIR)/main.o


.PHONY: all \
        dynamic \
        static \
        library-only \
        clean


# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

all: dynamic


# ------------------------------------------------------------
# Cross compilation
# ------------------------------------------------------------

ifeq ($(ARCH),arm64)
CC := aarch64-linux-gnu-gcc
CFLAGS += -march=armv8-a
endif

ifeq ($(ARCH),arm)
CC := arm-linux-gnueabihf-gcc
CFLAGS += -march=armv7-a
endif


# ------------------------------------------------------------
# Dynamic build
#
# build/dynamic/
# ├── fbprinter
# └── libfbprinter.so
#
# libfbprinter.so contains the third-party libraries statically:
#   libpng
#   libjpeg-turbo
#   giflib
#   zlib
#
# Runtime dependency:
#   libc
# ------------------------------------------------------------

dynamic: \
	$(DYNAMIC_DIR)/fbprinter \
	$(DYNAMIC_DIR)/libfbprinter.so


$(DYNAMIC_DIR)/fbprinter: \
	$(CLI_OBJECT) \
	$(DYNAMIC_DIR)/libfbprinter.so

	@mkdir -p $(DYNAMIC_DIR)

	$(CC) $(CFLAGS) $(LDFLAGS) \
		-o $@ \
		$(CLI_OBJECT) \
		-L$(DYNAMIC_DIR) \
		-lfbprinter \
		-Wl,-rpath,'$$ORIGIN'


$(DYNAMIC_DIR)/libfbprinter.so: $(LIB_OBJECTS)

	@mkdir -p $(DYNAMIC_DIR)

	$(CC) -shared \
		-Wl,-soname,libfbprinter.so.1 \
		-o $@ \
		$^ \
		-L$(PIC_DEPS) \
		-Wl,-Bstatic \
		-lpng \
		-ljpeg \
		-lgif \
		-lz \
		-Wl,-Bdynamic \
		-lc \
		-lm


# ------------------------------------------------------------
# Fully static build
#
# build/static/
# └── fbprinter
# ------------------------------------------------------------

static: \
	$(STATIC_DIR)/fbprinter


$(STATIC_DIR)/fbprinter: \
	$(CLI_OBJECT) \
	$(LIB_OBJECTS)

	@mkdir -p $(STATIC_DIR)

	$(CC) $(CFLAGS) -static \
		-o $@ \
		$(CLI_OBJECT) \
		$(LIB_OBJECTS) \
		-L$(PIC_DEPS) \
		-lpng \
		-ljpeg \
		-lgif \
		-lz \
		-lm


# ------------------------------------------------------------
# Library only
#
# build/library/
# └── libfbprinter.so
# ------------------------------------------------------------

library-only: \
	$(LIBRARY_DIR)/libfbprinter.so


$(LIBRARY_DIR)/libfbprinter.so: $(LIB_OBJECTS)

	@mkdir -p $(LIBRARY_DIR)

	$(CC) -shared \
		-Wl,-soname,libfbprinter.so.1 \
		-o $@ \
		$^ \
		-L$(PIC_DEPS) \
		-Wl,-Bstatic \
		-lpng \
		-ljpeg \
		-lgif \
		-lz \
		-Wl,-Bdynamic \
		-lc \
		-lm


# ------------------------------------------------------------
# Library object files
# ------------------------------------------------------------

$(OBJ_DIR)/%.o: src/%.c

	@mkdir -p $(OBJ_DIR)

	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-fPIC \
		-c $< \
		-o $@


# ------------------------------------------------------------
# CLI object
# ------------------------------------------------------------

$(CLI_OBJECT): cli/main.c

	@mkdir -p $(OBJ_DIR)

	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-c $< \
		-o $@


# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)
