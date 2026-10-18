# -*- coding: utf-8 -*-
# Generates font, tiles, character sprites and UI graphics for VEXEL.
import numpy as np, random, math, colorsys, sys, os

OUT = sys.argv[1] if len(sys.argv) > 1 else "source/gen_gfx.c"

def rgb15(r, g, b):
    r = max(0, min(255, int(r))); g = max(0, min(255, int(g))); b = max(0, min(255, int(b)))
    return 0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)

def hexc(s):
    s = s.lstrip('#'); return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))

# ---------------------------------------------------------------- FONT
G = {}
def g(ch, rows, top=1):
    G[ch] = (top, rows)

caps = {
'A': [".###.","#...#","#...#","#####","#...#","#...#","#...#"],
'B': ["####.","#...#","#...#","####.","#...#","#...#","####."],
'C': [".###.","#...#","#....","#....","#....","#...#",".###."],
'D': ["####.","#...#","#...#","#...#","#...#","#...#","####."],
'E': ["#####","#....","#....","####.","#....","#....","#####"],
'F': ["#####","#....","#....","####.","#....","#....","#...."],
'G': [".###.","#...#","#....","#.###","#...#","#...#",".###."],
'H': ["#...#","#...#","#...#","#####","#...#","#...#","#...#"],
'I': ["###",".#.",".#.",".#.",".#.",".#.","###"],
'J': ["..###","...#.","...#.","...#.","#..#.","#..#.",".##.."],
'K': ["#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"],
'L': ["#....","#....","#....","#....","#....","#....","#####"],
'M': ["#...#","##.##","#.#.#","#.#.#","#...#","#...#","#...#"],
'N': ["#...#","##..#","#.#.#","#..##","#...#","#...#","#...#"],
'O': [".###.","#...#","#...#","#...#","#...#","#...#",".###."],
'P': ["####.","#...#","#...#","####.","#....","#....","#...."],
'Q': [".###.","#...#","#...#","#...#","#.#.#","#..#.",".##.#"],
'R': ["####.","#...#","#...#","####.","#.#..","#..#.","#...#"],
'S': [".####","#....","#....",".###.","....#","....#","####."],
'T': ["#####","..#..","..#..","..#..","..#..","..#..","..#.."],
'U': ["#...#","#...#","#...#","#...#","#...#","#...#",".###."],
'V': ["#...#","#...#","#...#","#...#","#...#",".#.#.","..#.."],
'W': ["#...#","#...#","#...#","#.#.#","#.#.#","##.##","#...#"],
'X': ["#...#","#...#",".#.#.","..#..",".#.#.","#...#","#...#"],
'Y': ["#...#","#...#",".#.#.","..#..","..#..","..#..","..#.."],
'Z': ["#####","....#","...#.","..#..",".#...","#....","#####"],
}
for k, v in caps.items(): g(k, v)
low = {
'a': (3, [".##.","...#",".###","#..#",".###"]),
'b': (1, ["#...","#...","###.","#..#","#..#","#..#","###."]),
'c': (3, [".###","#...","#...","#...",".###"]),
'd': (1, ["...#","...#",".###","#..#","#..#","#..#",".###"]),
'e': (3, [".##.","#..#","####","#...",".###"]),
'f': (1, ["..##",".#..","###.",".#..",".#..",".#..",".#.."]),
'g': (3, [".###","#..#","#..#","#..#",".###","...#",".##."]),
'h': (1, ["#...","#...","###.","#..#","#..#","#..#","#..#"]),
'i': (1, [".#.","...","##.",".#.",".#.",".#.","###"]),
'j': (1, ["..#","...",".##","..#","..#","..#","..#","#.#",".#."]),
'k': (1, ["#...","#...","#..#","#.#.","##..","#.#.","#..#"]),
'l': (1, ["##.",".#.",".#.",".#.",".#.",".#.","###"]),
'm': (3, ["##.#.","#.#.#","#.#.#","#.#.#","#.#.#"]),
'n': (3, ["###.","#..#","#..#","#..#","#..#"]),
'o': (3, [".##.","#..#","#..#","#..#",".##."]),
'p': (3, ["###.","#..#","#..#","#..#","###.","#...","#..."]),
'q': (3, [".###","#..#","#..#","#..#",".###","...#","...#"]),
'r': (3, ["#.##","##..","#...","#...","#..."]),
's': (3, [".###","#...",".##.","...#","###."]),
't': (1, [".#..",".#..","###.",".#..",".#..",".#..","..##"]),
'u': (3, ["#..#","#..#","#..#","#..#",".###"]),
'v': (3, ["#...#","#...#",".#.#.",".#.#.","..#.."]),
'w': (3, ["#...#","#...#","#.#.#","#.#.#",".#.#."]),
'x': (3, ["#..#","#..#",".##.","#..#","#..#"]),
'y': (3, ["#..#","#..#","#..#","#..#",".###","...#",".##."]),
'z': (3, ["####","..#.",".#..","#...","####"]),
'ß': (1, [".##.","#..#","#..#","###.","#..#","#..#","###.","#..."]),
}
for k, (t, v) in low.items(): g(k, v, t)
digits = {
'0': [".##.","#..#","#.##","##.#","#..#","#..#",".##."],
'1': [".#.","##.",".#.",".#.",".#.",".#.","###"],
'2': [".##.","#..#","...#","..#.",".#..","#...","####"],
'3': ["###.","...#","...#",".##.","...#","...#","###."],
'4': ["#..#","#..#","#..#","####","...#","...#","...#"],
'5': ["####","#...","#...","###.","...#","...#","###."],
'6': [".##.","#...","#...","###.","#..#","#..#",".##."],
'7': ["####","...#","...#","..#.",".#..",".#..",".#.."],
'8': [".##.","#..#","#..#",".##.","#..#","#..#",".##."],
'9': [".##.","#..#","#..#",".###","...#","...#",".##."],
}
for k, v in digits.items(): g(k, v)
punct = {
'.': (7, ["#"]), ',': (7, [".#", "#."]), '!': (1, ["#","#","#","#","#",".","#"]),
'?': (1, [".##.","#..#","...#","..#.",".#..","....",".#.."]),
':': (3, ["#",".",".",".","#"]), ';': (3, [".#","..","..","..",".#","#."]),
'-': (4, ["###"]), '+': (2, [".#.",".#.","###",".#.",".#."]), "'": (1, ["#","#"]),
'"': (1, ["#.#","#.#"]), '(': (1, [".#","#.","#.","#.","#.","#.",".#"]),
')': (1, ["#.",".#",".#",".#",".#",".#","#."]), '/': (1, ["...#","...#","..#.",".#..","#...","#...","#..."]),
'%': (1, ["##..#","##.#.","..#..",".#...","#.##.","#.##.","....."]),
'&': (1, [".#..","#.#.","#.#.",".#..","#.##","#..#",".##."]),
'*': (2, ["#.#",".#.","#.#"]), '=': (3, ["###","...","###"]), '<': (2, ["..#",".#.","#..",".#.","..#"]),
'>': (2, ["#..",".#.","..#",".#.","#.."]), '_': (8, ["####"]), '[': (1, ["##","#.","#.","#.","#.","#.","##"]),
']': (1, ["##",".#",".#",".#",".#",".#","##"]), '#': (2, [".#.#.","#####",".#.#.","#####",".#.#."]),
'▶': (1, ["#...","##..","###.","####","###.","##..","#..."]),
'▼': (3, ["#####",".###.","..#.."]),
'▲': (3, ["..#..",".###.","#####"]),
'…': (7, ["#.#.#"]), '♥': (2, [".#.#.","#####","#####",".###.","..#.."]),
'→': (2, ["..#.","...#","####","...#","..#."]),
'·': (4, ["#"]), '~': (3, [".#.#", "#.#."]), '–': (4, ["####"]), '„': (7, ["#.#", "#.#"]), '“': (1, ["#.#", "#.#"]), '¢': (3, [".#.", "###", "#..", "###", ".#."]), '°': (1, [".#.","#.#",".#."]), '½': (1, ["#...#.", "#..#..", "#.#...", "..#.##", ".#...#", "#...#.", "...###"]), '¼': (1, ["#...#.", "#..#..", "#.#...", "..#.#.", ".#.##.", "#..###", "....#."]), '×': (3, ["#.#", ".#.", "#.#"]),
'Ω': (1, [".###.","#...#","#...#","#...#",".#.#.",".#.#.","##.##"]),
}
for k, (t, v) in punct.items(): g(k, v, t)
# umlauts
uml_l = {'ä': 'a', 'ö': 'o', 'ü': 'u'}
for ch, b in uml_l.items():
    t, rows = G[b]
    w = len(rows[0])
    d = "".join("#" if i in (0, w - 1) else "." for i in range(w))
    G[ch] = (1, [d, "." * w] + rows)
