# -*- coding: utf-8 -*-
# Executed inside gen_world.py: post-champion content.
# Aetherberg (legendary mountain), Champion-Halle, legendary spots and the 28 recurring challengers
# (written to source/gen_champ.inc).

# ---------------------------------------------------------------- AETHERBERG
def make_aetherberg():
    w, h = 24, 40
    m = Map("AETHERBERG", w, h, 'V')
    rr = random.Random(4711)
    way = [(11, 39), (11, 33), (4, 33), (4, 26), (17, 26), (17, 19), (6, 19), (6, 13), (11, 13), (11, 8)]
    path = set()
    for (x0, y0), (x1, y1) in zip(way, way[1:]):
        for x in range(min(x0, x1), max(x0, x1) + 2):
            for y in range(min(y0, y1), max(y0, y1) + 2):
                if y < h:
                    path.add((x, y))
    for (x, y) in path:
        m.put(x, y, 'k' if y >= 20 else 'e')
    # ledges with wild Vexel next to the path
    cells = sorted(path)
    for k in range(16):
        x, y = cells[rr.randrange(len(cells))]
        if y > 36 or y < 9:
            continue
        pw, ph = rr.randrange(3, 6), rr.randrange(2, 4)
        px_, py_ = x + rr.choice([-pw, 2]), y - ph // 2
        for j in range(ph):
            for i in range(pw):
                X, Y = px_ + i, py_ + j
                if 1 <= X < w - 1 and 9 <= Y < h - 2 and m.get(X, Y) == 'V':
                    m.put(X, Y, '*' if Y >= 20 else 'z')
    # drop ledges that are not connected to the path
    seen, todo = set(path), list(path)
    while todo:
        x, y = todo.pop()
        for (a, b) in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            X, Y = x + a, y + b
            if (X, Y) not in seen and 0 <= X < w and 0 <= Y < h and m.get(X, Y) in '*z':
                seen.add((X, Y)); todo.append((X, Y))
    for y in range(h):
        for x in range(w):
            if m.get(x, y) in '*z' and (x, y) not in seen:
                m.put(x, y, 'V')
    # summit plateau
    m.rect(3, 1, 18, 8, 'e')
    m.rect(9, 2, 6, 4, 'm')
    for (x, y) in ((3, 1), (20, 1), (3, 8), (20, 8), (6, 2), (17, 2), (5, 6), (18, 6)):
        m.put(x, y, 'C')
    for (x, y) in ((8, 1), (15, 1)):
        m.put(x, y, 'c')
    m.put(7, 7, 'S'); m.point("AE_SIGN", 7, 7)
    m.point("AETHERBERG_S", 11, 39)
    m.point("AE_HERMIT", 13, 35)
    m.point("LEG7", 11, 3)
    m.point("AE_XENO", 11, 6)
    m.border('V')
    m.rect(11, 39, 2, 1, 'k')
    GEN_MAPS.append((m, "Aetherberg", 'V', 15, "MUS_CAVE", 3, 0))
    N = NPCS.setdefault("AETHERBERG", [])
    N.append('{ 7, 7, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, %s }' % cq("AETHERGIPFEL|Hier schläft der erste Code. Nur wer sieben Urcodes geweckt hat, wird ihn hören."))
    N.append('{ 13, 35, CH_ELDER, D_LEFT, NK_SCRIPT, S_HERMIT, NOF, NOF, "Einsiedler", NULL }')
    for k in range(2):
        for _ in range(200):
            x, y = rr.randrange(2, w - 2), rr.randrange(10, h - 3)
            if m.get(x, y) in ('*', 'z') and (x, y) not in path:
                N.append('{ %d, %d, 0, 0, NK_ITEM, %s, NOF, %d, NULL, NULL }' % (x, y, ["IT_HYPERKAPSEL", "IT_MAXKIT"][k], ITEM_FLAG[0]))
                ITEM_FLAG[0] += 1
                break
    return m


