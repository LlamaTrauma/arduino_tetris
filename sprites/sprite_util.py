import math

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

def isTransparent (pixel):
    return len(pixel) == 4 and pixel[3] < 128

def bitsToHold(n):
    if n < 1:
        return 1
    return math.floor(math.log2(n)) + 1