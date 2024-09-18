# Format
    # byte:   type
    # nybble: bits per palette index
    # nybble: bits per run length
    # nybble: bits per address set
    # 12 bits: number of instructions
    # 2 ^ (bits per palette index) bytes: palette entries 

# Instruction
    # bit: 0 if draw colors, 1 if set address
    # if draw colors:
        # bits per run length: run length
        # bits per palette address: palette address
    # if set address:
        # bit: 0 if page, 1 if column
        # bits per address set: address start value
        # bits per address set: address end value

from PIL import Image
from sprite_util import *

class Instruction:
    def __init__(self, type):
        self.type = type

class DrawInstruction(Instruction):
    def __init__(self, type, len, palette_ind):
        super().__init__(type)
        self.len = len
        self.palette_ind = palette_ind

    def binary(self, lenBits, palBits):
        out = 0b0
        out += self.len << 1
        out += self.palette_ind << (lenBits + 1)
        return out

PAGE_ADDR = 0
COL_ADDR = 1
class AddressInstruction(Instruction):
    def __init__(self, type, which, start, end):
        super().__init__(type)
        self.which = which
        self.start = start
        self.end = end

    def binary(self, valBits):
        out = 0b1
        out += self.which << 1
        out += self.start << 2
        out += self.end << (2 + valBits)
        return out

class PalSpriteImage:
    def __init__(self, im_path = None, transparent = 0) -> None:
        self.transparent = transparent
        if im_path:
            self.im_path = im_path
            self.from_im(im_path)

    def from_im(self, im_path):
        im = Image.open(im_path)
        self.width = im.width
        self.height = im.height
        pixels = im.load()
        runs = []
        palette = dict()
        transparent_clr = None
        # run through image and find the palette and runs of similar pixels
        run = {'b': -1, 'l': 0}
        for y in range(self.height):
            for x in range(self.width):
                byte, p = closest565(pixels[x,y])
                transparent = isTransparent(pixels[x,y])
                if transparent:
                    byte = -1
                elif self.transparent and not transparent_clr:
                    transparent_clr = byte
                    byte = -1
                elif self.transparent and transparent_clr == byte:
                    byte = -1
                else:
                    if byte not in palette:
                        palette[byte] = len(palette)
                if run['b'] == byte:
                    run['l'] += 1
                else:
                    if run['l']:
                        runs.append(run.copy())
                    run = {'b': byte, 'l': 1}
        if run['l']:
            runs.append(run.copy())

        self.palette = []
        for i in range(0, len(palette)):
            for key, val in palette.items():
                if val == i:
                    self.palette.append(key)

        # create a list of instructions to draw the image based on the runs
        # this seems like it could be cleverer but it was surprisingly hard to think about
        self.instructions = []
        column_addr = 0
        row_addr = 0
        cursor_x = -1
        cursor_y = -1
        expected_x = 0
        expected_y = 0
        r = runs[0]
        runs = runs[1:]
        while len(runs) or r['l']:
            if r['l'] == 0:
                r = runs[0]
                runs = runs[1:]
            if r['b'] == -1:
                expected_x += r['l']
                r['l'] = 0
                while expected_x >= self.width:
                    expected_y += 1
                    expected_x -= self.width
            else:
                if cursor_x != expected_x or cursor_y != expected_y:
                    self.instructions.append(AddressInstruction(1, 0, expected_y, self.height))
                    self.instructions.append(AddressInstruction(1, 1, expected_x, self.width))
                    column_addr = expected_x
                    cursor_x = expected_x
                    row_addr = expected_y
                    cursor_y = expected_y
                adv = min(self.width - cursor_x, r['l'])
                r['l'] -= adv
                self.instructions.append(DrawInstruction(0, adv, palette[r['b']]))
                cursor_x += adv
                expected_x += adv
                if cursor_x == self.width:
                    cursor_y += 1
                    expected_y += 1
                    cursor_x = column_addr
                    expected_x = 0

        # Remove/combine instructions when able
        instr_ind = 0
        col_set = -1
        row_set = -1
        drawing = -1
        while instr_ind < len(self.instructions):
            instr = self.instructions[instr_ind]
            if instr.type == 0:
                col_set = -1
                row_set = -1
                if drawing >= 0 and self.instructions[drawing].palette_ind == instr.palette_ind:
                    self.instructions[drawing].len += instr.len
                    self.instructions = self.instructions[:instr_ind] + self.instructions[instr_ind+1:]
                else:
                    drawing = instr_ind
                    instr_ind += 1
            else:
                drawing = -1
                if instr.which == 0:
                    if row_set >= 0:
                        self.instructions = self.instructions[:row_set] + self.instructions[row_set + 1:]
                    else:
                        row_set = instr_ind
                        instr_ind += 1
                elif instr.which == 1:
                    if col_set >= 0:
                        self.instructions = self.instructions[:col_set] + self.instructions[col_set + 1:]
                    else:
                        col_set = instr_ind
                        instr_ind += 1

        # Find bits per element
        self.bitsPerPaletteInd = bitsToHold(len(palette) - 1)
        longestRun = 0
        largestAddressSet = 0
        for instr in self.instructions:
            if instr.type == 0:
                longestRun = max(longestRun, instr.len)
            elif instr.type == 1:
                largestAddressSet = max(largestAddressSet, instr.end)
        self.bitsPerRunLength = bitsToHold(longestRun)
        self.bitsPerAddressSet = bitsToHold(largestAddressSet)
        self.numInstructions = len(self.instructions)

        # Make the byte array
        self.data = [0]
        self.data.append(self.bitsPerPaletteInd + (self.bitsPerRunLength << 4))
        self.data.append(self.bitsPerAddressSet + ((self.numInstructions & 0b1111) << 4))
        self.data.append(self.numInstructions >> 4)
        for i in range(0, len(palette)):
            for key, val in palette.items():
                if val == i:
                    self.data.append(key)
        for i in range(len(palette), 2 ** self.bitsPerPaletteInd):
            self.data.append(0)

        instr_ind = 0
        byte = 0
        shift = 0
        while instr_ind < len(self.instructions):
            instr = self.instructions[instr_ind]
            instr_ind += 1
            if instr.type == 0:
                bits = 1 + self.bitsPerRunLength + self.bitsPerPaletteInd
                data = instr.binary(self.bitsPerRunLength, self.bitsPerPaletteInd)
            elif instr.type == 1:
                bits = 2 + self.bitsPerAddressSet * 2
                data = instr.binary(self.bitsPerAddressSet)
            else:
                print ("unknown instr type")
            bit = 0
            while bits > 0:
                byte += (1 if (data & (1 << bit)) else 0) << shift
                bit += 1
                bits -= 1
                shift += 1
                if shift == 8:
                    self.data.append(byte)
                    shift = 0
                    byte = 0
        if shift:
            self.data.append(byte)

