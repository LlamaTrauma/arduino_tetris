from PIL import Image
import re
import os

TAB = "    "

def byteToRGB (i):
    r_ = ((i & 0b11111000) >> 3) * 255 / 0b11111
    g_ = (((i & 0b00000111) << 3) + ((i & 0b11100000) >> 5)) * 255 / 0b111111
    b_ = (i & 0b00011111) * 255 / 0b11111
    r_ = int(r_)
    g_ = int(g_)
    b_ = int(b_)
    return (r_, g_, b_)

def closest565 (pixel):
    r, g, b = pixel[:3]
    closest = (0, 0, 0)
    closest_i = -1
    score = -1
    for i in range(256):
        r_, g_, b_ = byteToRGB(i)
        score_ = (r - r_) ** 2 + (g - g_) ** 2 + (b - b_) ** 2
        if (score == -1 or score_ < score):
            score = score_
            closest = (r_, g_, b_)
            closest_i = i
    return closest_i, closest

class Page:
    def __init__(self, y=0) -> None:
        self.y = y
        self.groups = []
    def toStr(self):
        s = ''
        # s += TAB + "// new page\n"
        # s += TAB + "// page x\n"
        s += TAB + str(self.y) + ",\n"
        # s += TAB + "// group count\n"
        s += TAB + str(len(self.groups)) + ",\n"
        for g in self.groups:
            s += g.toStr()
        return s

class Group:
    def __init__(self, x=0) -> None:
        self.x = x
        self.runs = []
    def toStr(self):
        s = ''
        # s += TAB * 2 + "// new group\n"
        # s += TAB * 2 + "// page y\n"
        s += TAB * 2 + str(self.x) + ",\n"
        # s += TAB * 2 + "// run count\n"
        s += TAB * 2 + str(len(self.runs)) + ",\n"
        for r in self.runs:
            s += r.toStr()
        return s

class Run:
    def __init__(self, count=0, color=0) -> None:
        self.count = count
        self.color = color
    def toStr(self):
        s = ''
        # s += TAB * 3 + "// new run\n"
        # s += TAB * 3 + "// color\n"
        s += TAB * 3 + str(self.color) + ",\n"
        # s += TAB * 3 + "// repeat\n"
        s += TAB * 3 + str(self.count) + ",\n"
        return s

def transparent (pixel):
    return len(pixel) == 4 and pixel[3] < 128

currentRun = Run()
currentGroup = Group()
currentPage = Page()
pages = []

def flushRun():
    global currentRun
    global currentGroup
    global currentPage
    global pages
    if currentRun.count:
        currentGroup.runs.append(currentRun)
    currentRun = Run()

def flushGroup():
    global currentRun
    global currentGroup
    global currentPage
    global pages
    flushRun()
    if len(currentGroup.runs):
        currentPage.groups.append(currentGroup)
    currentGroup = Group()

def flushPage():
    global currentRun
    global currentGroup
    global currentPage
    global pages
    flushGroup()
    if len(currentPage.groups):
        pages.append(currentPage)
    currentPage = Page()

def img_to_h(img_path, h_path, var_name):
    global pages
    pages = []
    im = Image.open(img_path)
    pix = im.load()
    w, h = im.size
    print(w, h)

    for i in range(h):
        currentPage.y = i
        print(i)
        for j in range(w):
            byte, new_pixel = closest565(pix[j,i])
            # print (new_pixel)
            if transparent(pix[j,i]):
                flushGroup()
                currentGroup.x = j + 1
            elif (abs(byte - currentRun.color) > 0):
                flushRun()
                currentRun.color = byte
                currentRun.count = 1
            else:
                currentRun.count += 1
            if j == w - 1:
                flushPage()

    im.close()

    f = open(h_path, 'w')
    f.write("#pragma once\n")
    f.write("const uint8_t " + var_name + "[] PROGMEM = {\n")
    # f.write(TAB + "// page count\n")
    f.write(TAB + str(len(pages)) + ',\n')
    for p in pages:
        f.write(p.toStr())
    f.write("};")
    f.close()
    return (w, h)

nums = []
pos = 0
def h_to_img (h_path, img_path, w, h):
    global nums
    global pos
    # Now recreate it
    f = open(h_path, 'r')
    lines = f.readlines()
    f.close()
    nums = [int(l[:l.find(',')]) for l in lines[2:-1]]

    pos = 0
    def next():
        global nums
        global pos
        out = nums[pos]
        pos += 1
        return out

    im = Image.new(mode="RGBA", size=(w, h))

    pages = next()
    while (pages):
        pages -= 1
        y = next()
        groups = next()
        while (groups):
            groups -= 1
            x = next()
            runs = next()
            while (runs):
                runs -= 1
                color = byteToRGB(next())
                count = next()
                while (count):
                    count -= 1
                    im.putpixel((x, y), color)
                    x += 1

    im.save(img_path)

img_paths = os.listdir('new')
for path in img_paths:
    name = path.split('.')[0]
    print(name)
    img_path = "new/"+path
    h_path = "added_h/"+name+".h"
    new_img_path = "added_img/"+name+".png"
    w, h = img_to_h(img_path, h_path, name.upper())
    h_to_img(h_path, new_img_path, w, h)