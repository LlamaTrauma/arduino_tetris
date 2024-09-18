import re
from PIL import Image
import math
import os

class Font:
    def __init__(self, font_path, chars):
        self.font_path = font_path
        self.chars = chars
        self.fromPath(self.font_path)

    def fromPath(self, font_path):
        f = open(font_path)
        lines = f.readlines()
        f.close()

        progmem_lines = [x for x, line in enumerate(lines) if re.search(r"PROGMEM", line) is not None]
        empty_lines = [x for x, line in enumerate(lines) if len(line.strip()) == 0]

        bitmap_vals = lines[progmem_lines[0]+1:empty_lines[1]]
        bitmap_vals = [int(x, 16) for x in re.findall(r"0x.{2}", ''.join(bitmap_vals))]

        min_y = 0
        max_y = -999999
        max_w = 0

        self.glyphs = []
        glyph_vals = lines[progmem_lines[1]+1:empty_lines[2]]
        for line in glyph_vals:
            args = [int(x, 10) for x in re.findall(r"-?[0-9]+", line)][:6]
            char = re.findall(r"'(.)'", line)[0]
            g = Glyph(char, *args)
            self.glyphs.append(g)
            min_y = min(min_y, g.y)
            max_y = max(max_y, g.y + g.height)
            max_w = max(max_w, g.x + g.width)

        self.data_pointers = [len(self.glyphs)]
        self.glyph_data = []
        data_len = 1 + len(self.glyphs) * 2
        for glyph in self.glyphs:
            if re.match('['+self.chars+']', glyph.char):
                self.data_pointers.append(data_len & 0xFF)
                self.data_pointers.append((data_len >> 8) & 0xFF)
                data = glyph.data(bitmap_vals, min_y, max_y, max_w)
                self.glyph_data.extend(data)
                data_len += len(data)
            else:
                self.data_pointers.append(0)
                self.data_pointers.append(0)

        self.data = self.data_pointers + self.glyph_data

class Glyph:
    def __init__(self, char, addr, width, height, advance, x, y):
        self.char = char
        self.addr = addr
        self.width = width
        self.height = height
        self.advance = advance
        self.x = x
        self.y = y

    def str(self, bitmap):
        s = ''
        addr = self.addr
        shift = 7
        for y in range(self.height):
            row = ''
            for x in range(self.width):
                c = '#' if bitmap[addr] & (1 << shift) else ' '
                row += c
                shift -= 1
                if shift == -1:
                    shift = 7
                    addr += 1
            s += row + '\n'
        print(s)

    def data(self, bitmap, min_y = 0, max_y = 0, max_w = 0):
        self.data = []
        start_y = self.y - min_y
        end_y = start_y + self.height
        start_x = self.x
        end_x = start_x + self.width
        w = max_w
        h = max_y - min_y

        byte_len = (w * h) // 8
        if (w * h) % 8:
            byte_len += 1
        for i in range(byte_len):
            self.data.append(0)
        addr = self.addr
        shift = 7
        for y in range(start_y, end_y):
            for x in range(start_x, end_x):
                byte = (x + y * w) // 8
                bit = (x + y * w) % 8
                c = 1 if bitmap[addr] & (1 << shift) else 0
                self.data[byte] |= c << bit
                shift -= 1
                if shift == -1:
                    shift = 7
                    addr += 1
        return [2] + [w, h] + self.data

def fontDataToPngs (png_path, data):
    pos = 0
    num_glpyhs = data[0]
    pos = 1
    glyph_pointers = []
    if not os.path.exists(png_path):
        os.mkdir(png_path)
    for i in range(num_glpyhs):
        glyph_pointers.append((data[pos + 1] << 8) + data[pos])
        pos += 2
    for i, glyph_pointer in enumerate(glyph_pointers):
        if glyph_pointer == 0:
            continue
        w = data[glyph_pointer + 1]
        h = data[glyph_pointer + 2]
        val = i + 32
        im = Image.new("RGBA", (w,h))
        name = png_path + "glyph_" + str(val) + ".png"
        for y in range(h):
            for x in range(w):
                byte = (y * w + x) // 8
                bit = (y * w + x) % 8
                pixel = data[glyph_pointer + 3 + byte] & (1 << bit)
                if pixel:
                    im.putpixel((x,y), (0, 0, 0, 255))
                else:
                    im.putpixel((x,y), (0, 0, 0, 0))
        im.save(name)

def fontToH (font_path, h_path, chars):
    h_dir = '/'.join(h_path.split('/')[:-1]) + '/'
    h_name = h_path.split('/')[-1][:-2]
    font = Font(font_path, chars)
    f = open(h_path, 'w')
    f.write("#pragma once\n\n")
    f.write("const uint8_t " + h_name.upper() + "[] PROGMEM = {\n")
    f.write("    " + ", ".join([str(x) for x in font.data]) + "\n")
    f.write("};")
    f.close()
    fontDataToPngs(h_dir + h_name + "_glyphs/", font.data)