def imgFromPalSpriteData (im_path, data):
    im = Image.new("RGBA", (240, 320))
    bitsPerPaletteInd = data[1] & 0b1111
    # print(bitsPerPaletteInd)
    bitsPerRunLen = data[1] >> 4
    # print(bitsPerRunLen)
    bitsPerAddrSet = data[2] & 0b1111
    # print(bitsPerAddrSet)
    numInstructions = (data[2] >> 4) + (data[3] << 4)
    # print(numInstructions)
    pos = 4
    palette = []
    for i in range(2 ** bitsPerPaletteInd):
        palette.append(byteToRGB(data[pos]))
        pos += 1 
    buffer = data[pos] + (data[pos+1] << 8) + (data[pos+2] << 16) + (data[pos+3] << 24)
    pos += 4
    buffer_left = 32
    row_start = 300
    row_end = 0
    col_start = 300
    col_end = 0
    x = 0
    y = 0
    while numInstructions:
        numInstructions -= 1
        while buffer_left < 24:
            buffer += (data[pos] if pos < len(data) else 0) << buffer_left
            buffer_left += 8
            pos += 1
        type = buffer & 1
        buffer >>= 1
        buffer_left -= 1
        if type == 0:
            l = buffer & ((1 << bitsPerRunLen) - 1)
            buffer >>= bitsPerRunLen
            buffer_left -= bitsPerRunLen
            p = buffer & ((1 << bitsPerPaletteInd) - 1)
            buffer >>= bitsPerPaletteInd
            buffer_left -= bitsPerPaletteInd
            while l:
                im.putpixel((x, y), palette[p])
                x += 1
                if x >= col_end:
                    x = col_start
                    y += 1
                l -= 1
        else:
            which = buffer & 1
            buffer >>= 1
            buffer_left -= 1
            start = buffer & ((1 << bitsPerAddrSet) - 1)
            buffer >>= bitsPerAddrSet
            end = buffer & ((1 << bitsPerAddrSet) - 1)
            buffer >>= bitsPerAddrSet
            buffer_left -= bitsPerAddrSet * 2
            if which == 0:
                row_start = start
                y = start
                row_end = end
            else:
                col_start = start
                x = start
                col_end = end
    im = im.crop((0, 0, col_end, row_end))
    im.save(im_path)

def palSpriteToH (im_path, h_path, transparent = 0):
    sprite = PalSpriteImage(im_path, transparent)
    data = sprite.data
    h_dir = '/'.join(h_path.split('/')[:-1])
    h_name = h_path.split('/')[-1][:-2]
    f = open(h_path, 'w')
    f.write("#pragma once\n\n")
    f.write("const uint8_t " + h_name.upper() + "[] PROGMEM = {\n")
    f.write("    " + ','.join([str(x) for x in data]) + '\n')
    f.write("};")
    f.close()
    imgFromPalSpriteData(h_dir + '/' + h_name + '_render.png', data)

def palSwapSpriteToH (im_paths, h_path):
    sprite = PalSpriteImage(im_paths[0])
    data = sprite.data
    data = data[:4] + data[4 + 2 ** sprite.bitsPerPaletteInd:]
    h_dir = '/'.join(h_path.split('/')[:-1])
    h_name = h_path.split('/')[-1][:-2]
    f = open(h_path, 'w')
    f.write("#pragma once\n\n")
    f.write("const uint8_t " + h_name.upper() + "[] PROGMEM = {\n")
    f.write("    " + ','.join([str(x) for x in data]) + '\n')
    f.write("};")
    for im_path in im_paths:
        sprite = PalSpriteImage(im_path)
        im_name = im_path.split('/')[-1].split('.')[0]
        pal = sprite.palette
        f.write("\n\n")
        f.write("const uint8_t " + im_name.upper() + "[] PROGMEM = {\n")
        f.write("    " + ','.join([str(x) for x in pal]) + '\n')
        f.write("};")
        data_ = data[:4] + pal + data[4:]
        imgFromPalSpriteData(h_dir + '/' + im_name + '_render.png', data_)
    f.close()