# ---------------------------------------------------------------- CHAMPION-HALLE (Neon-Distrikt)
champ = literal("CHAMPHALL", [
    "UUUUUUbbUUUUUU",
    "UpuuuMMMMuuupU",
    "UuuuuummuuuuuU",
    "UGuuuummuuuuQU",
    "UuaaaammaaaauU",
    "UuaaaammaaaauU",
    "UuAAAAmmAAAAuU",
    "UuaaaammaaaauU",
    "UuaaaammaaaauU",
    "UuuuuummuuuuuU",
    "UpuuuummuuuupU",
    "UUUUUUqqUUUUUU",
])
champ.point("CHH_IN", 6, 10); champ.point("CHH_EXIT", 6, 11)
GEN_MAPS.append((champ, "Champion-Halle", 'U', -1, "MUS_TOWER", 5, 0))
WARPS["CHAMPHALL"] = ["{ 6, 11, 2, 1, M_NEON, P_NE_H3_X, P_NE_H3_Y + 1, D_DOWN, NOF, 0 }"]
NPCS["CHAMPHALL"] = [
    '{ 6, 1, 0, 0, NK_SCRIPT, S_THRONE, NOF, NOF, NULL, NULL }',
    '{ 7, 1, 0, 0, NK_SCRIPT, S_THRONE, NOF, NOF, NULL, NULL }',
    '{ 12, 3, 0, 0, NK_SCRIPT, S_RANKING, NOF, NOF, NULL, NULL }',
    '{ 2, 2, CH_NURSEBOT, D_DOWN, NK_SCRIPT, S_HALL_NURSE, NOF, NOF, "NURSE-0", NULL }',
    '{ 9, 2, CH_SCIENTIST, D_DOWN, NK_TALK, 0, NOF, NOF, "Hallensprecherin", %s }' % cq(
        "Willkommen in deiner Halle, Champion! Am Thron kannst du deinen Titel verteidigen: drei Herausforderer hintereinander, ohne Heilung.|Am Terminal rechts siehst du die Rangliste deiner Herausforderer."),
]

make_aetherberg()
connect("HIMMEL", "N", "AETHERBERG", "S")
NPCS["HIMMEL"].append('{ 15, 1, CH_GRUNT, D_DOWN, NK_SCRIPT, S_AETHER_GUARD, NOF, F_CHAMPION, "Bergwächter", NULL }')
NPCS["HIMMEL"].append('{ 16, 1, CH_GRUNT, D_DOWN, NK_SCRIPT, S_AETHER_GUARD, NOF, F_CHAMPION, "Bergwächter", NULL }')


