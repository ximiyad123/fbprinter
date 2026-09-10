CC ?= gcc

CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
CPPFLAGS += -Iinclude
LDFLAGS ?=
LDLIBS += -lpng -ljpeg -lgif -lz -lm

TARGET := fbprinter

SOURCES := \
	src/fbprinter.c \
	src/framebuffer.c \
	src/renderer.c \
	src/image.c \
	src/text.c \
	src/png.c \
	src/jpg.c \
	src/gif.c \
	src/ini_parser.c

OBJECTS := $(SOURCES:.c=.o)

# Architecture selection
ifeq ($(ARCH),arm64)
	CC := aarch64-linux-gnu-gcc
	CFLAGS += -march=armv8-a
endif

ifeq ($(ARCH),arm)
	CC := arm-linux-gnueabihf-gcc
	CFLAGS += -march=armv7-a
endif

# Static linking
ifeq ($(STATIC),true)
	LDFLAGS += -static
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
