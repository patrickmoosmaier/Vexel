# Builds the 32x32 16-colour DS menu icon from the final starter sprite.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
from mon_art import render
import gen_mon as G
L = G.lines[0]
img, pal = render(L, L['nst'] - 1, 5)
icon = Image.new("P", (32, 32), 0)
rgb = []
for c in pal:
    rgb += [int(c[0]), int(c[1]), int(c[2])]
rgb[0:3] = [8, 6, 24]
icon.putpalette(rgb + [0] * (768 - len(rgb)))
for y in range(32):
    for x in range(32):
        icon.putpixel((x, y), int(img[y * 2, x * 2]))
icon.save(sys.argv[1])