# ---------------------------------------------------------------- legendary spots
def route_spot(key, frac):
    m, path, walk = ROUTE_INFO[key]
    plist = sorted(path, key=lambda c: (c[1], c[0]) if m.h > m.w else (c[0], c[1]))
    start = int(len(plist) * frac)
    occ = OCC.get(key, set())
    for (x, y) in plist[start:] + plist[:start]:
        for (dx, dy) in ((0, -2), (0, 2), (-2, 0), (2, 0), (0, -1), (0, 1), (-1, 0), (1, 0)):
            X, Y = x + dx, y + dy
            if (X, Y) in path or not (2 <= X < m.w - 2 and 2 <= Y < m.h - 2):
                continue
            if m.get(X, Y) not in walk or m.get(X, Y) == '~':
                continue
            if any(abs(X - ox) + abs(Y - oy) < 3 for (ox, oy) in occ):
                continue
            # needs a walkable neighbour
            if not any(m.get(X + a, Y + b) in walk for (a, b) in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                continue
            occ.add((X, Y))
            return (X, Y)
    raise SystemExit("no legend spot on " + key)


LEGEND_SPOTS = [
    ("R_HIMMEL",) + route_spot("R_HIMMEL", 0.5),
    ("R_TIEFSEE",) + route_spot("R_TIEFSEE", 0.6),
    ("CAVE", 12, 7),
    ("R_LAVA",) + route_spot("R_LAVA", 0.4),
    ("R_DSCHUNGEL",) + route_spot("R_DSCHUNGEL", 0.5),
    ("R_SCHATTEN",) + route_spot("R_SCHATTEN", 0.6),
    ("ORBIT", 9, 10),
    ("AETHERBERG",) + POINTS["LEG7"],
]

# ---------------------------------------------------------------- recurring challengers
CHALLENGERS = [  # name, chr, t1, t2, intro, lose
    ("Kai", "CH_KAI", "Plasma", "Draco", "Champion! Ich hab dir doch gesagt, dass ich dich wieder herausfordere. Diesmal gehört der Titel mir!", "Immer noch nicht... Aber ich hole auf!"),
    ("Volta", "CH_WARDEN_VOLT", "Volt", "Volt", "Seit du Champion bist, trainiere ich doppelt so hart. Spür die Spannung!", "Kurzschluss. Schon wieder."),
    ("Marin", "CH_WARDEN_HYDRO", "Hydro", "Toxin", "Die Flut kommt zurück, Champion. Immer wieder.", "Ebbe. Für heute."),
    ("Silja", "CH_WARDEN_KRIS", "Kristall", "Psi", "Lyra hat mich gebeten, dich auf Trab zu halten. Bereit?", "Klar wie Kristall: du bist besser."),
    ("Umbra", "CH_WARDEN_VOID", "Void", "Dämon", "Die Leere vergisst niemanden. Auch keinen Champion.", "Die Leere... verneigt sich."),
    ("Gunnar", "CH_GYM9", "Arsenal", "Metall", "Neue Kanonen, neues Glück! Ziel erfasst: Champion.", "Nachladen... dauert noch."),
    ("Drakon", "CH_GYM10", "Draco", "Aero", "Meine Drachen sind gewachsen. Und hungrig.", "Die Drachen ziehen sich zurück."),
    ("Stella", "CH_GYM18", "Kosmos", "Sakral", "Die Sterne sagen, heute verlierst du. Mal sehen!", "Die Sterne lagen falsch."),
    ("Rex", "CH_GYM21", "Bestie", "Draco", "GRAAH! Das Rudel will den Alpha herausfordern!", "Du bleibst der Alpha."),
    ("General Brakk", "CH_ELITE3", "Draco", "Dämon", "Helix ist Geschichte. Aber ein General gibt nie auf!", "Rückzug... geordnet."),
    ("Krell", "CH_ELITE0", "Arsenal", "Nano", "Ich habe meine Waffen neu kalibriert. Nur für dich.", "Kalibrierung fehlgeschlagen."),
    ("Titeljägerin Nyx", "CH_PUNK", "Void", "Psi", "Ich jage Titel. Und deiner glänzt am hellsten.", "Die Jagd geht weiter."),
    ("Ass-Pilot Rook", "CH_RUNNER", "Aero", "Arsenal", "Tieffluganflug! Champion im Visier!", "Notlandung..."),
    ("Ritter Gideon", "CH_ELDER", "Metall", "Sakral", "Ein Ritter fordert den Champion zum Duell!", "Meine Rüstung hat Dellen. Respekt."),
    ("Hackerin Pixel", "CH_WOMAN", "Cyber", "Virus", "Ich hab deinen Titel schon gehackt. Jetzt nur noch dich!", "Error 404: Sieg nicht gefunden."),
    ("Sturmjäger Kaito", "CH_RUNNER", "Volt", "Aero", "Wo ein Sturm ist, bin ich. Und wo du bist, auch!", "Der Sturm legt sich."),
    ("Taucherin Ember", "CH_WOMAN", "Plasma", "Gravo", "Ich tauche in Lava. Ein Champion macht mir keine Angst!", "Abgekühlt..."),
    ("Waldläufer Moss", "CH_KID", "Bio", "Bestie", "Ich bin aus dem Dschungel gekommen, nur um dich zu schlagen!", "Zurück in den Dschungel..."),
    ("Professor Quill", "CH_SCIENTIST", "Nano", "Psi", "Eine wissenschaftliche Frage: Wie stark ist ein Champion?", "Hypothese widerlegt."),
    ("Jäger Vance", "CH_GRUNT", "Arsenal", "Toxin", "Auf deinen Kopf ist kein Preis ausgesetzt. Noch nicht.", "Kein Kopfgeld heute."),
    ("Kapitänin Coral", "CH_WOMAN", "Hydro", "Kristall", "Ahoi, Champion! Mein Schiff braucht eine Trophäe!", "Mann über Bord!"),
    ("Seherin Lumi", "CH_KID", "Kosmos", "Sakral", "Ich habe gesehen, dass wir heute kämpfen.", "Das Ende habe ich nicht gesehen."),
    ("Schrottkönig Bolt", "CH_ELDER", "Metall", "Nano", "Aus Schrott wird Gold. Und aus dir mein Sieg!", "Zurück auf den Schrottplatz."),
    ("Jägerin Raven", "CH_PUNK", "Dämon", "Sakral", "Ich jage Dämonen. Heute jage ich dich.", "Du bist kein Dämon. Du bist schlimmer."),
    ("Alchemist Ivo", "CH_SCIENTIST", "Toxin", "Virus", "Mein neuestes Gift wartet auf einen Champion!", "Gegengift... wirkt."),
    ("Läufer Atlas", "CH_GRUNT", "Gravo", "Bestie", "Ich trage die Welt. Dich trage ich mit links!", "Zu schwer..."),
    ("Neon-Idol Kiki", "CH_WOMAN", "Cyber", "Volt", "Meine Fans wollen ein Duell mit dem Champion sehen!", "Die Show war trotzdem super!"),
    ("Mönch Shen", "CH_ELDER", "Psi", "Kristall", "Ruhe ist Stärke. Lass uns meditieren. Mit Fäusten.", "Mein Geist ist klar. Du bist stärker."),
]
TIDX = {n: i for i, n in enumerate(GM.TYPES)}


def challenger_lines(c, t1, t2):
    pool = [L for L in GM.lines if 'legend' not in L and L['idx'] not in (0, 10, 20) and L['t'] in (TIDX[t1], TIDX[t2])]
    pool.sort(key=lambda L: (-L['nst'], L['idx']))
    r = random.Random(500 + c)
    big = pool[:max(6, len(pool) // 2)]
    pick = r.sample(big, 6) if len(big) >= 6 else (big * 6)[:6]
    out = [L['idx'] for L in pick]
    if c == 0:
        out[5] = 0xFFFF    # Kai: his starter line as ace
    return out


def write_post():
    with open(os.path.join(os.path.dirname(OUT), "gen_champ.inc"), "w") as f:
        f.write("// generated by tools/gen_post.py\n")
        f.write("#define NUM_CHALLENGERS %d\n" % len(CHALLENGERS))
        f.write("static const ChallengerData challengers[NUM_CHALLENGERS] = {\n")
        for c, (name, chr_, t1, t2, intro, lose) in enumerate(CHALLENGERS):
            ls = challenger_lines(c, t1, t2)
            f.write('    { %s, %s, %d, %d, { %s }, %s, %s },\n' % (cq(name), chr_, TIDX[t1], TIDX[t2], ", ".join(map(str, ls)), cq(intro), cq(lose)))
        f.write("};\n")
        f.write("static const u8 legend_spot[NUM_LEGENDS][3] = {\n")
        for (mp, x, y) in LEGEND_SPOTS:
            f.write("    { M_%s, %d, %d },\n" % (mp, x, y))
        f.write("};\n")
    print("challengers", len(CHALLENGERS), "legend spots", LEGEND_SPOTS)