for ch, b in {'Ä': 'A', 'Ö': 'O', 'Ü': 'U'}.items():
    t, rows = G[b]
    w = len(rows[0])
    d = "".join("#" if i in (1, w - 2) else "." for i in range(w))
    G[ch] = (0, [d] + rows)

font_chars = [' '] + [chr(c) for c in range(33, 127) if chr(c) in G] + [c for c in G if ord(c) > 127]
font_chars = list(dict.fromkeys(font_chars))

def font_c():
    out = []
    cps, widths, bits = [], [], []
    for ch in font_chars:
        if ch == ' ':
            cps.append(32); widths.append(3); bits.append([0] * 10); continue
        top, rows = G[ch]
        w = max(len(r) for r in rows)
        b = [0] * 10
        for i, r in enumerate(rows):
            y = top + i
            if y >= 10: continue
            v = 0
            for x, c in enumerate(r):
                if c == '#': v |= (1 << x)
            b[y] = v
        cps.append(ord(ch)); widths.append(w); bits.append(b)
    out.append("const int font_count = %d;" % len(cps))
    out.append("const unsigned short font_cp[] = {%s};" % ",".join(str(c) for c in cps))
    out.append("const unsigned char font_w[] = {%s};" % ",".join(str(c) for c in widths))
    out.append("const unsigned char font_bits[][10] = {%s};" % ",".join("{" + ",".join(str(x) for x in b) + "}" for b in bits))
    return "\n".join(out)

# ---------------------------------------------------------------- TILES
rnd = random.Random(1337)
T = 16
tiles = []   # list of (name, [frames as HxWx3 arrays], solid, flags)
TILE_IDS = {}

def new(c=(0, 0, 0)):
    a = np.zeros((T, T, 3), dtype=np.float32); a[:, :] = c; return a

def noise(a, amt, seed):
    r = np.random.RandomState(seed)
    n = r.uniform(-amt, amt, (T, T, 1)); return a + n

def rect(a, x0, y0, x1, y1, c):
    a[max(0, y0):min(T, y1 + 1), max(0, x0):min(T, x1 + 1)] = c

def px(a, x, y, c, alpha=1.0):
    if 0 <= x < T and 0 <= y < T:
        a[y, x] = a[y, x] * (1 - alpha) + np.array(c, dtype=np.float32) * alpha

def add_tile(name, frames, solid, flags=0):
    TILE_IDS[name] = len(tiles)
    if not isinstance(frames, list): frames = [frames]
    tiles.append((name, frames, solid, flags))

F_ENC = 1; F_WATER = 2; F_WARP = 4; F_INTERACT = 8; F_ANIM = 16

def plate(base=(40, 46, 66), seed=1, light=False):
    a = new(base); a = noise(a, 4, seed)
    rect(a, 0, 0, 15, 0, (62, 70, 96)); rect(a, 0, 0, 0, 15, (58, 66, 90))
    rect(a, 0, 15, 15, 15, (24, 26, 40)); rect(a, 15, 0, 15, 15, (24, 26, 40))
    for (x, y) in ((2, 2), (13, 2), (2, 13), (13, 13)):
        px(a, x, y, (90, 98, 124)); px(a, x + 1, y + 1, (20, 22, 34))
    if light:
        rect(a, 6, 7, 9, 8, (40, 200, 230)); rect(a, 7, 7, 8, 8, (170, 250, 255))
    return a

add_tile('void', new((6, 6, 12)), True)
add_tile('plate', plate(), False)
add_tile('plate2', plate(seed=3, light=True), False)

def road(line=False, vline=False):
    a = new((28, 28, 38)); a = noise(a, 5, 7)
    for i in range(0, 16, 4):
        px(a, i, (i * 7) % 16, (40, 40, 54)); px(a, (i * 5 + 3) % 16, i + 1, (20, 20, 28))
    if line:
        rect(a, 1, 7, 6, 8, (0, 230, 255)); rect(a, 9, 7, 14, 8, (0, 230, 255))
        for x in list(range(1, 7)) + list(range(9, 15)):
            px(a, x, 6, (0, 110, 140), 0.6); px(a, x, 9, (0, 110, 140), 0.6)
    if vline:
        rect(a, 7, 1, 8, 6, (255, 40, 200)); rect(a, 7, 9, 8, 14, (255, 40, 200))
        for y in list(range(1, 7)) + list(range(9, 15)):
            px(a, 6, y, (120, 20, 100), 0.6); px(a, 9, y, (120, 20, 100), 0.6)
    return a
add_tile('road', road(), False)
add_tile('road_h', road(line=True), False)
add_tile('road_v', road(vline=True), False)

