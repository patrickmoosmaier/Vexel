# -*- coding: utf-8 -*-
# Generates all VEXEL creature data: types, type chart, attributes, abilities, moves,
# evolution lines (tiers Bit/Byte/Kern/Champion/Ultra/Mega), learnsets,
# encounter zones, dex texts and compressed sprites.
import numpy as np, random, math, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mon_art import render, S

OUT = sys.argv[1] if len(sys.argv) > 1 else "source/gen_mon.c"
PREVIEW = os.environ.get("PREVIEW")

TYPES = ["Plasma", "Hydro", "Bio", "Volt", "Cyber", "Kristall", "Nano", "Gravo", "Psi", "Toxin",
         "Metall", "Void", "Draco", "Sakral", "Dämon", "Bestie", "Aero", "Arsenal", "Virus", "Kosmos"]
NT = len(TYPES)
(PL, HY, BI, VO, CY, KR, NA, GR, PS, TO, ME, VD, DR, SA, DA, BE, AE, AR, VI, KO) = range(NT)
TIER_NAMES = ["Bit", "Byte", "Kern", "Champion", "Ultra", "Mega"]
ATTRS = ["Vakzin", "Daten", "Virus", "Frei"]

# ------------------------------------------------------------------ type chart
STRONG = {
    PL: [BI, NA, KR, ME, BE], HY: [PL, GR, KR], BI: [HY, GR, KR], VO: [HY, ME, CY, AE, AR], CY: [ME, PS, VI],
    KR: [PL, NA, VO, DR, AE], NA: [BI, PS, CY], GR: [VO, ME, KR, PL, AR], PS: [GR, TO, BE], TO: [BI, CY, BE],
    ME: [KR, VD, SA], VD: [PS, CY, KO], DR: [DR, BE, AE], SA: [DA, VI, VD, TO], DA: [SA, PS, BE],
    BE: [BI, NA, PS], AE: [BI, NA, BE], AR: [ME, BE, AE, KR], VI: [CY, NA, AR], KO: [SA, DA, PS, GR],
}
WEAK = {  # attacker -> defenders that resist
    PL: [HY, PL, GR, DR], HY: [HY, BI, DR], BI: [PL, BI, TO, NA, ME, AE], VO: [VO, BI, GR], CY: [VD, KR],
    KR: [GR, ME, HY], NA: [PL, ME, KR], GR: [BI, PS, AE], PS: [ME, PS, DA], TO: [TO, ME, GR, KR, SA],
    ME: [PL, HY, VO, ME], VD: [VD, NA, SA], DR: [ME, SA], SA: [SA, ME, AR, KO], DA: [DA, VD, KO],
    BE: [ME, GR, DR, AR], AE: [VO, KR, ME], AR: [GR, SA, VD], VI: [SA, VI, TO], KO: [KO, VD, DR],
}
EFF = [[2] * NT for _ in range(NT)]
for a, ds in STRONG.items():
    for d in ds:
        EFF[a][d] = 4
for a, ds in WEAK.items():
    for d in ds:
        EFF[a][d] = 1

# ------------------------------------------------------------------ abilities
EFFECTS = ["none", "burn", "para", "poison", "atk+", "def+", "spd+", "tec+", "fatk-", "fdef-", "fspd-", "heal", "drain",
           "recoil", "fast", "multi", "crit",
           # v5: classic status and battle effects
           "sleep", "freeze", "confuse", "flinch", "toxic", "atk+2", "def+2", "tec+2", "fatk-2", "fdef-2", "ftec-", "selftec-2",
           "selfdef-", "protect", "rest", "haze", "sun", "rain", "sand", "hail", "recharge", "fixed", "seed", "trap", "critup",
           "dance", "calm", "bulk", "charge", "dream", "hex", "sure", "wball", "cure", "half"]
EFF_C = {"none": "NONE", "burn": "BURN", "para": "PARA", "poison": "POISON", "atk+": "ATKUP", "def+": "DEFUP", "spd+": "SPDUP",
         "tec+": "TECUP", "fatk-": "FATKDN", "fdef-": "FDEFDN", "fspd-": "FSPDDN", "heal": "HEAL", "drain": "DRAIN", "recoil": "RECOIL",
         "fast": "FAST", "multi": "MULTI", "crit": "CRIT", "atk+2": "ATKUP2", "def+2": "DEFUP2", "tec+2": "TECUP2", "fatk-2": "FATKDN2",
         "fdef-2": "FDEFDN2", "ftec-": "FTECDN", "selftec-2": "SELFTECDN", "selfdef-": "SELFDEFDN"}
ABIL = []  # (name, desc, kind, param)
GENERIC_ABIL = [
    ("Panzerplatten", "Physischer Schaden -20%.", "phys_armor"), ("Energieschild", "Spezieller Schaden -20%.", "spec_shield"),
    ("Waffenmeister", "Physische Attacken +20%.", "weapon"), ("Hochtechnik", "Spezielle Attacken +20%.", "tech"),
    ("Zieloptik", "Doppelte Volltreffer-Chance.", "crit"), ("Turbo", "Initiative +25%.", "speed"),
    ("Selbstreparatur", "Heilt 1/16 KP pro Runde.", "regen"), ("Überladung", "Unter 1/3 KP: Schaden +50%.", "overload"),
    ("Reinheit", "Typ-Bonus 1,75 statt 1,5.", "stab"), ("Stachelpanzer", "Nahkämpfer erleiden 1/8 KP.", "thorns"),
    ("Antivirus", "Immun gegen Statusprobleme.", "immune"), ("Kampfrausch", "Bei Treffer: 30% Angriff +1.", "rage"),
    ("Lebensraub", "Heilt 1/8 des Schadens.", "leech"), ("Einschüchterung", "Senkt beim Erscheinen den gegn. Angriff.", "intimidate"),
    ("Präzision", "Genauigkeit +20%.", "accuracy"), ("Erstschlag", "Bei vollen KP greift es zuerst an.", "firststrike"),
    ("Phasenschritt", "10% Chance auszuweichen.", "evasive"), ("Bollwerk", "Übersteht einen K.O. bei vollen KP.", "sturdy"),
    ("Berserker", "Schaden +25%, erleidet aber +10%.", "berserk"), ("Sanitäter", "Heilt Status mit 33% pro Runde.", "medic"),
]
ABIL_KINDS = ["phys_armor", "spec_shield", "weapon", "tech", "crit", "speed", "regen", "overload", "stab", "thorns", "immune",
              "rage", "leech", "intimidate", "accuracy", "firststrike", "evasive", "sturdy", "berserk", "medic", "typeboost", "typeresist",
              "sun", "rain", "sand", "hail", "nosleep", "nofreeze", "noconfuse", "noflinch", "swim", "chloro", "sandveil", "icebody",
              "earlybird", "guts", "serene", "shielddust", "pickup"]
for (n, d, k) in GENERIC_ABIL:
    ABIL.append((n, d, ABIL_KINDS.index(k), 0))
TSTEM = ["Plasma", "Hydro", "Bio", "Volt", "Cyber", "Kristall", "Nano", "Gravo", "Psi", "Toxin", "Metall", "Void", "Drachen",
         "Sakral", "Dämonen", "Bestien", "Aero", "Arsenal", "Virus", "Kosmos"]
for t in range(NT):
    ABIL.append((TSTEM[t] + "kern", "%s-Attacken +30%%." % TYPES[t], ABIL_KINDS.index("typeboost"), t))
for t in range(NT):
    ABIL.append((TSTEM[t] + "filter", "Halbiert %s-Schaden." % TYPES[t], ABIL_KINDS.index("typeresist"), t))
