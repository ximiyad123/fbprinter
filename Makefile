CC ?= gcc
AR ?= ar
RANLIB ?= ranlib

CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS += -Iinclude -D_GNU_SOURCE
LDFLAGS ?=

LDLIBS += -lpng -ljpeg -lgif -lz -lm


# ============================================================
# Directories
# ============================================================

BUILD_DIR := build

FULL_STATIC_DIR    := $(BUILD_DIR)/full-static
STATIC_BIN_DIR     := $(BUILD_DIR)/static-bin-library
DYNAMIC_DIR        := $(BUILD_DIR)/dynamic
LIBRARY_DIR        := $(BUILD_DIR)/library
STATIC_LIBRARY_DIR := $(BUILD_DIR)/static-library

OBJ_DIR := $(BUILD_DIR)/obj


# ============================================================
# Sources
# ============================================================

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


# ============================================================
# Targets
# ============================================================

.PHONY: all \
	full-static \
	static-bin-library \
	dynamic \
	library \
	static-library \
	clean


# ============================================================
# Default target
#
# "make" builds the static-bin-library version.
# ============================================================

all: static-bin-library


# ============================================================
# Architecture selection
# ============================================================

ifeq ($(ARCH),arm64)

CC := aarch64-linux-gnu-gcc

CFLAGS += -march=armv8-a

endif


ifeq ($(ARCH),arm)

CC := arm-linux-gnueabihf-gcc

CFLAGS += -march=armv7-a

endif


# ============================================================
# DYNAMIC
#
# Output:
#
# build/dynamic/
# ├── fbprinter
# └── libfbprinter.so
#
# fbprinter dynamically links against libfbprinter.so.
# ============================================================

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
		-Wl,-rpath,'$$ORIGIN' \
		$(LDLIBS)


$(DYNAMIC_DIR)/libfbprinter.so: $(LIB_OBJECTS)

	@mkdir -p $(DYNAMIC_DIR)

	$(CC) -shared \
		-Wl,-soname,libfbprinter.so.1 \
		-o $@ \
		$^ \
		$(LDLIBS)


# ============================================================
# LIBRARY
#
# Output:
#
# build/library/
# └── libfbprinter.so
#
# Only builds the shared library.
# ============================================================

library: $(LIBRARY_DIR)/libfbprinter.so


$(LIBRARY_DIR)/libfbprinter.so: $(LIB_OBJECTS)

	@mkdir -p $(LIBRARY_DIR)

	$(CC) -shared \
		-Wl,-soname,libfbprinter.so.1 \
		-o $@ \
		$^ \
		$(LDLIBS)


# ============================================================
# STATIC LIBRARY
#
# Output:
#
# build/static-library/
# └── libfbprinter.a
#
# Only builds the static library.
# ============================================================

static-library: $(STATIC_LIBRARY_DIR)/libfbprinter.a


$(STATIC_LIBRARY_DIR)/libfbprinter.a: $(LIB_OBJECTS)

	@mkdir -p $(STATIC_LIBRARY_DIR)

	$(AR) rcs $@ $^

	$(RANLIB) $@


# ============================================================
# STATIC-BIN-LIBRARY
#
# Output:
#
# build/static-bin-library/
# └── fbprinter
#
# libfbprinter.a is linked INTO the executable.
#
# libfbprinter.so is NOT required.
#
# System libraries such as libc, libpng, libjpeg,
# giflib and zlib remain dynamically linked.
# ============================================================

static-bin-library: \
	$(STATIC_BIN_DIR)/fbprinter


$(STATIC_BIN_DIR)/fbprinter: \
	$(CLI_OBJECT) \
	$(STATIC_LIBRARY_DIR)/libfbprinter.a

	@mkdir -p $(STATIC_BIN_DIR)

	$(CC) $(CFLAGS) $(LDFLAGS) \
		-o $@ \
		$(CLI_OBJECT) \
		$(STATIC_LIBRARY_DIR)/libfbprinter.a \
		$(LDLIBS)


# ============================================================
# FULL STATIC
#
# Output:
#
# build/full-static/
# └── fbprinter
#
# Attempts to statically link EVERYTHING.
#
# This requires static versions of all dependencies to be
# installed in the build environment.
# ============================================================

full-static: \
	$(FULL_STATIC_DIR)/fbprinter


$(FULL_STATIC_DIR)/fbprinter: \
	$(CLI_OBJECT) \
	$(STATIC_LIBRARY_DIR)/libfbprinter.a

	@mkdir -p $(FULL_STATIC_DIR)

	$(CC) $(CFLAGS) -static \
		-o $@ \
		$(CLI_OBJECT) \
		$(STATIC_LIBRARY_DIR)/libfbprinter.a \
		$(LDLIBS)


# ============================================================
# C OBJECT FILES
# ============================================================

$(OBJ_DIR)/%.o: src/%.c

	@mkdir -p $(OBJ_DIR)

	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-fPIC \
		-c $< \
		-o $@


# ============================================================
# CLI OBJECT
# ============================================================

$(CLI_OBJECT): cli/main.c

	@mkdir -p $(OBJ_DIR)

	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-c $< \
		-o $@


# ============================================================
# CLEAN
# ============================================================

clean:

	rm -rf $(BUILD_DIR)
