# -*- coding: utf-8 -*-
# Executed inside gen_maps.py: builds the 8 new cities, 8 routes, gym table, route trainers
# and writes source/gen_world.inc (warps, NPCs, map definitions, gyms, trainers).
import gen_mon as GM

WR = random.Random(777)
EXTRA_WARPS = {}   # existing map name -> list of warp C initialisers
GEN_MAPS = []      # (map_obj, display_name, outside, zone, music, bg, enc_any)
NPCS = {}          # map name -> list of npc initialisers
WARPS = {}         # map name -> list of warp initialisers
GEN_TRAINERS = []  # C initialisers
TRAINER_BASE = 40
ITEM_FLAG = [60]
ROUTE_INFO = {}    # route name -> (map, path cells, walkable chars)
OCC = {}           # map name -> occupied npc cells


def side_cells(mname, side):
    # returns (x, y, w, h) of the exit and the inward step and facing dir
    x, y = POINTS[mname + "_" + side]
    if side in ('N', 'S'):
        return (x, y, 2, 1), (0, 1 if side == 'N' else -1), ('D_DOWN' if side == 'N' else 'D_UP')
    return (x, y, 1, 2), (1 if side == 'W' else -1, 0), ('D_RIGHT' if side == 'W' else 'D_LEFT')


def add_warp(mname, w):
    if mname in EXISTING:
        EXTRA_WARPS.setdefault(mname, []).append(w)
    else:
        WARPS.setdefault(mname, []).append(w)


EXISTING = {"NEON": "NE", "AQUARIS": "AQ", "KRISHAIN": "KH", "GLITCH": "GL", "SEKTOR0": "S0"}


def connect(a, sa, b, sb):
    (ax, ay, aw, ah), _, _ = side_cells(PFX(a), sa)
    (bx, by, bw, bh), (bdx, bdy), bdir = side_cells(PFX(b), sb)
    (_, _, _, _), (adx, ady), adir = side_cells(PFX(a), sa)
    add_warp(a, "{ %d, %d, %d, %d, M_%s, %d, %d, %s, NOF, 0 }" % (ax, ay, aw, ah, b, bx + bdx, by + bdy, bdir))
    add_warp(b, "{ %d, %d, %d, %d, M_%s, %d, %d, %s, NOF, 0 }" % (bx, by, bw, bh, a, ax + adx, ay + ady, adir))


def PFX(name):
    return EXISTING.get(name, name)


