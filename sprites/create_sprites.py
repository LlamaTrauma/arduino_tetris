import os
from pal_sprite import *
from font import *
import json

dirs = [f for f in os.listdir() if os.path.isdir(f)]

for dir in dirs:
    dir_name = dir + '/'
    src_name = dir_name + "src/"
    if not os.path.exists(src_name):
        continue
    todo = json.load(open(src_name + "src.json", 'r'))
    for sprite in todo['sprites']:
        if sprite["type"] == "instruction":
            if "transparent" in sprite:
                palSpriteToH(src_name + sprite["src"], dir_name + sprite["name"] + ".h", transparent = 1)
            else:
                palSpriteToH(src_name + sprite["src"], dir_name + sprite["name"] + ".h")
        elif sprite["type"] == "instruction_palettes":
            palSwapSpriteToH([src_name + x for x in sprite["src"]], dir_name + sprite["name"] + ".h")
        elif sprite["type"] == "font":
            fontToH(src_name + sprite["src"], dir_name + sprite["name"] + ".h", sprite["chars"])    