NEW_ABIL = [  # v5, appended so older ids stay the same
    ("Glutsonne", "Ruft beim Erscheinen Plasmasonne herbei.", "sun"), ("Regenmacher", "Ruft beim Erscheinen Datenregen herbei.", "rain"),
    ("Glitchaura", "Ruft beim Erscheinen einen Glitchsturm herbei.", "sand"), ("Frostaura", "Ruft beim Erscheinen Kristallhagel herbei.", "hail"),
    ("Schlaflos", "Kann nicht einschlafen.", "nosleep"), ("Thermokern", "Kann nicht einfrieren.", "nofreeze"),
    ("Eigensinn", "Kann nicht verwirrt werden.", "noconfuse"), ("Fokuskern", "Schreckt nie zurück.", "noflinch"),
    ("Wellenreiter", "Bei Datenregen doppelte Initiative.", "swim"), ("Sonnenflügel", "Bei Plasmasonne doppelte Initiative.", "chloro"),
    ("Glitchtarnung", "Im Glitchsturm schwer zu treffen, kein Sturmschaden.", "sandveil"),
    ("Eiskörper", "Heilt bei Kristallhagel 1/16 KP pro Runde.", "icebody"),
    ("Frühaufsteher", "Wacht doppelt so schnell auf.", "earlybird"), ("Wutpegel", "Mit Statusproblem Angriff +50%.", "guts"),
    ("Glücksfaktor", "Zusatzeffekte doppelt so wahrscheinlich.", "serene"), ("Staubschild", "Immun gegen Zusatzeffekte.", "shielddust"),
    ("Sammler", "Findet nach Kämpfen manchmal Items.", "pickup"),
]
for (n, d, k) in NEW_ABIL:
    ABIL.append((n, d, ABIL_KINDS.index(k), 0))
abil_id = {a[0]: i for i, a in enumerate(ABIL)}

# ------------------------------------------------------------------ moves (20 types x 26)
STEMS = {
    PL: ["Plasma", "Glut", "Flammen", "Sonnen", "Hitze", "Magma", "Feuer"], HY: ["Hydro", "Wasser", "Gischt", "Strudel", "Tiefsee", "Flut", "Aqua"],
    BI: ["Ranken", "Blatt", "Sporen", "Wurzel", "Dornen", "Gaia", "Blüten"], VO: ["Volt", "Blitz", "Funken", "Ionen", "Tesla", "Donner", "Strom"],
    CY: ["Daten", "Pixel", "Code", "Bit", "Hyper", "Quanten", "Netz"], KR: ["Kristall", "Frost", "Prisma", "Quarz", "Eis", "Diamant", "Spiegel"],
    NA: ["Nano", "Schwarm", "Mikro", "Chitin", "Zellen", "Replika", "Insekten"], GR: ["Gravo", "Erd", "Stein", "Beben", "Fels", "Masse", "Tekto"],
    PS: ["Psi", "Gedanken", "Traum", "Astral", "Mental", "Seelen", "Orakel"], TO: ["Gift", "Säure", "Smog", "Toxin", "Miasma", "Seuchen", "Gall"],
    ME: ["Stahl", "Eisen", "Chrom", "Titan", "Bolzen", "Magnet", "Adamant"], VD: ["Schatten", "Null", "Leeren", "Phantom", "Abgrund", "Echo", "Dunkel"],
    DR: ["Drachen", "Wyrm", "Schuppen", "Drako", "Lindwurm", "Urzeit", "Feuerwyrm"], SA: ["Heilig", "Licht", "Seraph", "Engels", "Gloria", "Halo", "Himmels"],
    DA: ["Dämonen", "Höllen", "Fluch", "Infernal", "Teufels", "Sünden", "Abaddon"], BE: ["Bestien", "Krallen", "Raub", "Wolfs", "Wild", "Fang", "Ur"],
    AE: ["Wind", "Sturm", "Zyklon", "Aero", "Jet", "Böen", "Himmel"], AR: ["Kanonen", "Raketen", "Granat", "Laser", "Gatling", "Mörser", "Artillerie"],
    VI: ["Virus", "Trojaner", "Malware", "Glitch", "Hack", "Wurm", "Exploit"], KO: ["Stern", "Komet", "Galaxie", "Nebel", "Meteor", "Pulsar", "Kosmo"],
}
NOUNS = [  # (noun, phys)
    [("stoß", 1), ("biss", 1), ("hieb", 1), ("schlag", 1), ("kratzer", 1), ("spritzer", 0), ("funke", 0), ("puls", 0), ("pfeil", 0), ("tritt", 1)],
    [("klinge", 1), ("kralle", 1), ("welle", 0), ("strahl", 0), ("salve", 0), ("faust", 1), ("wirbel", 0), ("ramme", 1), ("bombe", 0), ("lanze", 1)],
    [("sturm", 0), ("kanone", 0), ("rakete", 0), ("bohrer", 1), ("spirale", 0), ("axt", 1), ("barrage", 0), ("schwert", 1), ("laser", 0), ("flut", 0)],
    [("-Nova", 0), ("meteor", 1), ("inferno", 0), ("-Apokalypse", 0), ("gigakanone", 0), ("-Railgun", 0), ("-Omega", 0), ("titanenhieb", 1), ("urknall", 0), ("zerfall", 0)],
]
POWERS = [[25, 30, 35, 40, 45], [50, 55, 60, 65, 70], [75, 80, 85, 90, 95], [100, 110, 120, 135, 150]]
TYPE_SIDE = {PL: "burn", VO: "para", TO: "poison", KR: "fspd-", HY: "fatk-", BI: "drain", ME: "fdef-", VD: "fatk-", PS: "fdef-",
             NA: "fast", GR: "fspd-", CY: "fast", DR: "crit", SA: "drain", DA: "burn", BE: "crit", AE: "fast", AR: "multi", VI: "poison", KO: "para"}
STATUS_POOL = [("panzer", "def+"), ("aufladung", "tec+"), ("antrieb", "spd+"), ("kriegsruf", "atk+"), ("reparatur", "heal"),
               ("nebel", "fatk-"), ("korrosion", "fdef-"), ("fessel", "fspd-")]
INFLICT = {PL: ("brandmal", "burn"), VO: ("lähmfeld", "para"), TO: ("infektion", "poison"), VI: ("infektion", "poison"),
           DA: ("höllenmal", "burn"), PS: ("hypnose", "sleep"), KO: ("schwerelos", "confuse"), BI: ("sporenwolke", "poison")}

moves = []
move_id = {}
names_seen = set()


def addmove(name, t, power, acc, phys, eff, chance, pp):
    if name in names_seen:
        raise Exception("dup move " + name)
    names_seen.add(name)
    move_id[name] = len(moves)
    moves.append((name, t, power, acc, phys, EFFECTS.index(eff), chance, pp))