def cq(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


# ---------------------------------------------------------------- cities
CITY_THEMES = {
    "CHROM": ("Chromstadt", '.', 'w', ['P', 'L', 'G'], 0, "MUS_TOWN"),
    "DRACHEN": ("Drachenfels", 'k', 'V', ['O', 'C', 'O'], 3, "MUS_TOWN"),
    "GLUT": ("Glutschmiede", 'k', 'V', ['Z', 'O', 'L'], 4, "MUS_TOWN"),
    "HIMMEL": ("Himmelsspitze", 'u', 'U', ['p', 'f', 'L'], 5, "MUS_TOWN"),
    "SCHATTEN": ("Schattenmarkt", 'v', 'g', ['G', 'b', 'L'], 6, "MUS_TOWN"),
    "ORBITAL": ("Orbitalhafen", 'u', 'U', ['M', 'G', 'L'], 5, "MUS_TOWN"),
    "URWALD": ("Urwaldkuppel", ';', 'T', ['Y', 'T', 'f'], 1, "MUS_TOWN"),
    "NANORIFF": ("Nanoriff", '.', '~', ['P', 'c', 'L'], 2, "MUS_TOWN"),
}
CITY_EXITS = {"CHROM": "EW", "DRACHEN": "EWN", "GLUT": "E", "HIMMEL": "SN", "SCHATTEN": "N", "ORBITAL": "W", "URWALD": "S", "NANORIFF": "N"}
GYM_CITY = {"NEON": (0, 1), "AQUARIS": (2, 3), "KRISHAIN": (4, 5), "SEKTOR0": (6, 7), "CHROM": (8, 9), "DRACHEN": (10, 11),
            "GLUT": (12, 13), "HIMMEL": (14, 15), "SCHATTEN": (16, 17), "ORBITAL": (18, 19), "URWALD": (20, 21), "NANORIFF": (22, 23)}

TOWNSFOLK = [
    "Seit Helix die Netzknoten bewacht, trauen sich nur noch mutige Kuriere in die Arenen.",
    "Jeder Arenaleiter passt sich an! Je mehr Orden du hast, desto stärker kämpfen sie.",
    "Man sagt, Lyra Voss habe Protokoll Null auf alle 24 Netzknoten verteilt. Acht Fragmente reichen, um es zusammenzusetzen.",
    "Mein Vexel hat gestern eine Kanone bekommen! Er hat sich zur Ultra-Stufe entwickelt.",
    "Digitale Wesen haben Attribute: Vakzin schlägt Virus, Virus schlägt Daten, Daten schlägt Vakzin.",
    "Die Fähigkeit eines Vexels kann Kämpfe entscheiden. Schau im Status-Menü nach!",
    "Mit acht Orden öffnet sich der Helix-Turm in Sektor Null. Dort wartet der Hohe Rat.",
    "Von hier aus kommst du überallhin. Kein Weg ist gesperrt – nur deine Stärke setzt Grenzen.",
    "Die Mega-Stufe ist der Gipfel der Evolution. Nur wenige haben sie je gesehen.",
    "Arsenal-Vexel tragen echte Waffen. Bleib lieber auf Abstand!",
    "Die Datenautobahn verbindet den Neon-Distrikt mit Chromstadt.",
    "In der Glutschmiede werden Kapseln aus Sternenstahl gegossen.",
]


def make_city(key):
    disp, floor, border, deco, bg, mus = CITY_THEMES[key]
    m = Map(key, 32, 24, floor)
    m.border(border)
    m.rect(1, 11, 30, 2, '_'); m.rect(1, 11, 30, 1, '-')
    m.rect(14, 1, 4, 22, '_'); m.rect(15, 1, 2, 22, '|'); m.rect(14, 11, 4, 2, '_')
    m.building(2, 2, 10, 5, 4, key + "_GYM1", roof='R', wall='n', neon='N', door='E')
    m.building(20, 2, 10, 5, 5, key + "_GYM2", roof='R', wall='n', neon='N', door='E')
    m.rect(6, 7, 1, 4, ','); m.rect(25, 7, 1, 4, ',')
    m.building(3, 15, 7, 4, 3, key + "_STATION", roof='R', wall='W', neon='n')
    m.rect(6, 13, 1, 2, ',')
    m.building(21, 15, 6, 3, 2, key + "_H1", roof='r', wall='w')
    m.building(24, 19, 5, 3, 2, key + "_H2", roof='r', wall='N')
    m.rect(23, 18, 1, 1, ','); m.rect(26, 22, 1, 1, ',')
    m.put(13, 9, 'S'); m.point(key + "_SIGN", 13, 9)
    for (x, y) in ((4, 10), (27, 10), (12, 14), (19, 14)):
        m.put(x, y, 'L')
    sd = WR.randrange(1000)
    for ch in deco:
        m.scatter(ch, 5, 1, 1, 30, 22, [floor], sd + ord(ch))
    # keep roads and doorsteps clear
    for x in range(1, 31):
        for y in (10, 13):
            if m.get(x, y) in deco and m.get(x, y) != 'L':
                m.put(x, y, floor)
    for (x, y) in (POINTS[key + "_GYM1"], POINTS[key + "_GYM2"], POINTS[key + "_STATION"], POINTS[key + "_H1"], POINTS[key + "_H2"]):
        if m.get(x, y + 1) not in '_|-,.':
            m.put(x, y + 1, ',')
    # variety: mirror every other city and vary facades
    idx = list(CITY_THEMES).index(key)
    if idx % 2 == 1:
        for row in m.g:
            row.reverse()
        for nm in [key + s_ for s_ in ("_GYM1", "_GYM2", "_STATION", "_H1", "_H2", "_SIGN")]:
            x, y = POINTS[nm]
            POINTS[nm] = (31 - x, y)
    facade = ['W', 'w', 'N', 'n'][idx % 4]
    for y in range(m.h):
        for x in range(m.w):
            if m.g[y][x] == 'W' and y >= 14:
                m.g[y][x] = facade
    ex = CITY_EXITS[key]
    if 'N' in ex: m.rect(15, 0, 2, 1, '|'); m.point(key + "_N", 15, 0)
    if 'S' in ex: m.rect(15, 23, 2, 1, '|'); m.point(key + "_S", 15, 23)
    if 'W' in ex: m.rect(0, 11, 1, 2, '_'); m.point(key + "_W", 0, 11)
    if 'E' in ex: m.rect(31, 11, 1, 2, '_'); m.point(key + "_E", 31, 11)
    if key == "NANORIFF":
        m.rect(14, 0, 4, 1, '='); m.rect(15, 0, 2, 1, '|')
    GEN_MAPS.append((m, disp, border, -1, mus, bg, 0))
    g1, g2 = GYM_CITY[key]
    W = WARPS.setdefault(key, [])
    gx, gy = POINTS[key + "_GYM1"]
    W.append("{ %d, %d, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, %d }" % (gx, gy, g1 + 1))
    gx, gy = POINTS[key + "_GYM2"]
    W.append("{ %d, %d, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, %d }" % (gx, gy, g2 + 1))
    sx, sy = POINTS[key + "_STATION"]
    W.append("{ %d, %d, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF, 0 }" % (sx, sy))
    for h in ("_H1", "_H2"):
        hx, hy = POINTS[key + h]
        W.append("{ %d, %d, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF, 0 }" % (hx, hy))
    N = NPCS.setdefault(key, [])
    sgx, sgy = POINTS[key + "_SIGN"]
    N.append('{ %d, %d, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, %s }' % (sgx, sgy, cq("%s|Netzknoten-Arenen: %s und %s." % (disp.upper(), GYMS[g1][0], GYMS[g2][0]))))
    folks = [("CH_WOMAN", 8, 13), ("CH_ELDER", 22, 9), ("CH_KID", 11, 20), ("CH_RUNNER", 27, 13)]
    if idx % 2 == 1:
        folks = [(c_, 31 - x, y) for (c_, x, y) in folks]
    for k, (chr_, x, y) in enumerate(folks):
        if m.get(x, y) not in (floor, ',', '.', '_', '-', ';', 'u', 'k', 'v'):
            continue
        txt = TOWNSFOLK[(len(GEN_MAPS) * 3 + k) % len(TOWNSFOLK)]
        N.append('{ %d, %d, %s, D_DOWN, NK_TALK, 0, NOF, NOF, "Bewohner", %s }' % (x, y, chr_, cq(txt)))
    # one item
    ix, iy = (28, 21) if idx % 2 == 0 else (3, 21)
    if m.get(ix, iy) == floor:
        N.append('{ %d, %d, 0, 0, NK_ITEM, %s, NOF, %d, NULL, NULL }' % (ix, iy, WR.choice(["IT_SUPERKAPSEL", "IT_SUPERKIT", "IT_ZELLE", "IT_HYPERKAPSEL"]), ITEM_FLAG[0]))
        ITEM_FLAG[0] += 1
    return m


# ---------------------------------------------------------------- routes
ROUTES = {
    "R_DATA": ("Datenautobahn", True, '_', 'w', '*', ['P', 'L', 'b'], 7, 0),
    "R_SCHROTT": ("Schrottpass", True, 'k', 'V', 'x', ['O', 'P', 'G'], 8, 4),
    "R_LAVA": ("Lavafeld", True, 'k', 'V', 'x', ['Z', 'O', 'Z'], 9, 4),
    "R_TIEFSEE": ("Tiefseesteg", False, 'H', '~', '*', ['~', '~', 'c'], 10, 2),
    "R_HIMMEL": ("Himmelstreppe", False, 'e', 'c', 'z', ['C', 'c', 'O'], 11, 3),
    "R_SCHATTEN": ("Schattengasse", False, 'v', 'g', '*', ['g', 'G', 'c'], 12, 6),
    "R_RAUM": ("Raumhafenstraße", True, 'u', 'U', '*', ['M', 'G', 'p'], 13, 5),
    "R_DSCHUNGEL": ("Dschungelpfad", False, ';', 'T', '*', ['T', 'Y', 'f'], 14, 1),
}
CLASSES = [("Kurier", "CH_RUNNER"), ("Hackerin", "CH_PUNK"), ("Söldner", "CH_GRUNT"), ("Pilotin", "CH_WOMAN"), ("Ingenieur", "CH_SCIENTIST"),
           ("Bikerin", "CH_PUNK"), ("Mönch", "CH_ELDER"), ("Jägerin", "CH_WOMAN"), ("Schmied", "CH_ELDER"), ("Kadett", "CH_KID"),
           ("Taucherin", "CH_RUNNER"), ("Ninja", "CH_GRUNT")]
FIRST = ["Jax", "Mira", "Kenji", "Lena", "Oskar", "Yuki", "Tariq", "Zoe", "Bram", "Ines", "Rico", "Selma", "Dex", "Nia", "Hugo", "Vera",
         "Pavel", "Lou", "Arik", "Tessa", "Milo", "Juna", "Falk", "Rhea"]
INTROS = ["Mein Team ist aufgerüstet. Zeig mir deins!", "Auf dieser Route komme ich an niemandem vorbei – und du nicht an mir!",
          "Ich hab gerade eine Mega-Evolution gesehen. Jetzt bin ich motiviert!", "Lass uns sehen, wessen Code sauberer ist!",
          "Hier kämpfen nur Kuriere mit Mut.", "Meine Vexel haben Kanonen. Und du?", "Ich trainiere für die Arena. Du bist mein Testlauf!",
          "Ein Kampf pro Kurier – so ist die Regel hier."]
LOSES = ["Systemfehler... ich hab verloren.", "Okay, du bist echt stark.", "Ich muss zurück ins Training.", "Respekt, Kurier!",
         "Das war knapp... für mich.", "Meine Kanonen waren nicht genug.", "Nächstes Mal patch ich mein Team!", "Gut gekämpft."]


def make_route(key):
    disp, horiz, floor, border, encch, deco, zone, bg = ROUTES[key]
    w, h = (40, 20) if horiz else (20, 40)
    base_floor = floor
    fill = {'=': '~', 'H': '~'}.get(floor, floor)
    m = Map(key, w, h, fill)
    m.border(border)
    rr = random.Random(sum(ord(ch) * (i + 1) for i, ch in enumerate(key)))
    path = set()
    if horiz:
        y = 9
        for x in range(w):
            if 2 < x < w - 3 and rr.random() < 0.18:
                ny = max(3, min(h - 5, y + rr.choice([-1, 1])))
                for yy_ in range(min(y, ny), max(y, ny) + 2):
                    path.add((x, yy_))
                y = ny
            if x >= w - 3:
                # return to row 9 at the end
                while y != 9:
                    ny = y + (1 if y < 9 else -1)
                    path.add((x - 1, ny)); path.add((x - 1, ny + 1))
                    y = ny
            path.add((x, y)); path.add((x, y + 1))
        m.point(key + "_W", 0, 9); m.point(key + "_E", w - 1, 9)
    else:
        x = 9
        for y in range(h):
            if 2 < y < h - 3 and rr.random() < 0.18:
                nx = max(3, min(w - 5, x + rr.choice([-1, 1])))
                for xx_ in range(min(x, nx), max(x, nx) + 2):
                    path.add((xx_, y))
                x = nx
            if y >= h - 3:
                while x != 9:
                    nx = x + (1 if x < 9 else -1)
                    path.add((nx, y - 1)); path.add((nx + 1, y - 1))
                    x = nx
            path.add((x, y)); path.add((x + 1, y))
        m.point(key + "_N", 9, 0); m.point(key + "_S", 9, h - 1)
    for (x, y) in path:
        m.put(x, y, base_floor)
    # widen interior so it is not a corridor: open fields around the path
    for (x, y) in list(path):
        for dx in range(-3, 4):
            for dy in range(-3, 4):
                X, Y = x + dx, y + dy
                if 1 <= X < w - 1 and 1 <= Y < h - 1 and m.get(X, Y) == fill and rr.random() < 0.85 and fill == '~' and abs(dx) < 2 and abs(dy) < 2:
                    pass
    if fill == '~':
        # sea route: wide bridges and platforms
        for (x, y) in path:
            for dx in (-1, 0, 1, 2):
                X = x + dx
                if 1 <= X < w - 1:
                    m.put(X, y, base_floor)
        for k in range(6):
            px_, py_ = rr.randrange(2, w - 7), rr.randrange(3, h - 7)
            m.rect(px_, py_, 5, 4, '.')
            m.rect(px_ + 1, py_ + 1, 3, 2, encch)
            # connect platform to path with a bridge
            tx = 9
            for X in range(min(px_ + 2, tx), max(px_ + 2, tx) + 1):
                if m.get(X, py_ + 1) == '~':
                    m.put(X, py_ + 1, '=')
    else:
        # encounter patches
        for k in range(9):
            pw, ph = rr.randrange(3, 7), rr.randrange(2, 5)
            px_, py_ = rr.randrange(1, w - pw - 1), rr.randrange(1, h - ph - 1)
            m.rect(px_, py_, pw, ph, encch)
        # obstacles (not on or next to the path)
        near = set()
        for (x, y) in path:
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    near.add((x + dx, y + dy))
        for k in range(55):
            x, y = rr.randrange(1, w - 1), rr.randrange(1, h - 1)
            if (x, y) in near or m.get(x, y) not in (fill, encch):
                continue
            m.put(x, y, rr.choice(deco))
        # make sure path stays walkable
        for (x, y) in path:
            if m.get(x, y) not in (base_floor, encch):
                m.put(x, y, base_floor)
    GEN_MAPS.append((m, disp, border, zone, "MUS_ROUTE", bg, 0))
    ROUTE_INFO[key] = (m, path, (fill, base_floor, encch, '.', '='))
    # NPCs: sign, trainers, items
    N = NPCS.setdefault(key, [])
    plist = sorted(path)
    sx, sy = (2, 7) if horiz else (7, 2)
    if m.get(sx, sy) in (fill, base_floor, encch, '.'):
        m.put(sx, sy, 'S')
        N.append('{ %d, %d, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, %s }' % (sx, sy, cq("%s|Wilde Vexel werden mit jedem Orden stärker." % disp.upper())))
    zone_lines = GM.zone_lines[zone]
    placed = 0
    cands = plist[len(plist) // 6:]
    rr.shuffle(cands)
    used = set()
    for (x, y) in cands:
        if placed >= 4:
            break
        # stand next to the path, facing it
        for (dx, dy, d) in ((0, -1, "D_DOWN"), (0, 1, "D_UP"), (-1, 0, "D_RIGHT"), (1, 0, "D_LEFT")):
            X, Y = x + dx, y + dy
            if (X, Y) in path or not (1 <= X < w - 1 and 1 <= Y < h - 1):
                continue
            if m.get(X, Y) not in (fill, base_floor, encch, '.', '=', 'H'):
                continue
            if any(abs(X - ux) + abs(Y - uy) < 6 for (ux, uy) in used):
                continue
            tid = TRAINER_BASE + len(GEN_TRAINERS)
            cls, chr_ = CLASSES[(tid * 5) % len(CLASSES)]
            name = "%s %s" % (cls, FIRST[tid % len(FIRST)])
            n = 2 + (tid % 3)
            team = []
            base_lv = GM.ZONES[zone][2] + 2
            for k in range(n):
                li = zone_lines[(tid * 3 + k * 5) % len(zone_lines)]
                team.append("{ %d, %d }" % (li << 3, base_lv + k))
            GEN_TRAINERS.append('{ %s, %s, %d, { %s }, %d, %s, %s, 0, %d },' % (
                cq(name), chr_, n, ", ".join(team), 150 + 40 * n, cq(INTROS[tid % len(INTROS)]), cq(LOSES[tid % len(LOSES)]), bg))
            m.put(X, Y, base_floor if base_floor not in '=H' else '.')
            OCC.setdefault(key, set()).add((X, Y))
            N.append('{ %d, %d, %s, %s, NK_TRAINER, %d, NOF, NOF, NULL, NULL }' % (X, Y, chr_, d, tid))
            used.add((X, Y))
            placed += 1
            break
    for k in range(2):
        for _ in range(100):
            x, y = rr.randrange(2, w - 2), rr.randrange(2, h - 2)
            if m.get(x, y) in (fill, base_floor, encch) and fill != '~' and (x, y) not in path and all(abs(x - ux) + abs(y - uy) > 2 for (ux, uy) in used):
                OCC.setdefault(key, set()).add((x, y))
                N.append('{ %d, %d, 0, 0, NK_ITEM, %s, NOF, %d, NULL, NULL }' % (x, y, rr.choice(["IT_KIT", "IT_SUPERKAPSEL", "IT_ANTIVIRUS", "IT_SUPERKIT", "IT_REBOOT", "IT_ZELLE"]), ITEM_FLAG[0]))
                ITEM_FLAG[0] += 1
                used.add((x, y))
                break
    return m


# ---------------------------------------------------------------- gyms
T = {n: i for i, n in enumerate(GM.TYPES)}
GYMS = [
    ("Volta", "Volt", "Volt", "CH_WARDEN_VOLT", "Ich bin Volta, Wächterin des Volt-Knotens. Lyra hat mir ein Fragment anvertraut. Beweise, dass du es wert bist!", "Deine Vexel leuchten heller als jede Neonröhre."),
    ("Byte", "Cyber", "Cyber", "CH_GYM1", "Ich bin Byte, Glitchkönig des Neon-Distrikts. Ich hab dein Team schon gescannt. Langweilig.", "Moment... mein Scan war fehlerhaft?!"),
    ("Marin", "Hydro", "Hydro", "CH_WARDEN_HYDRO", "Die Flut nimmt, die Flut gibt. Ich bin Marin. Zeig mir, ob dein Wille fließt oder bricht!", "Wie ein Strom, der sich seinen Weg bahnt."),
    ("Nerea", "Toxin", "Toxin", "CH_GYM3", "Hafenchemie ist mein Spezialgebiet. Meine Vexel sind giftiger als das Hafenbecken.", "Du bist immun gegen mich. Beeindruckend."),
    ("Silja", "Kristall", "Kristall", "CH_WARDEN_KRIS", "Kristalle vergessen nichts. Ich erinnere mich an Lyra. Und ich prüfe dich, wie ich sie geprüft habe.", "Klar wie Kristall: du bist bereit."),
    ("Aurora", "Psi", "Psi", "CH_GYM5", "Ich habe diesen Kampf schon geträumt. Ich kenne jede deiner Entscheidungen.", "Dieses Ende... habe ich nicht gesehen."),
    ("Umbra", "Void", "Void", "CH_WARDEN_VOID", "Ich bin Umbra. Ich bewache ein Fragment – und die Wahrheit über OMEGA.", "Die Leere hat dich anerkannt."),
    ("Cipher", "Virus", "Virus", "CH_GYM7", "Ich bin Cipher. Mein Team ist ein Virus – und du bist das System.", "Firewall... aktiv? Unmöglich."),
    ("Ferro", "Metall", "Metall", "CH_GYM8", "Willkommen in Chromstadt! Hier wird alles geschmiedet – auch Champions.", "Du bist härter als Titanstahl."),
    ("Gunnar", "Arsenal", "Arsenal", "CH_GYM9", "Ziel erfasst. Kanonen geladen. Ich bin Gunnar, und ich treffe immer.", "Munition leer. Du hast gewonnen."),
    ("Drakon", "Draco", "Draco", "CH_GYM10", "Die Drachen von Drachenfels sind älter als das Netz. Spür ihren Zorn!", "Ein wahrer Drachenbändiger..."),
    ("Terra", "Gravo", "Gravo", "CH_GYM11", "Ich bin Terra. Gegen die Schwerkraft gewinnt niemand.", "Du hast die Schwerkraft besiegt?!"),
    ("Pyra", "Plasma", "Plasma", "CH_GYM12", "Hier in der Glutschmiede brennt alles. Auch deine Hoffnung!", "Dein Feuer brennt heller als meins."),
    ("Ignis", "Plasma", "Draco", "CH_GYM13", "Plasma und Drachenfeuer – mein Team ist ein lebender Vulkan!", "Der Vulkan ist erloschen..."),
    ("Seraphina", "Sakral", "Sakral", "CH_GYM14", "Das Licht des alten Netzes schützt mich. Bist du rein genug?", "Dein Herz ist reiner als mein Licht."),
    ("Zephyr", "Aero", "Aero", "CH_GYM15", "Hoch über den Wolken bin ich unbesiegbar. Fang mich doch!", "Du hast mich vom Himmel geholt!"),
    ("Mephista", "Dämon", "Dämon", "CH_GYM16", "Willkommen auf dem Schattenmarkt. Hier wird mit Seelen bezahlt.", "Diesen Handel habe ich verloren."),
    ("Malik", "Dämon", "Virus", "CH_GYM17", "Dämonischer Code, infiziert mit Viren. Du wirst nicht unbeschadet gehen.", "Mein Code... gelöscht."),
    ("Stella", "Kosmos", "Kosmos", "CH_GYM18", "Ich bin Stella. Die Sterne haben mir deine Ankunft verraten.", "Ein neuer Stern ist geboren."),
    ("Orion", "Kosmos", "Psi", "CH_GYM19", "Kosmische Gedanken, unendlicher Raum. Kannst du mit mir mithalten?", "Das Universum ist groß. Du auch."),
    ("Flora", "Bio", "Bio", "CH_GYM20", "Die Urwaldkuppel ist mein Reich. Jede Ranke gehorcht mir.", "Du bist gewachsen, Kurier."),
    ("Rex", "Bestie", "Bestie", "CH_GYM21", "GRAAH! Mein Rudel jagt alles, was sich bewegt!", "Du bist der Alpha. Respekt."),
    ("Nanette", "Nano", "Nano", "CH_GYM22", "Millionen Nanobots gegen dich allein. Rechne dir deine Chancen aus.", "Meine Rechnung ging nicht auf."),
    ("Kobalt", "Nano", "Metall", "CH_GYM23", "Ich baue Vexel aus Nanostahl. Unzerstörbar. Bisher.", "Bisher... war gestern."),
]
GYM_MAPS = ["NEON", "NEON", "AQUARIS", "AQUARIS", "KRISHAIN", "KRISHAIN", "SEKTOR0", "SEKTOR0", "CHROM", "CHROM", "DRACHEN", "DRACHEN",
            "GLUT", "GLUT", "HIMMEL", "HIMMEL", "SCHATTEN", "SCHATTEN", "ORBITAL", "ORBITAL", "URWALD", "URWALD", "NANORIFF", "NANORIFF"]

for key in ["CHROM", "DRACHEN", "GLUT", "HIMMEL", "SCHATTEN", "ORBITAL", "URWALD", "NANORIFF"]:
    make_city(key)
for key in ROUTES:
    make_route(key)
connect("NEON", "W", "R_DATA", "E")
connect("R_DATA", "W", "CHROM", "E")
connect("CHROM", "W", "R_SCHROTT", "E")
connect("R_SCHROTT", "W", "DRACHEN", "E")
connect("DRACHEN", "W", "R_LAVA", "E")
connect("R_LAVA", "W", "GLUT", "E")
connect("DRACHEN", "N", "R_DSCHUNGEL", "S")
connect("R_DSCHUNGEL", "N", "URWALD", "S")
POINTS["AQ_S"] = (14, 21)
connect("AQUARIS", "S", "R_TIEFSEE", "N")
connect("R_TIEFSEE", "S", "NANORIFF", "N")
connect("KRISHAIN", "N", "R_HIMMEL", "S")
connect("R_HIMMEL", "N", "HIMMEL", "S")
connect("GLITCH", "S", "R_SCHATTEN", "N")
connect("R_SCHATTEN", "S", "SCHATTEN", "N")
connect("SEKTOR0", "E", "R_RAUM", "W")
connect("R_RAUM", "E", "ORBITAL", "W")

exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gen_post.py')).read())

for (m, disp, border, zone, mus, bg, enc_any) in GEN_MAPS:
    MAPS.append((m.name, m.rows()))

with open(os.path.join(os.path.dirname(OUT), "gen_world.inc"), "w") as f:
    f.write("// generated by tools/gen_world.py\n")
    for (m, disp, border, zone, mus, bg, enc_any) in GEN_MAPS:
        f.write("static const Warp w_%s[] = {\n    %s\n};\n" % (m.name, ",\n    ".join(WARPS.get(m.name, []))))
        npcs = NPCS.get(m.name, [])
        if npcs:
            f.write("static const NpcDef n_%s[] = {\n    %s\n};\n" % (m.name, ",\n    ".join(npcs)))
    for k, lst in EXTRA_WARPS.items():
        f.write("static const Warp xw_%s[] = {\n    %s\n};\n" % (k, ",\n    ".join(lst)))
    f.write("#define GEN_MAPDEFS \\\n")
    for (m, disp, border, zone, mus, bg, enc_any) in GEN_MAPS:
        nn = ("n_%s, N(n_%s)" % (m.name, m.name)) if NPCS.get(m.name) else "NULL, 0"
        f.write("    [M_%s] = { %s, '%s', %d, %s, %d, %d, w_%s, N(w_%s), %s, NULL, 0, NULL, 0 }, \\\n" % (
            m.name, cq(disp), border, zone, mus, bg, enc_any, m.name, m.name, nn))
    f.write("\n")
with open(os.path.join(os.path.dirname(OUT), "gen_gyms.inc"), "w") as f:
    f.write("// generated by tools/gen_world.py\n")
    for g, (name, t1, t2, chr_, intro, lose) in enumerate(GYMS):
        f.write('{ %s, %d, %d, %s, M_%s, %s, %s },\n' % (cq(name), T[t1], T[t2], chr_, GYM_MAPS[g], cq(intro), cq(lose)))
write_post()
with open(os.path.join(os.path.dirname(OUT), "gen_trainers.inc"), "w") as f:
    f.write("// generated by tools/gen_world.py (ids from %d)\n" % TRAINER_BASE)
    for t in GEN_TRAINERS:
        f.write(t + "\n")
print("generated maps", len(GEN_MAPS), "route trainers", len(GEN_TRAINERS), "item flags up to", ITEM_FLAG[0])