def grass(seed=5):
    a = new((26, 86, 72)); a = noise(a, 6, seed)
    r = random.Random(seed)
    for i in range(22):
        x, y = r.randrange(16), r.randrange(16)
        px(a, x, y, (40, 122, 98)); px(a, x, max(0, y - 1), (60, 150, 118), 0.7)
    for i in range(6):
        x, y = r.randrange(16), r.randrange(16)
        px(a, x, y, (10, 40, 40))
    return a
add_tile('grass', grass(), False)

def datagrass(phase):
    a = grass(9) * 0.8
    r = random.Random(42)
    blades = [(r.randrange(1, 15), r.randrange(6, 16), r.randrange(4, 8), r.random()) for _ in range(11)]
    for (bx, by, h, ph) in blades:
        for k in range(h):
            y = by - k
            lean = int(round(math.sin(phase * math.pi + ph * 6) * (k / h) * 1.5))
            x = bx + lean
            t = k / h
            glow = 0.5 + 0.5 * math.sin(phase * math.pi * 2 + ph * 6)
            c = (20 + 60 * t, 150 + 105 * t * (0.6 + 0.4 * glow), 140 + 90 * t)
            px(a, x, y, c)
            if k == h - 1: px(a, x, y, (150 + 100 * glow, 255, 240))
    return a
add_tile('datagrass', [datagrass(0), datagrass(0.5)], False, F_ENC | F_ANIM)

def tree(c1=(0, 210, 160), c2=(0, 120, 110), c3=(0, 50, 60), base=None):
    a = grass(11) if base is None else base.copy()
    # trunk
    rect(a, 7, 11, 8, 15, (60, 44, 60)); px(a, 7, 15, (90, 70, 90)); px(a, 8, 11, (40, 30, 44))
    rect(a, 6, 15, 9, 15, (8, 30, 30))
    cx, cy = 7.5, 6.5
    for y in range(T):
        for x in range(T):
            dx, dy = (x - cx) / 7.0, (y - cy) / 6.2
            d = dx * dx + dy * dy
            if d <= 1.0:
                l = (-dx * 0.6 - dy * 0.8) + 0.3 * math.sqrt(max(0, 1 - d))
                c = c1 if l > 0.45 else (c2 if l > -0.15 else c3)
                if d > 0.82: c = (c3[0] * 0.6, c3[1] * 0.6, c3[2] * 0.6)
                px(a, x, y, c)
    for (x, y) in ((5, 4), (9, 3), (10, 7), (4, 7), (7, 9)):
        px(a, x, y, (220, 255, 245))
    return a
add_tile('tree', tree(), True)
add_tile('tree2', tree((255, 90, 220), (170, 40, 160), (70, 14, 80)), True)

def water(phase):
    a = new((14, 40, 96))
    for y in range(T):
        for x in range(T):
            w = math.sin((x * 0.6 + y * 0.35) + phase * math.pi * 2) + math.sin(y * 0.9 - phase * math.pi * 2)
            s = 0.5 + 0.25 * w
            a[y, x] = (14 + 20 * s, 44 + 50 * s, 110 + 60 * s)
            if w > 1.55: a[y, x] = (120, 230, 255)
    return a
add_tile('water', [water(0), water(1 / 3), water(2 / 3)], True, F_WATER | F_ANIM)

def bridge():
    a = water(0) * 0.5
    rect(a, 0, 1, 15, 14, (70, 76, 92))
    for y in range(2, 14, 3):
        rect(a, 0, y, 15, y, (40, 44, 58))
    rect(a, 0, 0, 15, 0, (0, 200, 255)); rect(a, 0, 15, 15, 15, (0, 200, 255))
    rect(a, 0, 1, 15, 1, (100, 108, 130))
    return a
add_tile('bridge', bridge(), False)
def bridge_v():
    a = water(0.3) * 0.5
    rect(a, 1, 0, 14, 15, (70, 76, 92))
    for x in range(2, 14, 3):
        rect(a, x, 0, x, 15, (40, 44, 58))
    rect(a, 0, 0, 0, 15, (0, 200, 255)); rect(a, 15, 0, 15, 15, (0, 200, 255))
    return a
add_tile('bridge_v', bridge_v(), False)

def wall(neon=None, seed=0, lit=0.5):
    a = new((30, 30, 48)); a = noise(a, 3, seed + 50)
    r = random.Random(seed)
    rect(a, 0, 0, 0, 15, (44, 44, 66)); rect(a, 15, 0, 15, 15, (18, 18, 30))
    for wy in (2, 9):
        for wx in (2, 9):
            on = r.random() < lit
            c = (255, 220, 120) if on else (40, 60, 90)
            if on and r.random() < 0.4: c = (120, 230, 255)
            rect(a, wx, wy, wx + 4, wy + 4, (12, 12, 20))
            rect(a, wx + 1, wy + 1, wx + 3, wy + 3, c)
            if on: px(a, wx + 1, wy + 1, (255, 255, 230))
    if neon:
        rect(a, 0, 7, 15, 7, neon)
        for x in range(16): px(a, x, 6, neon, 0.35); px(a, x, 8, neon, 0.35)
    return a
add_tile('wall', wall(seed=1), True)
add_tile('wall2', wall(seed=2, lit=0.7), True)
add_tile('wall_neon', wall((255, 40, 190), seed=3), True)
add_tile('wall_cyan', wall((0, 230, 255), seed=4), True)

def roof(seed=0):
    a = new((48, 50, 74)); a = noise(a, 3, seed + 90)
    rect(a, 0, 15, 15, 15, (20, 20, 32))
    rect(a, 0, 14, 15, 14, (80, 84, 110))
    r = random.Random(seed)
    if r.random() < 0.6:
        rect(a, 3, 4, 7, 8, (70, 74, 100)); rect(a, 4, 5, 6, 7, (36, 38, 56))
    if r.random() < 0.5:
        rect(a, 11, 2, 11, 9, (100, 100, 120)); px(a, 11, 1, (255, 50, 80))
    return a
add_tile('roof', roof(1), True)
add_tile('roof2', roof(2), True)

def door(c=(0, 230, 255)):
    a = wall(seed=7, lit=0.0)
    rect(a, 3, 2, 12, 15, (12, 14, 24))
    rect(a, 4, 3, 7, 15, (40, 60, 90)); rect(a, 8, 3, 11, 15, (40, 60, 90))
    rect(a, 7, 3, 8, 15, c)
    rect(a, 3, 1, 12, 1, c)
    for x in range(3, 13): px(a, x, 0, c, 0.4)
    return a
add_tile('door', door(), False, F_WARP)
add_tile('door_red', door((255, 60, 80)), False, F_WARP)

