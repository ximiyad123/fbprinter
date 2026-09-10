#!/usr/bin/env python3

from pathlib import Path

ROOT = Path("fbprinter")

directories = [
    ROOT / "include",
    ROOT / "src",
]

files = [
    ROOT / "Makefile",
    ROOT / "config.ini",
    ROOT / "font.h",

    ROOT / "include" / "fbprinter.h",
    ROOT / "include" / "framebuffer.h",
    ROOT / "include" / "renderer.h",
    ROOT / "include" / "image.h",
    ROOT / "include" / "text.h",
    ROOT / "include" / "png.h",
    ROOT / "include" / "jpg.h",
    ROOT / "include" / "gif.h",
    ROOT / "include" / "ini_parser.h",

    ROOT / "src" / "fbprinter.c",
    ROOT / "src" / "framebuffer.c",
    ROOT / "src" / "renderer.c",
    ROOT / "src" / "image.c",
    ROOT / "src" / "text.c",
    ROOT / "src" / "png.c",
    ROOT / "src" / "jpg.c",
    ROOT / "src" / "gif.c",
    ROOT / "src" / "ini_parser.c",
]

for directory in directories:
    directory.mkdir(parents=True, exist_ok=True)

for file in files:
    if not file.exists():
        file.touch()
        print(f"created: {file}")
    else:
        print(f"exists:  {file}")

print()
print("fbprinter skeleton created.")