addmove("Datenstoß", CY, 35, 100, 1, "none", 0, 35)
TYPE_MOVES = {t: [] for t in range(NT)}
TYPE_STATUS = {t: [] for t in range(NT)}
mr = random.Random(4242)
for t in range(NT):
    k = 0
    for tier in range(4):
        nouns = list(NOUNS[tier])
        mr.shuffle(nouns)
        if t == AR:
            nouns.sort(key=lambda n: 0 if n[0] in ("salve", "kanone", "rakete", "barrage", "laser", "gigakanone", "-Railgun", "bombe", "pfeil") else 1)
        cnt = 0
        for (noun, ph) in nouns:
            if cnt >= 5:
                break
            name = None
            for tries in range(len(STEMS[t])):
                stem = STEMS[t][(k * 3 + tier + tries) % len(STEMS[t])]
                cand = stem + noun
                if cand not in names_seen and len(cand) <= 17:
                    name = cand
                    break
            if name is None:
                continue
            power = POWERS[tier][cnt]
            acc = 100 if power < 90 else (95 if power < 110 else (90 if power < 130 else 85))
            eff, ch = "none", 0
            if cnt in (1, 3):
                eff = TYPE_SIDE[t]
                ch = {"drain": 100, "fast": 100, "multi": 100, "crit": 100}.get(eff, 20 if tier < 3 else 30)
                if eff == "fast" and tier > 1:
                    eff, ch = "none", 0
            if noun in ("salve", "barrage"):
                eff, ch = "multi", 100
            if eff == "multi":
                power = max(15, power // 3)
            if tier == 3 and ph and cnt == 4:
                eff, ch = "recoil", 100
            pp = max(5, 35 - (tier * 5 + cnt * 2))
            addmove(name, t, power, acc, ph, eff, ch, pp)
            TYPE_MOVES[t].append(move_id[name])
            cnt += 1
            k += 1
    pool = list(STATUS_POOL)
    mr.shuffle(pool)
    chosen = ([INFLICT[t]] + pool[:5]) if t in INFLICT else pool[:6]
    for (noun, e) in chosen:
        name = None
        for tries in range(len(STEMS[t])):
            cand = STEMS[t][(mr.randrange(len(STEMS[t])) + tries) % len(STEMS[t])] + noun
            if cand not in names_seen:
                name = cand
                break
        acc = 90 if e in ("burn", "para", "poison") else (75 if e == "sleep" else 100)
        addmove(name, t, 0, acc, 0, e, 100, 15)
        TYPE_STATUS[t].append(move_id[name])
assert len(moves) > 500, len(moves)

# v5: classic attacks with the new effects (appended, old move ids stay the same)
NEW_MOVES = [  # name, type, power, acc, phys, effect, chance, pp, universal
    ("Schlummercode", PS, 0, 75, 0, "sleep", 100, 15, 0), ("Traumraub", PS, 100, 100, 0, "dream", 100, 15, 0),
    ("Wiegenlied", SA, 0, 70, 0, "sleep", 100, 15, 0), ("Sporenschlaf", BI, 0, 75, 0, "sleep", 100, 15, 0),
    ("Nebelhypnose", VD, 0, 70, 0, "sleep", 100, 10, 0), ("Sternenschlaf", KO, 0, 75, 0, "sleep", 100, 10, 0),
    ("Frostatem", KR, 40, 100, 0, "freeze", 30, 25, 0), ("Gletscherhieb", KR, 85, 95, 1, "freeze", 10, 15, 0),
    ("Blizzardkern", KR, 110, 70, 0, "freeze", 20, 5, 0), ("Frostlaser", KR, 90, 100, 0, "freeze", 10, 10, 0),
    ("Verwirrstrahl", PS, 0, 100, 0, "confuse", 100, 15, 0), ("Glitchwalzer", VI, 0, 100, 0, "confuse", 100, 15, 0),
    ("Mentalwoge", PS, 65, 100, 0, "confuse", 20, 20, 0), ("Wirbelböe", AE, 60, 100, 0, "confuse", 20, 20, 0),
    ("Hackpuls", VI, 70, 100, 0, "confuse", 20, 15, 0), ("Schockbiss", BE, 65, 100, 1, "flinch", 30, 15, 0),
    ("Schattenhieb", VD, 80, 100, 1, "flinch", 20, 15, 0), ("Böenklinge", AE, 75, 95, 0, "flinch", 30, 15, 0),
    ("Drachenstampfer", DR, 85, 100, 1, "flinch", 20, 10, 0), ("Toxinflut", TO, 0, 90, 0, "toxic", 100, 10, 0),
    ("Schwervirus", VI, 0, 85, 0, "toxic", 100, 10, 0), ("Toxinbiss", TO, 50, 100, 1, "toxic", 30, 15, 0),
    ("Schwertritual", ME, 0, 100, 0, "atk+2", 100, 20, 0), ("Eisenmauer", ME, 0, 100, 0, "def+2", 100, 15, 0),
    ("Gedankenfokus", PS, 0, 100, 0, "tec+2", 100, 20, 0), ("Drachenritual", DR, 0, 100, 0, "dance", 100, 20, 0),
    ("Innenschau", SA, 0, 100, 0, "calm", 100, 20, 0), ("Massenaufbau", BE, 0, 100, 0, "bulk", 100, 20, 0),
    ("Furchtgebrüll", DA, 0, 100, 0, "fatk-2", 100, 20, 0), ("Kreischsignal", VO, 0, 85, 0, "fdef-2", 100, 20, 0),
    ("Störsender", CY, 0, 100, 0, "ftec-", 100, 20, 0), ("Datenleck", CY, 55, 95, 0, "ftec-", 50, 15, 0),
    ("Säurespray", TO, 40, 100, 0, "fdef-2", 100, 20, 0), ("Fokusenergie", BE, 0, 100, 0, "critup", 100, 30, 1),
    ("Reset", CY, 0, 100, 0, "haze", 100, 30, 1), ("Überhitzung", PL, 130, 90, 0, "selftec-2", 100, 5, 0),
    ("Kometensturz", KO, 130, 90, 0, "selftec-2", 100, 5, 0), ("Nahkampfwut", BE, 120, 100, 1, "selfdef-", 100, 5, 0),
    ("Titanenprall", ME, 120, 100, 1, "selfdef-", 100, 5, 0), ("Firewall-Schild", CY, 0, 100, 0, "protect", 100, 10, 1),
    ("Abwehrkuppel", SA, 0, 100, 0, "protect", 100, 10, 0), ("Ruhemodus", PS, 0, 100, 0, "rest", 100, 10, 1),
    ("Systemcheck", CY, 0, 100, 0, "cure", 100, 15, 1), ("Sonnenprotokoll", PL, 0, 100, 0, "sun", 100, 5, 0),
    ("Regenprotokoll", HY, 0, 100, 0, "rain", 100, 5, 0), ("Staubgewitter", GR, 0, 100, 0, "sand", 100, 10, 0),
    ("Hagelruf", KR, 0, 100, 0, "hail", 100, 10, 0), ("Wetterball", AE, 50, 100, 0, "wball", 100, 10, 1),
    ("Sonnenlanze", BI, 120, 100, 0, "charge", 100, 10, 0), ("Himmelsangriff", AE, 140, 90, 1, "charge", 100, 5, 0),
    ("Overdrive-Strahl", CY, 150, 90, 0, "recharge", 100, 5, 1), ("Titanenramme", BE, 150, 90, 1, "recharge", 100, 5, 0),
    ("Donnerkeil", VO, 110, 70, 0, "para", 30, 10, 0), ("Nachtcode", VD, 1, 100, 0, "fixed", 100, 15, 0),
    ("Seismowurf", GR, 1, 100, 1, "fixed", 100, 20, 0), ("Halbierbiss", BE, 1, 90, 1, "half", 100, 10, 0),
    ("Saugsporen", BI, 0, 90, 0, "seed", 100, 10, 0), ("Parasitencode", VI, 0, 90, 0, "seed", 100, 10, 0),
    ("Fesselnetz", NA, 35, 85, 0, "trap", 100, 15, 0), ("Glutkäfig", PL, 35, 85, 0, "trap", 100, 15, 0),
    ("Strudelfalle", HY, 35, 85, 0, "trap", 100, 15, 0), ("Unheilsstrahl", DA, 65, 100, 0, "hex", 100, 10, 0),
    ("Plagegeist", VD, 65, 100, 0, "hex", 100, 10, 0), ("Aura-Kugel", SA, 80, 100, 0, "sure", 100, 20, 0),
    ("Zielsuchrakete", AR, 80, 100, 0, "sure", 100, 15, 0), ("Glutrad", PL, 60, 100, 1, "burn", 10, 25, 0),
    ("Alarmhieb", BE, 70, 100, 1, "hex", 100, 10, 0), ("Gravitonfalle", GR, 35, 85, 0, "trap", 100, 15, 0),
    ("Nanoschwarmbiss", NA, 60, 100, 1, "flinch", 30, 20, 0), ("Kristallwand", KR, 0, 100, 0, "def+2", 100, 15, 0),
]
NEW_BY_TYPE = {t: [] for t in range(NT)}
NEW_UNIVERSAL = []
for (n, t, pw, ac, ph, e, ch, pp, uni) in NEW_MOVES:
    addmove(n, t, pw, ac, ph, e, ch, pp)
    (NEW_UNIVERSAL if uni else NEW_BY_TYPE[t]).append(move_id[n])

# ------------------------------------------------------------------ evolution lines
PREFIX = {
    PL: ["Pyr", "Ign", "Flar", "Sol", "Glut", "Bren", "Lum", "Fuso", "Nov", "Kalor"],
    HY: ["Aqu", "Hydr", "Mar", "Nix", "Thal", "Wel", "Gisch", "Kora", "Tid", "Flu"],
    BI: ["Phyt", "Flor", "Spor", "Sylv", "Rank", "Myk", "Blat", "Vir", "Wurz", "Chlor"],
    VO: ["Volt", "Amp", "Zap", "Ion", "Blitz", "Ohm", "Tesl", "Ark", "Watt", "Spark"],
    CY: ["Byt", "Pix", "Kod", "Dat", "Bin", "Hex", "Neur", "Syn", "Cach", "Algo"],
    KR: ["Kris", "Quar", "Prism", "Lith", "Geod", "Frost", "Glaz", "Opal", "Kryo", "Silik"],
    NA: ["Nan", "Mikr", "Schwa", "Chit", "Mot", "Larv", "Kokk", "Fil", "Zell", "Mod"],
    GR: ["Grav", "Terr", "Mag", "Gol", "Tekt", "Seis", "Bold", "Krat", "Mass", "Dun"],
    PS: ["Psy", "Ment", "Ast", "Oni", "Tel", "Vis", "Aur", "Zen", "Mir", "Orak"],
    TO: ["Tox", "Ven", "Mias", "Smog", "Viru", "Sau", "Mold", "Pest", "Gall", "Nox"],
    ME: ["Ferr", "Chrom", "Titan", "Stahl", "Kobal", "Mech", "Bolz", "Nick", "Alu", "Iridi"],
    VD: ["Nul", "Umbr", "Void", "Schat", "Abys", "Nek", "Echo", "Phant", "Vex", "Obsk"],
    DR: ["Drak", "Wyrm", "Drako", "Lind", "Fafn", "Tiam", "Bahm", "Ryu", "Viver", "Sauri"],
    SA: ["Seraf", "Angl", "Sank", "Glori", "Lumin", "Celes", "Halo", "Paladi", "Aurel", "Cheru"],
    DA: ["Daem", "Infer", "Mephi", "Bael", "Abad", "Lilit", "Belze", "Asmo", "Morg", "Diab"],
    BE: ["Wolf", "Leon", "Ursa", "Tigr", "Lupa", "Best", "Fang", "Gryff", "Rakk", "Feral"],
    AE: ["Aero", "Zeph", "Ventu", "Cirr", "Strat", "Avi", "Jett", "Sky", "Wind", "Falk"],
    AR: ["Gun", "Kanon", "Mors", "Bazu", "Rak", "Gatl", "Arsen", "Snip", "Mort", "Panz"],
    VI: ["Troj", "Malw", "Spyw", "Bugg", "Hack", "Rootk", "Virax", "Krypt", "Phish", "Botn"],
    KO: ["Astro", "Stell", "Galax", "Nebul", "Quasa", "Pulsa", "Komet", "Orbit", "Lunar", "Cosm"],
}
SUF = [
    ["bit", "pip", "chen", "ling", "mi", "ko", "pu", "ni", "bo", "li", "pi", "tsu"],
    ["byte", "ix", "o", "et", "y", "zi", "lo", "ru", "ka", "mo", "ito", "ana"],
    ["tron", "ex", "ion", "ar", "ox", "ent", "ino", "oid", "ux", "orn", "ek", "an", "ago", "ian"],
    ["rax", "oros", "ador", "anth", "ius", "otor", "agon", "yte", "axis", "ulon", "akon", "orix", "erra", "ispa"],
    ["ator", "arch", "oth", "aris", "ekor", "ithan", "orak", "eron", "uros", "agar", "ilith", "amon", "ultra", "on"],
    ["omega", "apex", "zenit", "nexus", "ultim", "prime", "titan", "astra", "rex", "dread", "lord", "kaiser", "lex", "vant"],
]
MID = ["", "", "", "a", "e", "o", "u", "i", "ar", "el", "or", "en", "ax", "yr", "um"]
SECOND = {PL: [VO, GR, VD, DR, AR], HY: [KR, PS, VO, BI, DR], BI: [TO, NA, PS, BE, SA], VO: [ME, CY, PL, AR, AE], CY: [VO, PS, ME, VI, AR],
          KR: [HY, PS, ME, SA, DR], NA: [ME, TO, BI, CY, VI], GR: [ME, PL, KR, BE, DR], PS: [VD, CY, KR, SA, KO], TO: [VD, NA, HY, VI, DA],
          ME: [CY, GR, VO, AR, DR], VD: [PS, TO, PL, DA, KO], DR: [PL, AE, ME, DA, KO], SA: [AE, KO, PS, ME, PL], DA: [VD, PL, DR, VI, TO],
          BE: [BI, GR, DR, ME, AR], AE: [VO, SA, DR, KO, CY], AR: [ME, PL, VO, CY, GR], VI: [CY, DA, TO, VD, NA], KO: [PS, SA, VD, DR, AE]}
FINALS = {PL: ["dragon", "gunner", "cannonbeast", "knight"], HY: ["serpentdragon", "cannonbeast", "knight"], BI: ["cannonbeast", "knight", "insectwarrior", "angel"],
          VO: ["cannonbeast", "gunner", "mechbird", "titan"], CY: ["gunner", "titan", "knight"], KR: ["knight", "dragon", "angel"],
          NA: ["insectwarrior", "titan", "gunner"], GR: ["titan", "cannonbeast", "dragon"], PS: ["angel", "knight", "demon"],
          TO: ["demon", "serpentdragon", "insectwarrior"], ME: ["titan", "gunner", "knight"], VD: ["demon", "serpentdragon", "knight"],
          DR: ["dragon", "serpentdragon", "knight"], SA: ["angel", "knight"], DA: ["demon", "dragon"], BE: ["cannonbeast", "knight"],
          AE: ["mechbird", "angel", "dragon"], AR: ["gunner", "titan", "cannonbeast"], VI: ["demon", "insectwarrior", "gunner"],
          KO: ["angel", "titan", "dragon"]}
BASE_OF = {"knight": ["biped"], "gunner": ["biped", "mech"], "angel": ["biped", "avian"], "demon": ["biped"], "insectwarrior": ["insect"],
           "titan": ["mech"], "dragon": ["biped", "quad"], "cannonbeast": ["quad"], "serpentdragon": ["serpent", "fish"], "mechbird": ["avian"]}
WEAPON_OF = {"knight": ["sword", "lance", "sword"], "gunner": ["armcannon", "gatling"], "angel": ["lance"], "demon": ["claws"],
             "insectwarrior": ["sword"], "titan": ["gatling"], "dragon": ["claws"], "cannonbeast": ["backcannons"],
             "serpentdragon": ["backcannons"], "mechbird": ["armcannon"]}
GUNS = ("gunner", "titan", "cannonbeast", "serpentdragon", "mechbird")

names_used = set()


def mkname(root, mid, tier, rs):
    for _ in range(300):
        s = rs.choice(SUF[tier])
        n = root + mid + s
        for a, b in (("aa", "a"), ("oo", "o"), ("ii", "i"), ("uu", "u"), ("yy", "y")):
            n = n.replace(a, b)
        n = n[0].upper() + n[1:].lower()
        if 4 <= len(n) <= 11 and n not in names_used:
            names_used.add(n)
            return n
        mid = rs.choice(MID)
    raise Exception("name")


lines = []
species = []
order = [(t, i) for t in range(NT) for i in range(10)]
STARTERS = {0: (PL, "dragon", ["Glimmbit", "Glimmbyte", "Glutron", "Pyrorax", "Solarch", "Novastra"]),
            10: (HY, "cannonbeast", ["Aquibit", "Aquibyte", "Aquarion", "Thalassor", "Maritron", "Ozeanexus"]),
            20: (BI, "knight", ["Sprossbit", "Sprossbyte", "Rankentron", "Sylvarax", "Floranth", "Gaiaprime"])}
for v in STARTERS.values():
    names_used.update(v[2])
TOTALS = [165, 235, 315, 415, 515, 615]
CATCH = [220, 200, 160, 90, 35, 8]
def make_line(li, t, i, root):
    rs = random.Random(1000 + li)
    nst = [4, 4, 4, 4, 5, 5, 5, 5, 6, 6][(i * 7 + t * 3) % 10]
    if li in STARTERS:
        nst = 6
    if i >= 10 and nst == 4 and (t * 5 + i) % 4 == 0:   # v6 lines: a few get an extra stage
        nst = 5
    first_tier = 6 - nst
    mid = rs.choice(MID)
    t2 = rs.choice(SECOND[t]) if rs.random() < 0.6 else -1
    t2tier = rs.choice([3, 4])
    final = rs.choice(FINALS[t])
    if li in STARTERS:
        final = STARTERS[li][1]
    base = rs.choice(BASE_OF[final])
    weapon = rs.choice(WEAPON_OF[final])
    genes = dict(
        bw=rs.uniform(0.85, 1.2), bh=rs.uniform(0.85, 1.2), hr=rs.uniform(0.85, 1.15), limb=rs.uniform(0.8, 1.3),
        ears=rs.choice(["horns", "ears", "antenna", "crest", "fins", "none", "horns", "spikes"]),
        tail=rs.choice(["thin", "thick", "none", "multi", "thin"]),
        back=rs.choice(["spikes", "shell", "none", "spikes", "plates"]),
        eyes=rs.choice(["normal", "glow", "normal", "visor", "glow", "mono"]),
        pattern=rs.choice(["circuit", "stripes", "spots", "core", "none", "circuit", "core"]),
        belly=rs.random() < 0.6, mouth=rs.random() < 0.5,
        helm=rs.choice(["vfin", "horns", "crest", "plume", "vfin"]), cape=rs.random() < 0.45, wings=rs.random() < 0.4,
        halo=rs.random() < 0.3, energy=rs.random() < 0.5, weapon=weapon,
        offhand=rs.choice(["shield", "shield", "cannon", "none"]) if final != "gunner" else rs.choice(["cannon", "none"]),
    )
    if base == "mech":
        genes['eyes'] = 'visor'
    w = [rs.uniform(0.7, 1.4) for _ in range(5)]
    bias = {PL: [0, .2, 0, .2, .1], HY: [.2, 0, .1, .1, 0], BI: [.1, 0, .1, .2, 0], VO: [0, 0, 0, .2, .3], CY: [0, .1, 0, .2, .2],
            KR: [0, .1, .3, .1, -.1], NA: [-.1, .2, 0, 0, .3], GR: [.2, .3, .2, -.2, -.2], PS: [0, -.2, 0, .4, .2], TO: [.1, 0, .1, .2, 0],
            ME: [0, .2, .4, 0, -.2], VD: [0, .2, -.1, .2, .2], DR: [.15, .3, .1, .1, 0], SA: [.1, 0, .1, .3, .1], DA: [0, .3, 0, .2, .1],
            BE: [.1, .35, 0, -.2, .3], AE: [-.1, .1, -.1, .1, .45], AR: [0, .2, .2, .3, -.2], VI: [-.1, .1, -.1, .3, .3], KO: [.1, -.1, .1, .35, .1]}[t]
    w = [max(0.4, a + b) for a, b in zip(w, bias)]
    if final in ("titan", "knight"):
        w[2] += 0.15
    if final in GUNS:
        w[3] += 0.1
    s_ = sum(w)
    w = [x / s_ for x in w]
    evo_lv = {0: rs.randint(5, 7), 1: rs.randint(11, 14), 2: rs.randint(20, 26), 3: rs.randint(35, 42), 4: rs.randint(52, 60)}
    attr = {SA: 0, CY: 1, VI: 2, DA: 2}.get(t, rs.choice([0, 1, 2, 3, 1, 0, 2]))
    L = dict(idx=li, t=t, t2=t2, t2tier=t2tier, nst=nst, first_tier=first_tier, root=root, first=len(species), final=final, base=base,
             genes=genes, w=w, evo_lv=evo_lv, attr=attr)
    lines.append(L)
    for st in range(nst):
        tier = first_tier + st
        nm = STARTERS[li][2][tier] if li in STARTERS else mkname(root, mid, tier, rs)
        tot = TOTALS[tier] + rs.randint(-14, 14)
        stats = [max(10, int(round(tot * x))) for x in w]
        stats[0] += 6
        stats = [min(255, v) for v in stats]
        types = (t, t2 if (t2 >= 0 and tier >= t2tier) else t)
        if tier <= 1:
            ab = rs.choice(["Selbstreparatur", "Turbo", "Präzision", "Phasenschritt", "Sanitäter", "Antivirus"])
        elif tier <= 3:
            ab = rs.choice([TSTEM[t] + "kern", TSTEM[t] + "kern", TSTEM[rs.choice(list(WEAK.keys()))] + "filter", "Kampfrausch", "Stachelpanzer", "Lebensraub"])
        else:
            pool = {"knight": ["Waffenmeister", "Bollwerk", "Panzerplatten"], "gunner": ["Zieloptik", "Präzision", "Hochtechnik"],
                    "titan": ["Panzerplatten", "Bollwerk", "Waffenmeister"], "angel": ["Energieschild", "Reinheit", "Sanitäter"],
                    "demon": ["Kampfrausch", "Einschüchterung", "Lebensraub", "Berserker"], "dragon": ["Einschüchterung", "Überladung", "Berserker"],
                    "cannonbeast": ["Zieloptik", "Überladung", "Hochtechnik"], "serpentdragon": ["Reinheit", "Einschüchterung", "Selbstreparatur"],
                    "mechbird": ["Turbo", "Erstschlag", "Zieloptik"], "insectwarrior": ["Erstschlag", "Waffenmeister", "Kampfrausch"]}[final]
            ab = rs.choice(pool + [TSTEM[t] + "kern"])
        species.append(dict(name=nm, line=li, stage=st, tier=tier, types=types, stats=stats, catch=CATCH[tier], exp=tot // 4 + 10,
                            evo=(evo_lv[tier] if st < nst - 1 else 0), abil=abil_id[ab], attr=attr))


for li, (t, i) in enumerate(order):
    make_line(li, t, i, PREFIX[t][i])


def learnset(L):
    rs = random.Random(5000 + L['idx'])
    t = L['t']
    out = [(1, move_id["Datenstoß"])]
    tm = TYPE_MOVES[t]
    lvls = [1, 7, 10, 13, 16, 20, 24, 28, 33, 38, 43, 48, 53, 58, 64, 70, 77, 85]
    picks = sorted(rs.sample(range(len(tm)), 16))
    for k, idx in enumerate(picks):
        out.append((max(1, lvls[min(k + 1, len(lvls) - 1)] + rs.randint(-1, 1)), tm[idx]))
    st = TYPE_STATUS[t]
    for k, idx in enumerate(rs.sample(range(len(st)), 3)):
        out.append(([3, 15, 30][k] + rs.randint(0, 3), st[idx]))
    if L['t2'] >= 0:
        lv0 = L['evo_lv'].get(L['t2tier'] - 1, 30)
        t2m = TYPE_MOVES[L['t2']]
        for k, tier_idx in enumerate([7, 11, 15, 18]):
            out.append((lv0 + k * 8, t2m[min(tier_idx, len(t2m) - 1)]))
    if L['final'] in GUNS or L['genes']['weapon'] in ('armcannon', 'gatling', 'backcannons'):
        am = TYPE_MOVES[AR]
        lv0 = L['evo_lv'].get(3, 38)
        for k, idx in enumerate([6, 10, 14, 17]):
            out.append((lv0 + k * 7, am[idx]))
    elif L['final'] in ("knight", "insectwarrior", "demon"):
        mm = TYPE_MOVES[ME if L['final'] != "demon" else DA]
        lv0 = L['evo_lv'].get(3, 38)
        for k, idx in enumerate([7, 12, 16]):
            out.append((lv0 + k * 8, mm[idx]))
    # v5: new classic attacks (own random stream so older choices stay the same)
    r2 = random.Random(7000 + L['idx'])
    nt = NEW_BY_TYPE[t]
    for k, mv in enumerate(r2.sample(nt, min(len(nt), 3))):
        out.append(([12, 26, 42][k] + r2.randint(0, 4), mv))
    if L['t2'] >= 0 and NEW_BY_TYPE[L['t2']]:
        out.append((34 + r2.randint(0, 8), r2.choice(NEW_BY_TYPE[L['t2']])))
    for k, mv in enumerate(r2.sample(NEW_UNIVERSAL, 2)):
        lv = [22, 50][k] + r2.randint(0, 8)
        if moves[mv][2] >= 150 and lv < 55: lv = 55
        out.append((lv, mv))
    return sorted(set((min(100, a), b) for a, b in out))


# ------------------------------------------------------------------ legendary Urcodes (post-champion story)
LEGENDS = [  # name, t1, t2, final archetype, ability
    ("Voltaris", VO, AE, "mechbird", "Turbo"), ("Abyssor", HY, VD, "serpentdragon", "Einschüchterung"),
    ("Kristallon", KR, PS, "angel", "Energieschild"), ("Pyrokron", PL, DR, "dragon", "Überladung"),
    ("Gaiarch", BI, BE, "cannonbeast", "Selbstreparatur"), ("Nullox", VI, DA, "demon", "Lebensraub"),
    ("Astrael", KO, SA, "angel", "Reinheit"), ("Aetherion", CY, KO, "knight", "Bollwerk"),
]
LEGEND_DEX = [
    "Urcode des Sturms. Sein Flügelschlag erzeugt Gewitter über ganz Neo-Kyra.",
    "Urcode der Tiefe. Schläft unter dem Meer und frisst versunkene Datenbanken.",
    "Urcode der Erinnerung. In seinen Kristallen ist das alte Netz gespeichert.",
    "Urcode des Feuers. Sein Atem schmolz einst die Polkappen.",
    "Urcode des Lebens. Wo es geht, wächst neuer Wald aus Beton.",
    "Urcode der Zerstörung. Der erste Virus, der je Bewusstsein erlangte.",
    "Urcode der Sterne. Wacht im Orbit über die Erde.",
    "Der erste Code. Aus Aetherion entstand das AETHER-Netz – und alle Vexel.",
]
for k, (nm, t, t2, final, ab) in enumerate(LEGENDS):
    li = len(lines)
    rs = random.Random(9100 + k)
    genes = dict(bw=1.1, bh=1.1, hr=1.0, limb=1.2, ears="horns", tail="thick", back="spikes", eyes="glow", pattern="core",
                 belly=True, mouth=False, helm="vfin", cape=True, wings=True, halo=True, energy=True,
                 weapon=WEAPON_OF[final][0], offhand="shield" if final in ("knight", "angel") else "cannon", legend=True)
    L = dict(idx=li, t=t, t2=t2, t2tier=5, nst=1, first_tier=5, root=nm, first=len(species), final=final, base=final,
             genes=genes, w=[0.2] * 5, evo_lv={}, attr=3, legend=k)
    lines.append(L)
    names_used.add(nm)
    stats = [150, 150, 140, 155, 135] if k < 7 else [170, 160, 160, 170, 150]
    species.append(dict(name=nm, line=li, stage=0, tier=5, types=(t, t2), stats=stats, catch=25, exp=260, evo=0,
                        abil=abil_id[ab], attr=3, legend=k))
FIRST_LEGEND_LINE = len(lines) - len(LEGENDS)

# v6: 100 more lines (5 per type), appended after the legends so all older Vexel keep their numbers
NEW_PREFIX = {
    PL: ["Vulk", "Fax", "Ember", "Calid", "Infla"], HY: ["Undi", "Nere", "Lagu", "Pelag", "Rivu"],
    BI: ["Bota", "Lian", "Fung", "Moos", "Kelp"], VO: ["Kilo", "Gaus", "Kond", "Neon", "Plasm"],
    CY: ["Pars", "Proz", "Ram", "Shell", "Vekt"], KR: ["Beryl", "Topa", "Sapph", "Mika", "Rubi"],
    NA: ["Pico", "Nanit", "Ameb", "Kapsi", "Mito"], GR: ["Grani", "Basal", "Monol", "Krust", "Pond"],
    PS: ["Kogni", "Somn", "Medi", "Prophe", "Cere"], TO: ["Korro", "Phag", "Alka", "Sulf", "Blight"],
    ME: ["Wolfr", "Zink", "Stann", "Platin", "Bronz"], VD: ["Nihil", "Vanta", "Lethe", "Hollo", "Eklip"],
    DR: ["Wyv", "Smau", "Ladon", "Jorm", "Nidh"], SA: ["Orat", "Elys", "Sancti", "Vigil", "Benedi"],
    DA: ["Mamm", "Azaz", "Lucif", "Molo", "Behem"], BE: ["Panth", "Lynx", "Bisan", "Jagu", "Hyen"],
    AE: ["Nimb", "Kumul", "Alto", "Tornad", "Zirk"], AR: ["Kalib", "Flak", "Howit", "Musket", "Salvo"],
    VI: ["Worm", "Kernl", "Daemn", "Ransom", "Zomb"], KO: ["Sideri", "Zenit", "Aster", "Plane", "Gamma"],
}
for t in range(NT):
    for i in range(10, 15):
        make_line(len(lines), t, i, NEW_PREFIX[t][i - 10])

# v5: themed classic abilities for some lines (own random stream, all stages of a line share it)
THEME_ABIL = {PL: ["Glutsonne", "Thermokern", "Wutpegel"], HY: ["Regenmacher", "Wellenreiter", "Wellenreiter"],
              BI: ["Sonnenflügel", "Frühaufsteher", "Sammler"], VO: ["Fokuskern", "Wellenreiter"],
              CY: ["Eigensinn", "Glücksfaktor"], KR: ["Frostaura", "Eiskörper", "Eiskörper"], NA: ["Staubschild", "Sammler"],
              GR: ["Glitchaura", "Glitchtarnung"], PS: ["Schlaflos", "Eigensinn", "Glücksfaktor"], TO: ["Staubschild", "Wutpegel"],
              ME: ["Fokuskern", "Glitchtarnung"], VD: ["Schlaflos", "Glücksfaktor"], DR: ["Wutpegel", "Fokuskern"],
              SA: ["Glücksfaktor", "Schlaflos"], DA: ["Wutpegel", "Schlaflos"], BE: ["Frühaufsteher", "Wutpegel", "Sammler"],
              AE: ["Sonnenflügel", "Fokuskern"], AR: ["Fokuskern", "Glücksfaktor"], VI: ["Glitchaura", "Eigensinn"],
              KO: ["Glücksfaktor", "Eigensinn"]}
ar = random.Random(7777)
for L in [L for L in lines if 'legend' not in L]:
    if ar.random() < 0.3:
        ab = abil_id[ar.choice(THEME_ABIL[L['t']])]
        for sp in species:
            if sp['line'] == L['idx']:
                sp['abil'] = ab
for (nm, ab) in [("Abyssor", "Regenmacher"), ("Pyrokron", "Glutsonne"), ("Kristallon", "Frostaura"), ("Nullox", "Glitchaura")]:
    for sp in species:
        if sp['name'] == nm:
            sp['abil'] = abil_id[ab]


LEARN = [learnset(L) for L in lines]

# ------------------------------------------------------------------ zones
ZONES = [  # name, type prefs, base level min, max   (runtime adds badges*2)
    ("Kabelpfad", [CY, BI, NA, VO, HY, PL, BE], 2, 6),
    ("Neonpark", [VO, CY, PS, TO, NA, AE], 4, 8),
    ("Hydrokanal", [HY, BI, TO, NA, VO], 6, 10),
    ("Kristallhöhle", [KR, GR, ME, VD, PS], 8, 12),
    ("Glitchwüste", [GR, PL, VD, PS, ME, TO, VI], 9, 13),
    ("Orbitalgarten", list(range(NT)), 30, 38),
    ("Datenabgrund", list(range(NT)), 50, 65),
    ("Datenautobahn", [CY, ME, AR, VO, VI], 5, 9),
    ("Schrottpass", [ME, AR, GR, TO], 7, 11),
    ("Lavafeld", [PL, DR, GR, DA], 8, 12),
    ("Tiefseesteg", [HY, NA, KO, BI], 6, 10),
    ("Himmelstreppe", [AE, SA, KR, PS], 8, 12),
    ("Schattengasse", [DA, VI, VD, TO], 8, 12),
    ("Raumhafenstraße", [KO, PS, CY, AE], 9, 13),
    ("Dschungelpfad", [BI, BE, NA, DR], 7, 11),
]
zr = random.Random(99)
pool = [L for L in lines if L['idx'] not in STARTERS and 'legend' not in L]
zr.shuffle(pool)
zone_lines = [[] for _ in ZONES]
for L in pool:
    best = None
    for zi, (zn, prefs, a, b) in enumerate(ZONES):
        if len(zone_lines[zi]) >= 22:
            continue
        score = (prefs.index(L['t']) if L['t'] in prefs else 9) + len(zone_lines[zi]) * 0.3 + zr.random() * 2
        if best is None or score < best[0]:
            best = (score, zi)
    zone_lines[best[1]].append(L['idx'])
ZONES.append(("Aetherberg", [], 62, 75))
zone_lines.append(sorted(random.Random(15).sample([L['idx'] for L in pool], 22)))
enc = []
for zi, (zn, prefs, a, b) in enumerate(ZONES):
    lst = [(lines[li]['first'], a, b, 10) for li in zone_lines[zi]]
    if zi == 6:
        for li in STARTERS:
            lst.append((lines[li]['first'], a, b, 1))
    enc.append(lst)

# ------------------------------------------------------------------ dex text
DEX_A = {
    PL: ["Sein Kern brennt heißer als ein Reaktor.", "Speichert Sonnenwind in seinen Schuppen.", "Hinterlässt glühende Spuren aus Plasma.", "Entstand in den Kühltürmen von Neo-Kyra."],
    HY: ["Reinigt die Kanäle der Unterstadt.", "Kann Daten in Wassertropfen speichern.", "Lebt in den gefluteten Ruinen der Altstadt.", "Sein Körper besteht aus Kühlflüssigkeit."],
    BI: ["Wächst auf verlassenen Serverdächern.", "Filtert Smog und atmet Sauerstoff aus.", "Seine Blätter leuchten bei Nacht.", "Wurzelt in Glasfaserkabeln."],
    VO: ["Zapft heimlich das Stromnetz an.", "Lädt sich bei Gewittern vollständig auf.", "Funkt mit Artgenossen über Kurzwelle.", "Seine Nerven leiten Strom wie Supraleiter."],
    CY: ["Ein Fragment des alten AETHER-Netzes.", "Besteht aus selbstlernendem Code.", "Kann sich in jedes Terminal hacken.", "Taucht als Glitch auf Werbetafeln auf."],
    KR: ["Wächst aus reinem Silizium-Kristall.", "Bricht Licht in hundert Farben.", "Speichert Erinnerungen in Kristallgittern.", "Bleibt bei minus 200 Grad aktiv."],
    NA: ["Ein Schwarm aus Millionen Nanobots.", "Repariert sich selbst in Sekunden.", "Baut Nester aus Elektroschrott.", "Kommuniziert über Pheromon-Code."],
    GR: ["Verbiegt die Schwerkraft um sich herum.", "Grub die Tunnel unter dem Rost-Sektor.", "Wiegt mehr, als sein Körper aussieht.", "Ernährt sich von Erzadern."],
    PS: ["Liest Gedanken aus dem Datenstrom.", "Schwebt dank telekinetischer Felder.", "Träumt die Zukunft voraus.", "Sein drittes Auge sieht Funkwellen."],
    TO: ["Entstand aus ausgelaufenen Chemikalien.", "Zersetzt Metall mit Säure.", "Lebt in den Abwassern der Fabriken.", "Sein Gift lähmt selbst Drohnen."],
    ME: ["Ein Panzer aus Titanlegierung schützt es.", "Wurde einst als Wachdrohne gebaut.", "Schmiedet sich neue Teile aus Schrott.", "Magnetisch: zieht Schrauben an."],
    VD: ["Existiert zwischen zwei Datenpaketen.", "Taucht nur bei Stromausfällen auf.", "Frisst gelöschte Dateien.", "Sein Schatten bewegt sich von allein."],
    DR: ["Uralter Code, älter als das Netz selbst.", "Sein Brüllen lässt Firewalls zerbersten.", "Hortet Datenschätze in Höhlen.", "Seine Schuppen sind härter als Diamant."],
    SA: ["Ein Schutzprogramm des alten Netzes.", "Sein Licht löscht Viren aus.", "Erscheint, wenn Daten in Gefahr sind.", "Wacht über die Server-Kathedralen."],
    DA: ["Entstand aus verbotenem Code.", "Lockt Kuriere mit falschen Signalen.", "Ernährt sich von Angst und Datenmüll.", "Seine Flammen brennen schwarz."],
    BE: ["Jagt im Rudel durch die Unterstadt.", "Seine Instinkte schlagen jeden Prozessor.", "Markiert sein Revier mit Kratzspuren.", "Wild, stolz und unbezähmbar."],
    AE: ["Reitet auf den Abwinden der Türme.", "Erreicht Überschallgeschwindigkeit.", "Nistet auf Antennenmasten.", "Lenkt Stürme mit seinen Flügeln."],
    AR: ["Trägt mehr Waffen als eine Kampfdrohne.", "Wurde für Helix-Kriege programmiert.", "Lädt seine Kanonen mit Sonnenenergie.", "Trifft ein Ziel auf zehn Kilometer."],
    VI: ["Ein mutierter Computervirus.", "Infiziert Systeme in Millisekunden.", "Kopiert sich selbst in jedes Netz.", "Von Helix als Waffe gezüchtet."],
    KO: ["Fiel als Datenkomet aus dem Orbit.", "Trägt Sternenstaub in seinem Kern.", "Kommuniziert mit Satelliten.", "Seine Augen zeigen ferne Galaxien."],
}
DEX_B = ["Gilt als scheu, aber neugierig.", "Wird schnell zutraulich.", "Verteidigt sein Revier erbittert.", "Sehr selten in freier Wildbahn.",
         "Liebt Neonlicht.", "Folgt Menschen, die es gut behandeln.", "Wird von Helix gejagt.", "Schläft in Ladestationen.",
         "Kämpft nur, wenn es muss.", "Sein Ruf klingt wie Modemrauschen.", "Bildet Rudel mit Artgenossen.", "Seine Kraft ist kaum gezähmt.",
         "Legendär unter Kurieren.", "Verehrt als Wächter des Netzes."]
DEX_TIER = ["Ein frisch geschlüpfter Datenkern.", "Lernt gerade, sich zu bewegen.", None, None,
            "Seine Waffen sind voll einsatzbereit.", "Die höchste bekannte Evolutionsstufe."]
for sp in species:
    if 'legend' in sp:
        sp['dex'] = LEGEND_DEX[sp['legend']]
        sp['height'] = 7.0 + sp['legend']
        sp['weight'] = 900.0 + sp['legend'] * 77
        continue
    r = random.Random(sp['line'] * 31 + sp['stage'])
    a = r.choice(DEX_A[sp['types'][0]])
    b = DEX_TIER[sp['tier']] or DEX_B[(sp['line'] * 3 + sp['stage'] * 5) % len(DEX_B)]
    sp['dex'] = a + " " + b
    sp['height'] = round([0.2, 0.4, 0.8, 1.5, 2.6, 4.2][sp['tier']] * r.uniform(0.7, 1.3), 1)
    sp['weight'] = round([2, 6, 18, 60, 180, 420][sp['tier']] * r.uniform(0.6, 1.5), 1)


# ------------------------------------------------------------------ sprites + compression
def pack4(img):
    flat = img.flatten()
    return bytes(int(flat[i]) | (int(flat[i + 1]) << 4) for i in range(0, len(flat), 2))


def lz_compress(data):
    out = bytearray()
    i, n = 0, len(data)
    table = {}
    while i < n:
        flag_pos = len(out)
        out.append(0)
        flags = 0
        for bit in range(8):
            if i >= n:
                break
            best_len, best_off = 0, 0
            if i + 2 < n:
                for p in reversed(table.get(data[i:i + 3], [])[-48:]):
                    off = i - p
                    if off > 4096:
                        continue
                    l = 0
                    while l < 18 and i + l < n and data[p + l] == data[i + l]:
                        l += 1
                    if l > best_len:
                        best_len, best_off = l, off
                        if l == 18:
                            break
            if best_len >= 3:
                flags |= 1 << bit
                o = best_off - 1
                out.append(((best_len - 3) << 4) | (o >> 8))
                out.append(o & 0xFF)
                for k in range(best_len):
                    if i + k + 2 < n:
                        table.setdefault(data[i + k:i + k + 3], []).append(i + k)
                i += best_len
            else:
                out.append(data[i])
                if i + 2 < n:
                    table.setdefault(data[i:i + 3], []).append(i)
                i += 1
        out[flag_pos] = flags
    return bytes(out)


def lz_decompress(src, n):
    out = bytearray()
    i = 0
    while len(out) < n:
        flags = src[i]
        i += 1
        for bit in range(8):
            if len(out) >= n:
                break
            if flags & (1 << bit):
                l = (src[i] >> 4) + 3
                o = (((src[i] & 15) << 8) | src[i + 1]) + 1
                i += 2
                for k in range(l):
                    out.append(out[-o])
            else:
                out.append(src[i])
                i += 1
    return bytes(out)


def rgb15(c):
    r, g, b = [max(0, min(255, int(v))) for v in c]
    return 0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)