def fence():
    a = plate(seed=13)
    for x in range(T):
        px(a, x, 4, (0, 255, 230), 0.9); px(a, x, 9, (0, 255, 230), 0.7)
        px(a, x, 3, (0, 255, 230), 0.25); px(a, x, 10, (0, 255, 230), 0.25)
    rect(a, 0, 2, 1, 13, (110, 110, 140)); rect(a, 14, 2, 15, 13, (110, 110, 140))
    return a
add_tile('fence', fence(), True)

def rockfloor(seed=17, base=(46, 40, 60)):
    a = new(base); a = noise(a, 6, seed)
    r = random.Random(seed)
    for i in range(8):
        x, y = r.randrange(15), r.randrange(15)
        px(a, x, y, (base[0] + 24, base[1] + 22, base[2] + 28)); px(a, x + 1, y + 1, (base[0] - 18, base[1] - 16, base[2] - 18))
    return a
add_tile('rockfloor', rockfloor(), False)

def crystal(c=(120, 220, 255)):
    a = rockfloor(19)
    shards = [(4, 15, 3, 11), (8, 15, 4, 14), (12, 15, 3, 9)]
    for (cx, by, w, h) in shards:
        for k in range(h):
            y = by - k
            hw = max(0, int(w * (1 - k / h) + 0.5))
            for x in range(cx - hw, cx + hw + 1):
                light = (x < cx)
                cc = tuple(v * (1.0 if light else 0.6) for v in c)
                if k == h - 1 or abs(x - cx) == hw: cc = tuple(v * 0.35 for v in c)
                px(a, x, y, cc)
        px(a, cx - 1, by - h + 3, (255, 255, 255))
    return a
add_tile('crystal', crystal(), True)
add_tile('crystal2', crystal((240, 120, 255)), True)

def sand(seed):
    a = new((92, 60, 110)); a = noise(a, 6, seed)
    r = random.Random(seed)
    for i in range(10):
        x, y = r.randrange(16), r.randrange(16)
        px(a, x, y, (130, 90, 150))
    for i in range(2):
        x, y = r.randrange(12), r.randrange(16)
        c = r.choice([(0, 255, 200), (255, 60, 200), (255, 240, 80)])
        for k in range(r.randrange(2, 5)): px(a, x + k, y, c, 0.7)
    return a
add_tile('sand', sand(23), False)
def sand_enc(ph):
    a = sand(29) * 0.85
    r = random.Random(77)
    for i in range(6):
        x, y = r.randrange(2, 14), r.randrange(6, 15)
        h = r.randrange(3, 7)
        g = 0.5 + 0.5 * math.sin(ph * 6.28 + i * 1.7)
        for k in range(h):
            t = k / h
            px(a, x, y - k, (160 + 90 * t * g, 40 + 60 * t, 200 + 55 * t))
            if k < h - 2: px(a, x + 1, y - k, (80, 20, 110))
        px(a, x, y - h, (255, 200 + 55 * g, 255))
    return a
add_tile('sand_enc', [sand_enc(0), sand_enc(0.5)], False, F_ENC | F_ANIM)

def rock():
    a = sand(37)
    cx, cy = 7.5, 8.5
    for y in range(T):
        for x in range(T):
            dx, dy = (x - cx) / 7.2, (y - cy) / 6.5
            d = dx * dx + dy * dy
            if d < 1:
                l = -dx * 0.6 - dy * 0.7
                c = (130, 118, 140) if l > 0.3 else ((90, 80, 104) if l > -0.3 else (56, 48, 70))
                if d > 0.8: c = (30, 24, 40)
                px(a, x, y, c)
    return a
add_tile('rock', rock(), True)

def sign():
    a = plate(seed=41)
    rect(a, 7, 9, 8, 15, (90, 90, 110))
    rect(a, 2, 2, 13, 9, (10, 30, 50))
    rect(a, 2, 2, 13, 2, (0, 230, 255)); rect(a, 2, 9, 13, 9, (0, 230, 255))
    rect(a, 2, 2, 2, 9, (0, 230, 255)); rect(a, 13, 2, 13, 9, (0, 230, 255))
    for y in (4, 6):
        rect(a, 4, y, 11, y, (120, 220, 255))
    return a
add_tile('sign', sign(), True, F_INTERACT)

def lamp(base):
    a = base.copy()
    rect(a, 7, 4, 8, 15, (80, 84, 104)); rect(a, 8, 4, 8, 15, (50, 52, 70))
    rect(a, 4, 2, 11, 4, (60, 60, 80)); rect(a, 5, 4, 10, 4, (255, 240, 180))
    for y in range(5, 9):
        for x in range(3 + (y - 5), 13 - (y - 5)):
            px(a, x, y, (255, 240, 180), 0.12)
    rect(a, 6, 15, 9, 15, (30, 30, 40))
    return a
add_tile('lamp', lamp(plate(seed=43)), True)

def pipe():
    a = plate(seed=47)
    for y in range(4, 12):
        t = (y - 4) / 7.0
        s = 1 - abs(t - 0.35) * 1.4
        rect(a, 0, y, 15, y, (60 + 90 * s, 70 + 80 * s, 80 + 70 * s))
    rect(a, 6, 3, 9, 12, (120, 70, 50)); rect(a, 6, 3, 9, 3, (180, 110, 80))
    return a
add_tile('pipe', pipe(), True)

def flower():
    a = grass(53)
    r = random.Random(53)
    for i in range(4):
        x, y = r.randrange(2, 14), r.randrange(3, 14)
        c = r.choice([(255, 80, 210), (0, 240, 255), (255, 230, 60), (160, 100, 255)])
        px(a, x, y + 1, (20, 90, 60))
        for (dx, dy) in ((0, -1), (-1, 0), (1, 0), (0, 1)):
            px(a, x + dx, y + dy - 1, c)
        px(a, x, y - 1, (255, 255, 255))
    return a
add_tile('flower', flower(), False)

