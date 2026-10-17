# Checks that every dialog page fits into the 4-line text box (simulated word wrap with the real font metrics).
import re, glob
src = open("source/gen_gfx.c").read()
cps = [int(x) for x in re.search(r"font_cp\[\] = \{([^}]*)\}", src).group(1).split(",")]
ws = [int(x) for x in re.search(r"font_w\[\] = \{([^}]*)\}", src).group(1).split(",")]
W = dict(zip(cps, ws))
def wrap_lines(s, width):
    cx, lines = 0, 1
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == '\n': cx = 0; lines += 1; i += 1; continue
        if ch == ' ':
            j = i + 1; ww = 0
            while j < len(s) and s[j] not in ' \n':
                ww += W.get(ord(s[j]), 0) + 1; j += 1
            if cx + 4 + ww > width: cx = 0; lines += 1; i += 1; continue
        cx += W.get(ord(ch), 0) + 1 if ord(ch) in W else 0
        if ord(ch) not in W and ch not in ' ': print("  missing glyph", repr(ch))
        i += 1
    return lines
bad = 0
strings = []
for f in glob.glob("source/*.c") + glob.glob("source/*.inc"):
    if "gen_" in f and not f.endswith(".inc"): continue
    s = open(f).read()
    for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', s):
        t = m.group(1).replace('\\"', '"')
        if len(t) > 40: strings.append((f, t))
for f, t in strings:
    for page in t.split('|'):
        n = wrap_lines(page, 232)
        if n > 4:
            bad += 1
            print(f"{f}: {n} lines: {page[:90]}...")
print("checked", len(strings), "strings, too long:", bad)
