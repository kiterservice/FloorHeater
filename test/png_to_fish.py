#!/usr/bin/env python3
"""
Конвертирует спрайт-лист PNG в C-заголовок с массивами RGB565.

Использование:
    python3 png_to_fish.py sprites.png fish_data.h

Скрипт ожидает, что у вас есть координаты верхнего левого угла
каждого цвета. По умолчанию берётся сетка 4 кадра в столбце.
Отредактируйте FISH_SOURCES под свою картинку.
"""
from PIL import Image
import sys

SPRITE_W = 50
SPRITE_H = 50
FRAMES   = 2

# (col_start, row_start) в пикселях — откуда начинается группа 4 кадров.
# Отредактируйте под свою картинку: смотрите координаты верхнего левого
# кадра нужного цвета.
FISH_SOURCES = [
    (0,   0,   "grass"),    # верхний левый угол 1-й рыбки
    #(0,   512, "fish_green"),   # подставьте реальные координаты
    #(256,   1024,"fish_red"),     # подставьте реальные координаты
]

def rgb888_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def write_fish(out, img, col0, row0, name):
    out.write(f"// {name}: 4 frames at ({col0},{row0})\n")
    for frame in range(FRAMES):
        out.write(f"const uint16_t {name}_f{frame}[{SPRITE_W*SPRITE_H}] = {{\n")
        for y in range(SPRITE_H):
            out.write("    ")
            for x in range(SPRITE_W):
                px = img.getpixel((col0 + frame * SPRITE_W + x, row0 + y))
                if len(px) == 4:
                    r, g, b, a = px
                    if a < 128:
                        c = 0x0000   # прозрачный → чёрный (или спец-ключ)
                    else:
                        c = rgb888_to_565(r, g, b)
                else:
                    r, g, b = px[:3]
                    c = rgb888_to_565(r, g, b)
                out.write(f"0x{c:04X},")
            out.write("\n")
        out.write("};\n\n")

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    img = Image.open(sys.argv[1]).convert("RGBA")
    print(f"Image: {img.size[0]}x{img.size[1]}")

    with open(sys.argv[2], "w") as f:
        f.write("// Auto-generated from sprites.png — do not edit\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        for col, row, name in FISH_SOURCES:
            write_fish(f, img, col, row, name)

if __name__ == "__main__":
    main()