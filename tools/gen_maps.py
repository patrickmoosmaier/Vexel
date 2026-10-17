# -*- coding: utf-8 -*-
# Builds the world maps (ASCII) and exports them as C arrays plus named points.
import sys, random, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
OUT = sys.argv[1] if len(sys.argv) > 1 else "source/gen_maps.c"

MAPS = []      # (const_name, rows)
POINTS = {}    # name -> (x, y)

class Map:
    def __init__(s, name, w, h, fill='.'):
        s.name, s.w, s.h = name, w, h
        s.g = [[fill] * w for _ in range(h)]
    def put(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h: s.g[y][x] = c
    def get(s, x, y): return s.g[y][x]
    def rect(s, x, y, w, h, c):
        for j in range(h):
            for i in range(w): s.put(x + i, y + j, c)
    def border(s, c, t=1):
        for y in range(s.h):
            for x in range(s.w):
                if x < t or y < t or x >= s.w - t or y >= s.h - t: s.g[y][x] = c
    def building(s, x, y, w, h, door_dx, name=None, roof='R', wall='W', neon=None, door='D', windows=None):
        s.rect(x, y, w, 1, roof)
        s.rect(x, y + 1, w, h - 1, wall)
        if neon:
            for i in range(w):
                if i % 2 == 0: s.put(x + i, y + 1, neon)
        if windows:
            for (i, c) in windows: s.put(x + i, y + 1, c)
        s.put(x + door_dx, y + h - 1, door)
        if name: POINTS[name] = (x + door_dx, y + h - 1)
    def text(s, x, y, rows):
        for j, r in enumerate(rows):
            for i, c in enumerate(r):
                if c != '?': s.put(x + i, y + j, c)
    def scatter(s, c, n, x, y, w, h, on, seed):
        r = random.Random(seed)
        for _ in range(n * 6):
            if n <= 0: break
            px, py = r.randrange(x, x + w), r.randrange(y, y + h)
            if s.g[py][px] in on: s.g[py][px] = c; n -= 1
    def point(s, name, x, y): POINTS[name] = (x, y)
    def rows(s): return ["".join(r) for r in s.g]

def literal(name, rows):
    w = len(rows[0])
    for r in rows: assert len(r) == w, (name, r, len(r), w)
    m = Map(name, w, len(rows)); m.g = [list(r) for r in rows]
    return m

# ---------------------------------------------------------------- HOME
home = literal("HOME", [
    "IIJIIIIJII",
    "Qiiiiiiipi",
    "iiiiiiiiBi",
    "iiittiiiii",
    "iiittiiiii",
    "piiiiiiiip",
    "iiiiiiiiii",
    "iiiiqqiiii",
])
home.point("HOME_START", 7, 2); home.point("HOME_PC", 0, 1); home.point("HOME_EXIT", 4, 7); home.point("HOME_IN", 4, 6)

# ---------------------------------------------------------------- ROST
rost = literal("ROST", [
    "TTTTTTTTTT_||_TTTTTTTTTT",
    "T;;;;;;;;;_||_;;;;;;;;;T",
    "T;rrrrrrr;_||_;;;RRRRR;T",
    "T;wwnnnww;_||_;;;WWNWW;T",
    "T;wwwDwww;_||_;f;WWDWW;T",
    "T;;;;.;;;;_||_;;;;;.;;;T",
    "T;L;;.;;;;_||_;;;;;.;L;T",
    "T;;;;.......,.......;;;T",
    "T;f;;.;;;;;;.;;;;S;;;;fT",
    "T;;;;.;;;;;;.;;;;;;;;;;T",
    "T;;;;.;;;;;;.;;;;;;;;;;T",
    "T;RRRRR;;;;;.;;;;;;YY;;T",
    "T;NNNNN;;;;;.;;;;;;YY;;T",
    "T;WWDWW;;;;;.;;;;;;;;;;T",
    "T;;;.;;;;;;;.;;;;;;;f;;T",
    "T;L;.........;;;;;;;;;LT",
    "T;;;;;;;;;;;;;;;;;;;;;;T",
    "FFFFFFFFFFFFFFFFFFFFFFFF",
    "~~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~",
])
rost.point("ROST_LAB", 5, 4); rost.point("ROST_HOME", 19, 4); rost.point("ROST_STATION", 4, 13); rost.point("ROST_SIGN", 17, 8)
rost.point("ROST_N", 10, 0)

# ---------------------------------------------------------------- LAB
lab = literal("LAB", [
    "IIJIIGGGGIIJII",
    "MiiiiiiiiiiiiM",
    "iiiiKKKKKiiiii",
    "iiiiiiiiiiiiii",
    "piiiiiiiiiiiip",
    "iiiiiiiiiiiiii",
    "QiiiiiiiiiiiiQ",
    "iiiiiiiiiiiiii",
    "piiiiiqqiiiiip",
])
lab.point("LAB_IN", 6, 7); lab.point("LAB_EXIT", 6, 8)

# ---------------------------------------------------------------- STATION (shared)
station = literal("STATION", [
    "IIJIIIIIJIII",
    "piiiKhKiiQQp",
    "iiiiiiiiiiii",
    "iiiiiiiKKKii",
    "iiiiiiiiiiii",
    "tiiiiiiiiiit",
    "iiiiiiiiiiii",
    "piiiiqqiiiip",
])
station.point("ST_IN", 5, 6); station.point("ST_EXIT", 5, 7); station.point("ST_NURSE", 5, 1); station.point("ST_SHOP", 8, 2)
station.point("ST_BOX", 9, 1)

# ---------------------------------------------------------------- ARENA (shared)
arena = literal("ARENA", [
    "IIIIJIIJIIII",
    "Maaaaaaaaaam",
    "Maaaaaaaaaaa",
    "aaaaaaaaaaaa",
    "aaaaaaaaaaaa",
    "AAAAAAAAAAAA",
    "aaaaaaaaaaaa",
    "aaaaaaaaaaaa",
    "aaaaaaaaaaaa",
    "AAAAAAAAAAAA",
    "aaaaaaaaaaaa",
    "aaaaaaaaaaaa",
    "aaaaaqqaaaaa",
])
arena.g[1][11] = 'M'; arena.g[2][11] = 'M'
arena.point("AR_IN", 5, 11); arena.point("AR_EXIT", 5, 12); arena.point("AR_BOSS", 5, 2); arena.point("AR_T1", 3, 7); arena.point("AR_T2", 8, 4)

# ---------------------------------------------------------------- HOUSE (shared)
house = literal("HOUSE", [
    "IJIIIIJI",
    "piiiiiBp",
    "iiiiiiii",
    "iittiiii",
    "iiiiiiii",
    "Qiiiiiip",
    "iiiqqiii",
])
house.point("HO_IN", 3, 5); house.point("HO_EXIT", 3, 6); house.point("HO_NPC", 5, 3)

# ---------------------------------------------------------------- KABELPFAD
kabel = literal("KABEL", [
    "TTTTTTTT_||_TTTTTTTT",
    "TT;;;;;;_||_;;;;;;TT",
    "T;;***;;_||_;;;;;;;T",
    "T;*****;;..;;****;;T",
    "T;*****;;..;;*****;T",
    "T;;***;;;..;;*****;T",
    "TT;;;;;;;..;;;;;;;TT",
    "TTTFFFF;;..;;FFFFTTT",
    "TY;;;;;;;..;;;;;;;YT",
    "T;;;;~~~~..~~~~;;;;T",
    "T;S;;~~~~==~~~~;;L;T",
    "T;;;;~~~~==~~~~;;;;T",
    "T;;;;;;;;..;;;;;;;;T",
    "T;****;;;..;;;;;;;;T",
    "T;*****;;..;;;***;;T",
    "T;*****;;..;;****;;T",
    "T;;***;;;..;;****;;T",
    "T;;;;;;;;..;;;;;;;;T",
    "TTTT;;;;;..;;;;TTTTT",
    "TTTT;;;;;..;;;;TTTTT",
    "T;;;;;;;;..;;;;;;;;T",
    "T;***;;PP..PP;;***;T",
    "T;***;;;;..;;;;***;T",
    "T;***;;;;..;;;;***;T",
    "T;;;;;;;;..;;;;;;;;T",
    "TY;;;;;;;..;;;;;;;YT",
    "T;;;;;;;;..;;;;;;;;T",
    "T;f;;;;;;..;;;;;;f;T",
    "TT;;;;;;;..;;;;;;;TT",
    "TTT;;;;;;..;;;;;;TTT",
    "TTTT;;;;;..;;;;;TTTT",
    "TTTTT;;;;..;;;;TTTTT",
    "TTTTTT;;_||_;;TTTTTT",
    "TTTTTTTT_||_TTTTTTTT",
])
kabel.point("KA_S", 8, 33); kabel.point("KA_N", 8, 0); kabel.point("KA_SIGN", 2, 10)

# ---------------------------------------------------------------- NEON-DISTRIKT
neon = Map("NEON", 36, 28, '.')
neon.rect(0, 0, 36, 1, 'r')
neon.border('w')
# avenue
neon.rect(1, 13, 35, 2, '_'); neon.rect(1, 13, 35, 1, '-')
neon.rect(16, 5, 4, 23, '_'); neon.rect(17, 5, 2, 23, '|')
neon.rect(16, 13, 4, 2, '_')
# arena (north centre)
neon.building(13, 0, 10, 5, 4, "NE_ARENA", roof='R', wall='n', neon='N', door='E')
neon.put(18, 4, 'E'); POINTS["NE_ARENA2"] = (18, 4)
neon.rect(13, 0, 10, 1, 'R')
# houses
neon.building(2, 1, 6, 4, 2, "NE_H1", roof='r', wall='w', windows=[(1, 'b'), (4, 'N')])
neon.building(26, 1, 7, 4, 3, "NE_GYM2", roof='R', wall='n', neon='N', door='E')
neon.building(8, 6, 5, 3, 2, "NE_H3", roof="R", wall="n", neon="N", door="E")
neon.building(24, 7, 8, 4, 2, "NE_STATION", roof='R', wall='W', neon='n')
neon.rect(24, 7, 8, 1, 'R')
neon.building(23, 17, 6, 4, 2, "NE_H4", roof='r', wall='w', windows=[(4, 'b')])
neon.building(30, 17, 5, 4, 2, "NE_H5", roof='R', wall='N')
# neon park
neon.rect(1, 17, 13, 10, ';')
neon.rect(1, 16, 14, 1, 'F'); neon.rect(14, 16, 1, 11, 'F')
neon.put(12, 16, ';'); neon.put(13, 16, ';')      # park entrance
neon.put(14, 22, ';'); neon.put(14, 23, ';')
neon.rect(2, 18, 4, 3, '*'); neon.rect(7, 19, 5, 3, '*'); neon.rect(2, 23, 6, 3, '*'); neon.rect(9, 23, 4, 3, '*')
for (x, y) in [(1, 17), (6, 17), (12, 18), (1, 22), (8, 22), (13, 26), (6, 26), (1, 26)]: neon.put(x, y, 'Y')
neon.put(6, 22, 'T'); neon.put(12, 21, 'T')
neon.put(10, 17, 'S'); POINTS["NE_PARKSIGN"] = (10, 17)
# lamps & deco
for x in (3, 8, 12, 23, 27, 32):
    neon.put(x, 12, 'L')
neon.put(15, 12, 'S'); POINTS["NE_SIGN"] = (15, 12)
for x in (21, 22): neon.put(x, 26, 'f')
neon.rect(20, 22, 2, 2, 'p' if False else 'P')
# exits
neon.rect(35, 13, 1, 2, '-')
neon.point("NE_E", 35, 13); neon.point("NE_S", 16, 27)
neon.rect(0, 13, 1, 1, '-'); neon.rect(0, 14, 1, 1, '_'); neon.point("NE_W", 0, 13)
neon.rect(16, 27, 4, 1, '_'); neon.rect(17, 27, 2, 1, '|')

# ---------------------------------------------------------------- HYDROKANAL
hyd = Map("HYDRO", 40, 18, '~')
hyd.rect(0, 7, 6, 4, '.')          # west landing
hyd.rect(0, 8, 40, 2, '=')         # long bridge
hyd.rect(0, 8, 6, 2, '_')
hyd.rect(34, 7, 6, 4, '.')
hyd.rect(34, 8, 6, 2, '_')
# islands with datagrass
for (x, y, w, h) in [(7, 2, 8, 5), (18, 11, 9, 5), (27, 1, 7, 6), (8, 12, 7, 4)]:
    hyd.rect(x, y, w, h, ';')
    hyd.rect(x + 1, y + 1, w - 2, h - 2, '*')
hyd.rect(10, 7, 2, 1, 'H'); hyd.rect(21, 10, 2, 1, 'H'); hyd.rect(30, 7, 2, 1, 'H'); hyd.rect(11, 10, 2, 2, 'H')
hyd.put(7, 2, 'T'); hyd.put(14, 2, 'Y'); hyd.put(26, 15, 'T'); hyd.put(18, 15, 'Y'); hyd.put(33, 1, 'T'); hyd.put(8, 15, 'Y')
hyd.put(2, 7, 'S'); POINTS["HY_SIGN"] = (2, 7)
hyd.put(5, 7, 'L'); hyd.put(34, 10, 'L')
hyd.point("HY_W", 0, 8); hyd.point("HY_E", 39, 8)

# ---------------------------------------------------------------- AQUARIS
aq = Map("AQUARIS", 30, 22, '.')
aq.border('w')
aq.rect(0, 0, 30, 1, 'r')
aq.rect(0, 16, 30, 6, '~')
aq.rect(0, 15, 30, 1, 'F')
aq.rect(10, 15, 10, 1, '.'); aq.rect(12, 16, 6, 4, '=')    # pier
aq.rect(1, 8, 29, 2, '_'); aq.rect(1, 8, 29, 1, '-')
aq.rect(0, 8, 1, 2, '-')
aq.rect(14, 1, 2, 14, '|')
aq.building(3, 2, 8, 5, 3, "AQ_ARENA", roof='R', wall='n', neon='N', door='E')
aq.building(19, 2, 7, 4, 3, "AQ_STATION", roof='R', wall='W', neon='n')
aq.building(3, 11, 5, 3, 2, "AQ_H1", roof='r', wall='w')
aq.building(21, 11, 6, 3, 2, "AQ_GYM2", roof='R', wall='n', neon='N', door='E')
aq.rect(14, 0, 2, 1, '|')
for x in (2, 12, 17, 27): aq.put(x, 7, 'L')
aq.put(17, 11, 'S'); POINTS["AQ_SIGN"] = (17, 11)
aq.put(10, 12, 'f'); aq.put(18, 13, 'f'); aq.put(11, 13, 'Y')
aq.rect(12, 16, 6, 6, '='); aq.point("AQ_W", 0, 8); aq.point("AQ_N", 14, 0); aq.point("AQ_S", 12, 21)

# ---------------------------------------------------------------- KRISTALLHÖHLE
cave = Map("CAVE", 30, 30, 'V')
r = random.Random(5)
def carve(x0, y0, x1, y1, wdt=2):
    x, y = x0, y0
    while (x, y) != (x1, y1):
        cave.rect(x, y, wdt, wdt, 'k')
        if x != x1 and (y == y1 or r.random() < 0.5): x += 1 if x1 > x else -1
        else: y += 1 if y1 > y else -1
    cave.rect(x, y, wdt, wdt, 'k')
carve(14, 29, 14, 22, 2)
carve(14, 22, 5, 18, 3)
carve(5, 18, 6, 8, 2)
carve(6, 8, 15, 4, 3)
carve(15, 4, 14, 0, 2)
carve(14, 22, 24, 14, 2)
carve(24, 14, 22, 5, 3)
carve(22, 5, 15, 4, 2)
cave.rect(9, 11, 6, 5, 'k'); cave.rect(18, 18, 5, 4, 'k')
for (x, y) in [(10, 12), (13, 14), (19, 19), (21, 20), (7, 19), (24, 9), (8, 6), (16, 24), (12, 3)]:
    if cave.get(x, y) == 'k': cave.put(x, y, 'C' if (x + y) % 2 else 'c')
cave.scatter('C', 30, 1, 1, 28, 28, ['V'], 11)
cave.scatter('c', 20, 1, 1, 28, 28, ['V'], 12)
cave.rect(14, 28, 2, 2, 'k'); cave.rect(14, 0, 2, 2, 'k')
cave.point("CA_S", 14, 29); cave.point("CA_N", 14, 0)

# ---------------------------------------------------------------- KRISTALLHAIN
kh = Map("KRISHAIN", 28, 22, 'e')
kh.border('c')
kh.rect(13, 21, 2, 1, 'e'); kh.rect(27, 10, 1, 2, 'e')
kh.building(10, 1, 8, 5, 3, "KH_ARENA", roof='R', wall='n', neon='N', door='E')
kh.building(2, 3, 6, 4, 2, "KH_STATION", roof='R', wall='W', neon='n')
kh.building(19, 2, 6, 4, 2, "KH_GYM2", roof='R', wall='n', neon='N', door='E')
kh.rect(25, 0, 2, 11, 'e'); kh.point("KH_N", 25, 0)
kh.building(3, 13, 6, 3, 3, "KH_H2", roof='r', wall='W')
kh.rect(16, 12, 9, 7, 'z'); kh.rect(16, 12, 9, 1, 'e')
kh.rect(1, 10, 27, 2, '.')
kh.rect(13, 6, 2, 16, '.')
for (x, y) in [(10, 8), (18, 8), (4, 18), (9, 19), (25, 19), (16, 19)]: kh.put(x, y, 'C')
kh.put(12, 9, 'S'); POINTS["KH_SIGN"] = (12, 9)
kh.point("KH_S", 13, 21); kh.point("KH_E", 27, 10)

# ---------------------------------------------------------------- GLITCHWÜSTE
gl = Map("GLITCH", 40, 26, 's')
gl.border('V')
gl.rect(0, 11, 1, 2, 's'); gl.rect(39, 11, 1, 2, 's')
for (x, y, w, h) in [(4, 3, 7, 5), (14, 15, 8, 6), (25, 3, 9, 6), (29, 16, 7, 6), (5, 17, 6, 5), (16, 6, 5, 4)]:
    gl.rect(x, y, w, h, 'x')
gl.scatter('O', 18, 1, 1, 38, 24, ['s'], 21)
gl.scatter('g', 12, 1, 1, 38, 24, ['s'], 22)
gl.rect(10, 10, 4, 2, 'Z'); gl.rect(22, 12, 3, 3, 'Z'); gl.rect(33, 11, 2, 1, 'Z')
gl.rect(1, 11, 38, 2, 's')
gl.rect(10, 10, 4, 1, 'Z')
for x in range(0, 40, 9): gl.put(x + 3, 10, 'O')
gl.put(3, 13, 'S'); POINTS["GL_SIGN"] = (3, 13)
gl.rect(19, 13, 2, 13, 's'); gl.point("GL_W", 0, 11); gl.point("GL_E", 39, 11); gl.point("GL_S", 19, 25)

# ---------------------------------------------------------------- SEKTOR NULL
s0 = Map("SEKTOR0", 26, 20, '.')
s0.border('U')
s0.rect(0, 9, 1, 2, '_')
s0.rect(12, 0, 2, 1, 'X'); POINTS["S0_N"] = (12, 0)
s0.rect(1, 9, 25, 2, '_')
s0.rect(12, 1, 2, 18, '|')
s0.building(2, 2, 8, 5, 3, "S0_ARENA", roof='R', wall='n', neon='N', door='E')
s0.building(17, 2, 6, 4, 2, "S0_STATION", roof='R', wall='W', neon='n')
s0.building(3, 13, 5, 3, 2, "S0_H1", roof='r', wall='w')
s0.building(18, 13, 6, 3, 2, "S0_GYM2", roof='R', wall='n', neon='N', door='E')
s0.rect(10, 1, 1, 7, 'F'); s0.rect(15, 1, 1, 7, 'F')
for x in (9, 16): s0.put(x, 8, 'L')
s0.put(11, 12, 'S'); POINTS["S0_SIGN"] = (11, 12)
s0.rect(9, 16, 8, 3, 'G')
s0.point("S0_W", 0, 9); s0.point("S0_GATE", 12, 1); s0.point("S0_E", 25, 9)

# ---------------------------------------------------------------- HELIX TOWER LOBBY
lob = Map("LOBBY", 20, 16, 'u')
lob.border('U')
lob.rect(0, 0, 20, 2, 'U')
lob.rect(9, 15, 2, 1, 'q')
lob.rect(5, 4, 10, 1, 'K'); lob.put(9, 4, 'Q')
lob.rect(9, 2, 2, 1, 'X'); POINTS["LO_WARP"] = (9, 2)
lob.rect(1, 6, 2, 8, 'G'); lob.rect(17, 6, 2, 8, 'G')
lob.rect(8, 6, 4, 9, 'm')
for (x, y) in [(4, 8), (15, 8), (4, 12), (15, 12)]: lob.put(x, y, 'p')
lob.point("LO_S", 9, 15)

# ---------------------------------------------------------------- ORBITALGARTEN
orb = Map("ORBIT", 30, 24, 'u')
orb.border('U')
orb.rect(14, 23, 2, 1, 'X'); POINTS["OR_S"] = (14, 23)
orb.rect(14, 1, 2, 1, 'X'); POINTS["OR_N"] = (14, 1)
orb.rect(14, 1, 2, 22, 'm')
for (x, y, w, h) in [(2, 3, 9, 6), (19, 3, 9, 6), (2, 13, 10, 7), (18, 13, 10, 7)]:
    orb.rect(x, y, w, h, ';'); orb.rect(x + 1, y + 1, w - 2, h - 2, '*')
orb.rect(1, 10, 28, 1, 'J' if False else 'u')
for (x, y) in [(2, 3), (10, 3), (19, 3), (27, 3), (2, 19), (11, 19), (18, 19), (27, 19), (6, 10), (23, 10)]: orb.put(x, y, 'Y')
orb.rect(1, 1, 28, 1, 'b')
orb.put(14, 1, 'X'); orb.put(15, 1, 'X')

# ---------------------------------------------------------------- CORE
core = Map("CORE", 16, 16, 'v')
core.border('G')
core.rect(0, 0, 16, 2, 'G')
core.rect(7, 15, 2, 1, 'X'); POINTS["CO_S"] = (7, 15)
core.rect(3, 3, 10, 9, 'v')
for (x, y) in [(3, 4), (12, 4), (3, 9), (12, 9)]: core.put(x, y, 'M')
core.rect(6, 2, 4, 1, 'M')
core.rect(7, 13, 2, 1, 'v')
core.put(1, 13, 'X'); POINTS["CO_ABYSS"] = (1, 13)
core.point("CO_BOSS", 7, 4); core.point("CO_CEO", 7, 8)

# ---------------------------------------------------------------- DATENABGRUND
ab = Map("ABYSS", 30, 30, 'v')
ab.border('g')
ab.scatter('g', 70, 1, 1, 28, 28, ['v'], 31)
ab.scatter('c', 25, 1, 1, 28, 28, ['v'], 32)
ab.rect(13, 26, 4, 3, 'v'); ab.rect(14, 28, 2, 1, 'X'); POINTS["AB_S"] = (14, 28)
ab.rect(12, 12, 6, 6, 'v'); ab.put(14, 14, 'S'); POINTS["AB_SIGN"] = (14, 14)
for k in range(14):
    ab.rect(14, 12 + k, 2, 1, 'v')
    ab.rect(2 + k, 15, 1, 1, 'v'); ab.rect(14 + k, 15, 1, 1, 'v')


# ---------------------------------------------------------------- LIGA (Hoher Rat)
liga = Map("LIGA", 14, 44, 'u')
liga.border('U')
for k, y in enumerate((34, 24, 14, 4)):
    liga.rect(1, y, 12, 1, 'U')
    liga.put(6, y, 'u'); liga.put(7, y, 'u')
    liga.rect(2, y + 2, 10, 6, 'a')
    liga.rect(2, y + 5, 10, 1, 'A')
    liga.put(1, y + 3, 'G'); liga.put(12, y + 3, 'G')
    liga.point("LI_E%d" % k, 6, y)
liga.rect(6, 43, 2, 1, 'X'); liga.point("LI_S", 6, 43)
liga.rect(6, 1, 2, 1, 'X'); liga.point("LI_N", 6, 1)
liga.rect(1, 40, 12, 2, 'm')

for (mm, sd) in [(neon, 41), (aq, 42), (s0, 43)]:
    mm.scatter(',', 14, 1, 1, mm.w - 2, mm.h - 2, ['.'], sd)
for m in [home, rost, lab, station, arena, house, kabel, neon, hyd, aq, cave, kh, gl, s0, lob, orb, core, ab, liga]:
    MAPS.append((m.name, m.rows()))

exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gen_world.py')).read())

def cstr(s): return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
with open(OUT, "w") as f:
    f.write("// generated by tools/gen_maps.py\n#include \"gen_maps.h\"\n")
    for (n, rows) in MAPS:
        f.write("static const char *const rows_%s[] = {\n" % n)
        for r in rows: f.write("  %s,\n" % cstr(r))
        f.write("};\n")
    f.write("const MapGeom map_geom[%d] = {\n" % len(MAPS))
    for (n, rows) in MAPS:
        f.write("  {%d, %d, rows_%s},\n" % (len(rows[0]), len(rows), n))
    f.write("};\n")
with open(OUT.replace(".c", ".h"), "w") as f:
    f.write("#pragma once\n// generated\ntypedef struct { unsigned char w, h; const char *const *rows; } MapGeom;\nextern const MapGeom map_geom[];\n")
    for i, (n, rows) in enumerate(MAPS):
        f.write("#define M_%s %d\n" % (n, i))
    f.write("#define NUM_MAPS %d\n" % len(MAPS))
    for k, (x, y) in sorted(POINTS.items()):
        f.write("#define P_%s_X %d\n#define P_%s_Y %d\n" % (k, x, k, y))

if len(sys.argv) > 2:
    for (n, rows) in MAPS:
        print(n)
        for r in rows: print("  " + r)