def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


if __name__ == "__main__":
    only_preview = os.environ.get("PREVIEW_ONLY")
    sel = os.environ.get("PREVIEW_LINES")
    sprites, pals = {}, {}
    todo = range(len(species))
    if only_preview and sel:
        todo = []
        for li in [int(x) for x in sel.split(",")]:
            L = lines[li]
            todo += list(range(L['first'], L['first'] + L['nst']))
    for i in todo:
        sp = species[i]
        sprites[i], pals[i] = render(lines[sp['line']], sp['stage'], sp['tier'])

    if PREVIEW:
        from PIL import Image
        idxs = []
        if sel:
            for li in [int(x) for x in sel.split(",")]:
                L = lines[li]
                idxs += [-1] * L['first_tier'] + list(range(L['first'], L['first'] + L['nst']))
            cols = 6
        else:
            idxs = list(range(min(len(species), int(os.environ.get("PREVIEW_N", "120")))))
            cols = 20
        rows = (len(idxs) + cols - 1) // cols
        sheet = Image.new("RGB", (cols * 66, rows * 66), (40, 40, 56))
        for k, i in enumerate(idxs):
            if i < 0 or i not in sprites:
                continue
            img, pal = sprites[i], pals[i]
            a = np.zeros((S, S, 3), np.uint8)
            for c in range(1, 16):
                a[img == c] = [int(v) for v in pal[c]]
            a[img == 0] = (40, 40, 56)
            sheet.paste(Image.fromarray(a), ((k % cols) * 66 + 1, (k // cols) * 66 + 1))
        if sel:
            sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
        sheet.save(PREVIEW)
    if only_preview:
        sys.exit(0)

    blobs = []
    for i in range(len(species)):
        raw = pack4(sprites[i])
        z = lz_compress(raw)
        assert lz_decompress(z, 2048) == raw
        blobs.append(z)
    print("sprite bytes raw", 2048 * len(blobs), "compressed", sum(len(b) for b in blobs))

    with open(OUT, "w", encoding="utf-8") as f:
        f.write('// generated by tools/gen_mon.py\n#include "gen_mon.h"\n')
        f.write("const char *const type_names[NUM_TYPES] = {%s};\n" % ",".join(cstr(t) for t in TYPES))
        f.write("const char *const tier_names[6] = {%s};\n" % ",".join(cstr(t) for t in TIER_NAMES))
        f.write("const char *const attr_names[4] = {%s};\n" % ",".join(cstr(t) for t in ATTRS))
        f.write("const unsigned char type_eff[NUM_TYPES][NUM_TYPES] = {%s};\n" % ",".join("{" + ",".join(str(v) for v in row) + "}" for row in EFF))
        f.write("const AbilityData abilities[%d] = {\n" % len(ABIL))
        for (n, d, k, p) in ABIL:
            f.write("{%s,%s,%d,%d},\n" % (cstr(n), cstr(d), k, p))
        f.write("};\n")
        f.write("const MoveData moves[%d] = {\n" % len(moves))
        for (n, t, p, a, ph, e, ch, pp) in moves:
            f.write("{%s,%d,%d,%d,%d,%d,%d,%d},\n" % (cstr(n), t, p, a, ph, e, ch, pp))
        f.write("};\n")
        f.write("const SpeciesData species[%d] = {\n" % len(species))
        for sp in species:
            st = sp['stats']
            f.write("{%s,%s,{%d,%d,%d,%d,%d},%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},\n" % (
                cstr(sp['name']), cstr(sp['dex']), st[0], st[1], st[2], st[3], st[4], sp['types'][0], sp['types'][1],
                sp['line'], sp['stage'], sp['evo'], sp['catch'], sp['exp'], int(sp['height'] * 10), int(sp['weight'] * 10),
                sp['tier'], sp['abil'], sp['attr']))
        f.write("};\n")
        f.write("const LineData mlines[%d] = {\n" % len(lines))
        allls = []
        for L in lines:
            f.write("{%d,%d,%d,%d,%d},\n" % (L['first'], L['nst'], len(allls), len(LEARN[L['idx']]), L['t']))
            allls += LEARN[L['idx']]
        f.write("};\n")
        f.write("const unsigned short learn_data[%d][2] = {%s};\n" % (len(allls), ",".join("{%d,%d}" % x for x in allls)))
        f.write("const int zone_count = %d;\n" % len(ZONES))
        f.write("const char *const zone_names[] = {%s};\n" % ",".join(cstr(z[0]) for z in ZONES))
        flat, zidx = [], []
        for lst in enc:
            zidx.append((len(flat), len(lst)))
            flat += lst
        f.write("const unsigned short enc_data[][4] = {%s};\n" % ",".join("{%d,%d,%d,%d}" % e for e in flat))
        f.write("const unsigned short zone_enc[][2] = {%s};\n" % ",".join("{%d,%d}" % z for z in zidx))
        f.write("const unsigned short mon_pal[%d][16] = {\n" % len(species))
        for i in range(len(species)):
            f.write("{" + ",".join(str(rgb15(c)) if k else "0" for k, c in enumerate(pals[i])) + "},\n")
        f.write("};\n")
        ofs = [0]
        for b in blobs:
            ofs.append(ofs[-1] + len(b))
        f.write("const unsigned int mon_lz_ofs[%d] = {%s};\n" % (len(ofs), ",".join(str(o) for o in ofs)))
        f.write("const unsigned char mon_lz[%d] = {\n" % ofs[-1])
        allb = b"".join(blobs)
        for i in range(0, len(allb), 64):
            f.write(",".join(str(x) for x in allb[i:i + 64]) + ",\n")
        f.write("};\n")

    with open(OUT.replace(".c", ".h"), "w", encoding="utf-8") as f:
        f.write("""#pragma once
// generated by tools/gen_mon.py
typedef struct { const char *name; unsigned char type, power, acc, phys, effect, chance, pp; } MoveData;
typedef struct { const char *name; const char *dex; unsigned char base[5]; unsigned char type1, type2; unsigned short line; unsigned char stage, evo, catchrate; unsigned short exp; unsigned short height, weight; unsigned char tier, ability, attr; } SpeciesData;
typedef struct { unsigned short first; unsigned char nstages; unsigned short learn_ofs; unsigned char learn_n; unsigned char type; } LineData;
typedef struct { const char *name; const char *desc; unsigned char kind, param; } AbilityData;
#define NUM_SPECIES %d
#define NUM_MOVES %d
#define NUM_LINES %d
#define NUM_TYPES %d
#define NUM_ABILITIES %d
#define NO_MOVE 0xFFFF\n#define NUM_LEGENDS %d\n#define FIRST_LEGEND_SPECIES %d\n#define FIRST_LEGEND_LINE %d\n#define IS_LEGEND_LINE(l) ((l) >= FIRST_LEGEND_LINE && (l) < FIRST_LEGEND_LINE + NUM_LEGENDS)
extern const char *const type_names[NUM_TYPES];
extern const char *const tier_names[6];
extern const char *const attr_names[4];
extern const unsigned char type_eff[NUM_TYPES][NUM_TYPES];
extern const AbilityData abilities[];
extern const MoveData moves[];
extern const SpeciesData species[];
extern const LineData mlines[];
extern const unsigned short learn_data[][2];
extern const int zone_count;
extern const char *const zone_names[];
extern const unsigned short enc_data[][4];
extern const unsigned short zone_enc[][2];
extern const unsigned short mon_pal[][16];
extern const unsigned int mon_lz_ofs[];
extern const unsigned char mon_lz[];
enum { T_PLASMA, T_HYDRO, T_BIO, T_VOLT, T_CYBER, T_KRISTALL, T_NANO, T_GRAVO, T_PSI, T_TOXIN, T_METALL, T_VOID,
       T_DRACO, T_SAKRAL, T_DAEMON, T_BESTIE, T_AERO, T_ARSENAL, T_VIRUS, T_KOSMOS };
enum { %s, NUM_EFFECTS };
enum { %s };
""" % (len(species), len(moves), len(lines), NT, len(ABIL), len(LEGENDS), lines[FIRST_LEGEND_LINE]['first'], FIRST_LEGEND_LINE,
       ", ".join("E_" + EFF_C.get(e, e.upper()) for e in EFFECTS), ", ".join("AB_" + k.upper() for k in ABIL_KINDS)))

    print("species", len(species), "moves", len(moves), "lines", len(lines), "abilities", len(ABIL))
    for zi, lst in enumerate(enc):
        print(" ", ZONES[zi][0], len(lst))
