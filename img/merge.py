import os
import re
import natsort

f = open("merge.h", 'w')
f.write("#pragma once\n#include <stdint.h>\n\n")
f.write("const uint8_t GLYPHS[] PROGMEM = {\n")

to_merge = os.listdir("merge")
to_merge = natsort.natsorted(to_merge)
names = []
offsets = []
total_nums = 0
wanted = r"[A-Z:0-9 ]"

for fname in to_merge:
    fpath = "merge/" + fname
    print(fpath)
    f_ = open(fpath, 'r')
    lines = f_.readlines()
    f_.close()
    names.append(re.findall(r" ([a-zA-Z_0-9]*)\[\]", lines[1])[0])
    offsets.append(total_nums)
    char = chr(int(fname.split('_')[1][:-2]))
    if re.match(wanted, char) is None:
        continue
    num_lines = (line for line in lines[2:] if len(re.findall(r"[0-9]+,", line)))
    # print(len(list(num_lines)))
    nums = [int(re.findall(r"[0-9]+", x)[0]) for x in num_lines]
    f.write("    ")
    for num in nums:
        f.write(str(num))
        f.write(', ')
    total_nums += len(nums)
    f.write("\n")

f.write("};\n\n")
f.write("const uint16_t GLYPH_POINTERS[] PROGMEM = {\n")
for i, name in enumerate(names):
    f.write('    ' + str(offsets[i]))
    if i < len(names) - 1:
        f.write(',')
    f.write('\n')
f.write('};')

f.close()