def infloor(c1=(150, 160, 190), c2=(130, 140, 172)):
    a = new(c1)
    for y in range(T):
        for x in range(T):
            if ((x // 8) + (y // 8)) % 2: a[y, x] = c2
    rect(a, 0, 0, 15, 0, (170, 180, 210)); rect(a, 0, 0, 0, 15, (170, 180, 210))
    return a
add_tile('in_floor', infloor(), False)

def inwall(window=False):
    a = new((70, 74, 110))
    rect(a, 0, 12, 15, 15, (40, 42, 66)); rect(a, 0, 12, 15, 12, (0, 210, 255))
    for x in (0, 8): rect(a, x, 0, x, 11, (50, 54, 84))
    if window:
        rect(a, 2, 2, 13, 9, (14, 16, 40))
        r = random.Random(3)
        for i in range(8): px(a, r.randrange(3, 13), r.randrange(3, 9), (255, 230, 150))
        rect(a, 3, 7, 12, 9, (40, 30, 80))
        rect(a, 2, 2, 13, 2, (120, 124, 160))
    return a
add_tile('in_wall', inwall(), True)
add_tile('in_window', inwall(True), True)

def counter(top=(200, 210, 230)):
    a = infloor()
    rect(a, 0, 3, 15, 15, (60, 64, 100)); rect(a, 0, 3, 15, 6, top)
    rect(a, 0, 7, 15, 7, (0, 210, 255))
    return a
add_tile('counter', counter(), True)

def pc():
    a = counter()
    rect(a, 3, 0, 12, 6, (20, 22, 30)); rect(a, 4, 1, 11, 5, (0, 160, 200))
    rect(a, 5, 2, 9, 2, (200, 255, 255)); rect(a, 5, 4, 8, 4, (200, 255, 255))
    return a
add_tile('pc', pc(), True, F_INTERACT)

def bed():
    a = infloor()
    rect(a, 1, 1, 14, 15, (40, 40, 70)); rect(a, 2, 2, 13, 5, (230, 230, 240))
    rect(a, 2, 6, 13, 14, (60, 120, 200)); rect(a, 2, 6, 13, 6, (100, 170, 240))
    return a
add_tile('bed', bed(), True)

def table():
    a = infloor()
    rect(a, 1, 3, 14, 11, (120, 90, 140)); rect(a, 1, 3, 14, 4, (170, 140, 190))
    rect(a, 2, 12, 3, 15, (60, 50, 70)); rect(a, 12, 12, 13, 15, (60, 50, 70))
    px(a, 6, 6, (0, 255, 200)); px(a, 7, 6, (0, 255, 200))
    return a
add_tile('table', table(), True)

def plant():
    a = infloor()
    rect(a, 5, 10, 10, 15, (200, 200, 220)); rect(a, 5, 10, 10, 10, (240, 240, 250))
    for (x, y, rr) in ((8, 6, 4), (5, 5, 2), (11, 4, 2)):
        for yy in range(y - rr, y + rr + 1):
            for xx in range(x - rr, x + rr + 1):
                if (xx - x) ** 2 + (yy - y) ** 2 <= rr * rr: px(a, xx, yy, (20, 200, 140) if xx < x else (10, 130, 100))
    return a
add_tile('plant', plant(), True)

def machine(ph):
    a = new((50, 54, 80))
    rect(a, 1, 1, 14, 15, (80, 86, 120)); rect(a, 2, 2, 13, 8, (16, 20, 34))
    for i in range(5):
        h = int(2 + 4 * (0.5 + 0.5 * math.sin(ph * 6.28 + i * 1.3)))
        rect(a, 3 + i * 2, 8 - h, 3 + i * 2, 7, (0, 255, 160))
    cols = [(255, 60, 80), (255, 220, 40), (0, 230, 255)]
    for i in range(3):
        on = (int(ph * 3) + i) % 3 == 0
        px(a, 4 + i * 3, 11, cols[i] if on else (40, 40, 50))
    rect(a, 1, 15, 14, 15, (30, 30, 46))
    return a
add_tile('machine', [machine(0), machine(0.33), machine(0.66)], True, F_ANIM)

def warp(ph):
    a = plate(seed=61)
    for y in range(T):
        for x in range(T):
            d = math.hypot(x - 7.5, y - 7.5)
            if d < 7:
                v = 0.5 + 0.5 * math.sin(d * 1.4 - ph * 6.28)
                a[y, x] = (30 + 100 * v, 60 + 190 * v, 140 + 115 * v)
            if 6.3 < d < 7.3: a[y, x] = (200, 255, 255)
    return a
add_tile('warp', [warp(0), warp(0.33), warp(0.66)], False, F_WARP | F_ANIM)

def carpet():
    a = new((120, 30, 70))
    rect(a, 0, 0, 15, 1, (220, 180, 60)); rect(a, 0, 14, 15, 15, (220, 180, 60))
    for x in range(0, 16, 4): px(a, x + 1, 7, (255, 100, 160)); px(a, x + 2, 8, (255, 100, 160))
    return a
add_tile('carpet', carpet(), False)

def arena(ph=0, c=(0, 230, 255)):
    a = new((24, 20, 40))
    for y in range(T):
        for x in range(T):
            if x == 0 or y == 0: a[y, x] = (50, 44, 80)
    rect(a, 4, 4, 11, 11, (34, 30, 56))
    for i in range(4, 12):
        px(a, i, 4, c, 0.5); px(a, i, 11, c, 0.5); px(a, 4, i, c, 0.5); px(a, 11, i, c, 0.5)
    return a
add_tile('arena', arena(), False)
def arena_line():
    a = arena()
    rect(a, 0, 7, 15, 8, (255, 40, 200))
    return a
add_tile('arena_line', arena_line(), False)

def lava(ph):
    a = new((120, 20, 10))
    for y in range(T):
        for x in range(T):
            v = 0.5 + 0.25 * (math.sin(x * 0.7 + ph * 6.28) + math.sin(y * 0.8 + x * 0.3 - ph * 6.28))
            a[y, x] = (180 + 75 * v, 40 + 160 * v * v, 20 + 40 * v * v * v)
    return a
add_tile('plasma', [lava(0), lava(0.33), lava(0.66)], True, F_ANIM)

def voidfloor(seed):
    a = new((8, 6, 20)); a = noise(a, 2, seed)
    r = random.Random(seed)
    for i in range(5):
        px(a, r.randrange(16), r.randrange(16), r.choice([(255, 255, 255), (160, 120, 255), (100, 200, 255)]))
    for x in range(T): px(a, x, 0, (60, 20, 120), 0.6)
    for y in range(T): px(a, 0, y, (60, 20, 120), 0.6)
    return a
add_tile('voidfloor', voidfloor(71), False)

def towerwall():
    a = new((220, 224, 236))
    rect(a, 0, 0, 0, 15, (250, 250, 255)); rect(a, 15, 0, 15, 15, (170, 174, 190))
    rect(a, 0, 6, 15, 8, (200, 20, 50)); rect(a, 0, 7, 15, 7, (255, 80, 100))
    rect(a, 0, 15, 15, 15, (140, 144, 160))
    return a
add_tile('towerwall', towerwall(), True)
def towerfloor():
    a = new((200, 204, 218))
    rect(a, 0, 0, 15, 0, (235, 238, 248)); rect(a, 0, 0, 0, 15, (235, 238, 248))
    rect(a, 15, 0, 15, 15, (160, 164, 180)); rect(a, 0, 15, 15, 15, (160, 164, 180))
    px(a, 7, 7, (255, 60, 80)); px(a, 8, 8, (255, 60, 80))
    return a
add_tile('towerfloor', towerfloor(), False)

def rack(ph):
    a = new((30, 32, 48))
    rect(a, 1, 0, 14, 15, (20, 22, 34))
    r = random.Random(int(ph * 10) + 3)
    for y in range(1, 15, 2):
        rect(a, 2, y, 13, y, (44, 48, 70))
        for k in range(3):
            if r.random() < 0.55: px(a, 3 + k * 2, y, r.choice([(0, 255, 120), (0, 200, 255), (255, 200, 0)]))
    return a
add_tile('rack', [rack(0), rack(0.5)], True, F_ANIM)

def cliff():
    a = sand(79)
    for y in range(T):
        for x in range(T):
            s = 0.6 + 0.4 * math.sin(x * 1.3 + y * 0.2)
            a[y, x] = (70 * s, 50 * s, 80 * s)
    rect(a, 0, 0, 15, 1, (140, 110, 160))
    for x in range(0, 16, 5): rect(a, x, 3, x, 15, (34, 24, 44))
    return a
add_tile('cliff', cliff(), True)

def ice():
    a = new((170, 220, 245)); a = noise(a, 5, 83)
    for i in range(16):
        px(a, i, (i * 3) % 16, (240, 255, 255)); px(a, (i * 7) % 16, i, (140, 190, 230))
    return a
add_tile('ice', ice(), False)
def ice_enc(ph):
    a = ice() * 0.95
    r = random.Random(89)
    for i in range(7):
        x, y = r.randrange(2, 14), r.randrange(5, 15)
        h = r.randrange(3, 6)
        for k in range(h):
            px(a, x, y - k, (90 + 40 * (k / h), 160 + 90 * (k / h), 255))
        g = 0.5 + 0.5 * math.sin(ph * 6.28 + i)
        px(a, x, y - h, (200 + 55 * g, 255, 255))
    return a
add_tile('ice_enc', [ice_enc(0), ice_enc(0.5)], False, F_ENC | F_ANIM)

def billboard(ph):
    a = wall(seed=91, lit=0.2)
    rect(a, 1, 1, 14, 12, (10, 10, 20))
    for y in range(2, 12):
        for x in range(2, 14):
            v = 0.5 + 0.5 * math.sin(x * 0.5 + y * 0.3 + ph * 6.28)
            a[y, x] = (255 * v, 40 + 60 * (1 - v), 200 * (1 - v) + 55)
    rect(a, 1, 13, 14, 13, (0, 230, 255))
    return a
add_tile('billboard', [billboard(0), billboard(0.33), billboard(0.66)], True, F_ANIM)

def heal_pad(ph):
    a = infloor()
    for y in range(T):
        for x in range(T):
            d = math.hypot(x - 7.5, y - 7.5)
            if d < 6.5:
                v = 0.5 + 0.5 * math.sin(ph * 6.28 - d)
                a[y, x] = (60 + 100 * v, 200 + 55 * v, 160 + 90 * v)
    rect(a, 6, 4, 9, 11, (255, 255, 255)); rect(a, 4, 6, 11, 9, (255, 255, 255))
    return a
add_tile('heal', [heal_pad(0), heal_pad(0.5)], True, F_INTERACT | F_ANIM)

def wallpad(base):
    a = base.copy(); return a
add_tile('mat', infloor((40, 160, 170), (30, 140, 150)), False)
add_tile('glitch', [sand(97) * 0.5 + np.array([0, 0, 40]), sand(98) * 0.5 + np.array([40, 0, 0])], True, F_ANIM)

def tiles_c():
    out = []
    frames = []
    info = []
    for (name, fr, solid, flags) in tiles:
        first = len(frames)
        for f in fr: frames.append(f)
        info.append((first, len(fr), int(solid), flags))
    out.append("const int tile_count = %d;" % len(tiles))
    out.append("const unsigned short tile_gfx[%d][256] __attribute__((aligned(4))) = {" % len(frames))
    for f in frames:
        f = np.clip(f, 0, 255)
        vals = [rgb15(*f[y, x]) for y in range(T) for x in range(T)]
        out.append("{" + ",".join(str(v) for v in vals) + "},")
    out.append("};")
    out.append("const unsigned char tile_info[][4] = {%s};" % ",".join("{%d,%d,%d,%d}" % i for i in info))
    return "\n".join(out)

def tiles_h():
    out = []
    for i, (name, fr, solid, flags) in enumerate(tiles):
        out.append("#define TL_%s %d" % (name.upper(), i))
    return "\n".join(out)

# ---------------------------------------------------------------- CHARACTERS (16x24)
CH_W, CH_H = 16, 24
DOWN = [
"................",
".....kkkkkk.....",
"....khhhhhhk....",
"...khhhhhhhhk...",
"...khhhhhhhhk...",
"..khvvvvvvvvhk..",
"..khhssssssHhk..",
"..khsessssesHk..",
"..khsessssesHk..",
"...kssssssssk...",
"....ksssSSsk....",
".....kkkkkk.....",
"....kjjvvjjk....",
"...kjjjvvjjjk...",
"..kjjjjvvjjjJk..",
"..kjkjjvvjjkJk..",
"..kskjjjjjjksk..",
"...kkjjjjjjkk...",
"...kppppppppk...",
"...kppPkkpPPk...",
"...kppk..kpPk...",
"...kbbk..kbbk...",
"...kbbk..kbbk...",
"....kk....kk....",
]
UP = [
"................",
".....kkkkkk.....",
"....khhhhhhk....",
"...khhhhhhhhk...",
"...khhhhhhhhk...",
"..khvvvvvvvvhk..",
"..khhhhhhhhHhk..",
"..khhhhhhhhHhk..",
"..khhhhhhhhHhk..",
"...khhhhhhHHk...",
"....kHHHHHHk....",
".....kkkkkk.....",
"....kjjjjjjk....",
"...kjjjjjjjjk...",
"..kjjjjjjjjjJk..",
"..kjkjjvvjjkJk..",
"..kskjjvvjjksk..",
"...kkjjjjjjkk...",
"...kppppppppk...",
"...kppPkkpPPk...",
"...kppk..kpPk...",
"...kbbk..kbbk...",
"...kbbk..kbbk...",
"....kk....kk....",
]
LEFT = [
"................",
".....kkkkkk.....",
"....khhhhhhk....",
"...khhhhhhhhk...",
"...khhhhhhhhk...",
"..kvvvvvvvhhhk..",
"..kssssshhhHhk..",
"..ksesssshhHhk..",
"..ksesssshhHk...",
"..ksssssshhk....",
"...kSssssHk.....",
"....kkkkkk......",
".....kjjjjk.....",
"....kjjjjjJk....",
"....kvjjjjJk....",
"....kvjjjjJk....",
"....ksjjjjJk....",
".....kjjjjk.....",
".....kppppk.....",
".....kpppPk.....",
".....kppPPk.....",
".....kbbbbk.....",
"....kbbbbbk.....",
".....kkkkk......",
]
# walking leg variants (rows 18-23)
DOWN_W1 = [
"...kppppppppk...",
"...kppPkkpPPk...",
"...kppk..kpPk...",
"...kbbk..kbbk...",
"...kbbk...kk....",
"....kk..........",
]
DOWN_W2 = [
"...kppppppppk...",
"...kppPkkpPPk...",
"...kppk..kpPk...",
"...kbbk..kbbk...",
"....kk...kbbk...",
"..........kk....",
]
LEFT_W1 = [
".....kppppk.....",
"....kppkpPPk....",
"...kppk.kpPk....",
"...kbbk..kbbk...",
"..kbbbk..kbbk...",
"...kkk....kk....",
]
LEFT_W2 = [
".....kppppk.....",
".....kpppPk.....",
".....kppPPk.....",
".....kpbbbk.....",
".....kbbbbk.....",
"......kkkk......",
]
BOT = [
"................",
"................",
".......kk.......",
".......kvk......",
"......kkkk......",
"....kkwwwwkk....",
"...kwwwwwwwwk...",
"..kwwkkkkkkwWk..",
"..kwkvvvvvvkWk..",
"..kwkvevvevkWk..",
"..kwkvvvvvvkWk..",
"..kwwkkkkkkwWk..",
"...kwwwwwwwWk...",
"..kkkjjjjjjkkk..",
".kwkjjjvvjjjkWk.",
".kwkjjjvvjjjkWk.",
".kkkjjjjjjjJkkk.",
"...kjjjjjjjJk...",
"....kjjjjjJk....",
".....kkkkkk.....",
"......vvvv......",
".....v....v.....",
"................",
"................",
]

def build_char_frames(design):
    """returns frames list: 0 down,1 down w1,2 down w2,3 up,4 up w1,5 up w2,6 left,7 left w1,8 left w2"""
    if design == 'bot':
        fr = []
        for d in range(3):
            base = [r for r in BOT]
            for w in range(3):
                rows = list(base)
                if w == 1: rows = rows[1:] + [rows[0]]
                if w == 2: rows = rows[1:] + [rows[0]]
                fr.append(rows)
        return fr
    fr = []
    def with_legs(base, legs):
        rows = list(base[:18]) + list(legs)
        return rows
    def bob(rows):
        return rows[1:] + ["................"]
    fr.append(list(DOWN)); fr.append(bob(with_legs(DOWN, DOWN_W1))); fr.append(bob(with_legs(DOWN, DOWN_W2)))
    fr.append(list(UP)); fr.append(bob(with_legs(UP, DOWN_W2))); fr.append(bob(with_legs(UP, DOWN_W1)))
    fr.append(list(LEFT)); fr.append(bob(with_legs(LEFT, LEFT_W1))); fr.append(bob(with_legs(LEFT, LEFT_W2)))
    return fr

PAL_KEYS = ".kshHvjJpPbewSW"
def make_pal(skin, hair, visor, jacket, pants, boots, eye=(20, 20, 30)):
    def sh(c, f): return tuple(v * f for v in c)
    return {
        'k': (14, 12, 22), 's': skin, 'S': sh(skin, 0.78), 'h': hair, 'H': sh(hair, 0.65), 'v': visor,
        'j': jacket, 'J': sh(jacket, 0.62), 'p': pants, 'P': sh(pants, 0.65), 'b': boots, 'e': eye,
        'w': (235, 240, 250), 'W': (150, 156, 180),
    }

CHARS = [
    # name, design, palette
    ('player', 'human', make_pal((248, 206, 170), (40, 50, 90), (0, 240, 255), (40, 40, 60), (30, 34, 70), (230, 240, 255))),
    ('mom', 'human', make_pal((240, 200, 170), (210, 110, 60), (255, 220, 120), (230, 232, 245), (60, 70, 110), (50, 50, 70))),
    ('kai', 'human', make_pal((246, 210, 180), (230, 40, 60), (255, 200, 40), (240, 240, 250), (40, 40, 50), (200, 40, 60))),
    ('grunt', 'human', make_pal((230, 190, 160), (240, 240, 250), (255, 30, 60), (225, 228, 240), (190, 20, 50), (40, 40, 50))),
    ('scientist', 'human', make_pal((240, 205, 175), (110, 110, 120), (120, 255, 160), (235, 238, 248), (80, 80, 110), (40, 40, 60))),
    ('kid', 'human', make_pal((252, 214, 180), (250, 200, 60), (255, 80, 200), (60, 200, 140), (60, 80, 160), (240, 240, 240))),
    ('punk', 'human', make_pal((220, 170, 140), (255, 40, 200), (0, 255, 160), (30, 30, 30), (80, 30, 120), (20, 20, 20))),
    ('elder', 'human', make_pal((230, 190, 160), (220, 220, 230), (160, 120, 255), (90, 60, 130), (60, 40, 90), (40, 30, 50))),
    ('warden_volt', 'human', make_pal((240, 200, 160), (255, 230, 40), (255, 255, 120), (40, 40, 60), (250, 210, 0), (30, 30, 30))),
    ('warden_hydro', 'human', make_pal((200, 150, 120), (20, 120, 220), (0, 255, 255), (20, 80, 140), (240, 240, 255), (20, 60, 120))),
    ('warden_kris', 'human', make_pal((250, 220, 200), (200, 230, 255), (150, 220, 255), (120, 160, 230), (230, 240, 255), (80, 120, 200))),
    ('warden_void', 'human', make_pal((200, 180, 200), (40, 20, 60), (180, 60, 255), (20, 14, 30), (50, 20, 80), (10, 10, 20))),
    ('ceo', 'human', make_pal((240, 210, 190), (30, 30, 30), (255, 20, 40), (20, 20, 24), (20, 20, 24), (200, 20, 40))),
    ('nursebot', 'bot', make_pal((0, 0, 0), (0, 0, 0), (60, 255, 180), (230, 80, 140), (0, 0, 0), (0, 0, 0), (255, 255, 255))),
    ('shopbot', 'bot', make_pal((0, 0, 0), (0, 0, 0), (0, 200, 255), (250, 190, 40), (0, 0, 0), (0, 0, 0), (255, 255, 255))),
    ('omega', 'bot', make_pal((0, 0, 0), (0, 0, 0), (255, 20, 60), (40, 40, 50), (0, 0, 0), (0, 0, 0), (255, 255, 255))),
    ('woman', 'human', make_pal((200, 150, 120), (20, 20, 30), (255, 120, 40), (200, 60, 120), (40, 40, 70), (230, 230, 240))),
    ('runner', 'human', make_pal((246, 210, 180), (90, 255, 140), (255, 255, 255), (20, 140, 90), (30, 30, 40), (255, 255, 255))),
]

LEADER_COLS = [  # hair, visor, jacket, pants for generated gym leaders and elites
    ('gym1', (0, 230, 230), (255, 0, 200), (20, 30, 40), (0, 160, 160)), ('gym3', (120, 255, 60), (200, 0, 255), (70, 30, 100), (40, 120, 40)),
    ('gym5', (255, 170, 230), (255, 255, 255), (180, 90, 200), (90, 50, 120)), ('gym7', (255, 40, 140), (255, 0, 80), (30, 10, 20), (90, 0, 40)),
    ('gym8', (180, 190, 210), (0, 200, 255), (110, 120, 140), (60, 66, 80)), ('gym9', (90, 100, 60), (255, 120, 0), (80, 90, 50), (60, 64, 40)),
    ('gym10', (200, 30, 40), (255, 200, 0), (120, 20, 30), (60, 20, 20)), ('gym11', (140, 100, 60), (255, 200, 120), (110, 80, 50), (70, 50, 30)),
    ('gym12', (255, 120, 0), (255, 240, 0), (200, 50, 20), (80, 30, 20)), ('gym13', (255, 60, 30), (255, 200, 0), (60, 20, 20), (150, 30, 30)),
    ('gym14', (255, 240, 180), (255, 220, 80), (250, 250, 255), (220, 200, 140)), ('gym15', (140, 230, 255), (255, 255, 255), (100, 200, 220), (220, 240, 250)),
    ('gym16', (120, 0, 80), (255, 0, 60), (40, 0, 40), (90, 0, 60)), ('gym17', (60, 60, 60), (0, 255, 120), (30, 30, 40), (120, 0, 60)),
    ('gym18', (230, 230, 255), (160, 120, 255), (40, 30, 110), (20, 20, 60)), ('gym19', (40, 40, 120), (0, 255, 255), (60, 40, 140), (30, 30, 70)),
    ('gym20', (60, 200, 60), (255, 120, 200), (40, 140, 60), (90, 70, 40)), ('gym21', (200, 120, 40), (255, 60, 0), (140, 90, 40), (70, 50, 30)),
    ('gym22', (180, 255, 120), (0, 255, 120), (40, 90, 60), (60, 60, 70)), ('gym23', (90, 110, 200), (120, 220, 255), (70, 80, 120), (50, 55, 80)),
    ('elite0', (60, 60, 70), (255, 80, 0), (90, 100, 60), (40, 40, 40)), ('elite1', (20, 20, 20), (255, 0, 140), (40, 10, 50), (20, 10, 30)),
    ('elite2', (250, 250, 255), (180, 120, 255), (230, 60, 150), (60, 30, 80)), ('elite3', (90, 20, 20), (255, 40, 0), (60, 20, 20), (30, 30, 30)),
    ('cult', (40, 10, 60), (0, 255, 200), (60, 20, 90), (30, 10, 45)), ('xeno', (240, 240, 255), (255, 0, 255), (20, 10, 30), (80, 0, 120)),
]
for (nm, hair, visor, jacket, pants) in LEADER_COLS:
    CHARS.append((nm, 'human', make_pal((236, 200, 170), hair, visor, jacket, pants, (30, 30, 40))))

def chars_c():
    out = []
    out.append("const int chr_count = %d;" % len(CHARS))
    out.append("const unsigned short chr_pal[%d][16] = {" % len(CHARS))
    for (n, d, pal) in CHARS:
        vals = [0] + [rgb15(*pal[k]) for k in PAL_KEYS[1:]]
        vals += [0] * (16 - len(vals))
        out.append("{" + ",".join(str(v) for v in vals) + "},")
    out.append("};")
    out.append("const unsigned char chr_gfx[%d][9][%d] = {" % (len(CHARS), CH_W * CH_H))
    for (n, d, pal) in CHARS:
        frs = build_char_frames(d)
        out.append("{")
        for rows in frs:
            assert len(rows) == CH_H, (n, len(rows))
            vals = []
            for r in rows:
                assert len(r) == CH_W, (n, r)
                for c in r: vals.append(PAL_KEYS.index(c) if c in PAL_KEYS else 0)
            out.append("{" + ",".join(str(v) for v in vals) + "},")
        out.append("},")
    out.append("};")
    return "\n".join(out)

def chars_h():
    return "\n".join("#define CH_%s %d" % (n.upper(), i) for i, (n, d, p) in enumerate(CHARS))

# ---------------------------------------------------------------- write
with open(OUT, "w", encoding="utf-8") as f:
    f.write("// generated by tools/gen_gfx.py\n")
    f.write(font_c() + "\n")
    f.write(tiles_c() + "\n")
    f.write(chars_c() + "\n")
with open(OUT.replace(".c", ".h"), "w", encoding="utf-8") as f:
    f.write("#pragma once\n// generated\n")
    f.write("extern const int font_count;\nextern const unsigned short font_cp[];\nextern const unsigned char font_w[];\nextern const unsigned char font_bits[][10];\n")
    f.write("extern const int tile_count;\nextern const unsigned short tile_gfx[][256];\nextern const unsigned char tile_info[][4];\n")
    f.write("extern const int chr_count;\nextern const unsigned short chr_pal[][16];\nextern const unsigned char chr_gfx[][9][384];\n")
    f.write("#define TF_ENC 1\n#define TF_WATER 2\n#define TF_WARP 4\n#define TF_INTERACT 8\n#define TF_ANIM 16\n")
    f.write(tiles_h() + "\n" + chars_h() + "\n")

# preview sheet
if os.environ.get("PREVIEW"):
    from PIL import Image
    n = len(tiles)
    img = Image.new("RGB", (16 * 12 * 2, ((n + 11) // 12) * 16 * 2 + 24 * 2 * ((len(CHARS) * 9 + 15) // 16) + 8))
    for i, (name, fr, s, fl) in enumerate(tiles):
        a = np.clip(fr[0], 0, 255).astype(np.uint8)
        im = Image.fromarray(a).resize((32, 32), Image.NEAREST)
        img.paste(im, ((i % 12) * 32, (i // 12) * 32))
    y0 = ((n + 11) // 12) * 32 + 8
    k = 0
    for (nm, d, pal) in CHARS:
        for rows in build_char_frames(d):
            a = np.zeros((24, 16, 3), np.uint8)
            for y, r in enumerate(rows):
                for x, c in enumerate(r):
                    if c != '.': a[y, x] = [int(v) for v in pal[c]]
            im = Image.fromarray(a).resize((32, 48), Image.NEAREST)
            img.paste(im, ((k % 12) * 32, y0 + (k // 12) * 48)); k += 1
    img.save(os.environ["PREVIEW"])
print("tiles", len(tiles), "font", len(font_chars), "chars", len(CHARS))
