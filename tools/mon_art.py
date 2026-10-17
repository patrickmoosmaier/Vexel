# -*- coding: utf-8 -*-
# Procedural pixel-art renderer for VEXEL creatures (64x64, 16 colours).
# Palette layout: 0 transparent, 1 outline, 2-5 primary ramp, 6-8 secondary ramp,
# 9-11 accent (glow) ramp, 12-14 metal ramp, 15 white.
import numpy as np, random, math, colorsys

S = 64
yy, xx = np.mgrid[0:S, 0:S].astype(np.float32) + 0.5
LIGHT = np.array([-0.55, -0.70, 0.55]); LIGHT /= np.linalg.norm(LIGHT)
M_PRI, M_SEC, M_ACC, M_DARK, M_BELLY, M_SHADOW, M_METAL = 0, 1, 2, 3, 4, 5, 6

# tiers: 0 Bit, 1 Byte, 2 Kern, 3 Champion, 4 Ultra, 5 Mega
TIER_SCALE = [0.34, 0.44, 0.56, 0.72, 0.88, 1.0]


class Canvas:
    def __init__(s):
        s.fill = np.zeros((S, S), bool)
        s.mat = np.zeros((S, S), np.int8)
        s.shade = np.zeros((S, S), np.float32)
        s.pid = np.zeros((S, S), np.int16)
        s.n = 0
        s.shadow = np.zeros((S, S), bool)

    def put(s, mask, mat, shade):
        s.n += 1
        if mat == M_SHADOW:
            s.shadow |= mask & ~s.fill
            return
        s.fill |= mask
        s.mat[mask] = mat
        s.shade[mask] = shade[mask] if isinstance(shade, np.ndarray) else shade
        s.pid[mask] = s.n


def sh_from_normal(nx, ny, nz):
    d = nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2]
    return np.clip(0.18 + 0.82 * d, 0, 1)


def _mx(x, m):
    return (S - x) if m else x


def ellipse(c, cx, cy, rx, ry, mat, rot=0.0, mirror=False, only_mirror=False):
    for m in ([True] if only_mirror else ([False, True] if mirror else [False])):
        x0 = _mx(cx, m)
        r = -rot if m else rot
        dx, dy = xx - x0, yy - cy
        cs, sn = math.cos(r), math.sin(r)
        u = (dx * cs + dy * sn) / max(rx, 0.6)
        v = (-dx * sn + dy * cs) / max(ry, 0.6)
        d = u * u + v * v
        mask = d <= 1.0
        nz = np.sqrt(np.clip(1 - d, 0, 1))
        nx = u * cs - v * sn
        ny = u * sn + v * cs
        c.put(mask, mat, sh_from_normal(nx, ny, nz))


def ring(c, cx, cy, rx, ry, th, mat, shade=0.9):
    e = ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2
    inner = ((xx - cx) / max(rx - th, 0.5)) ** 2 + ((yy - cy) / max(ry - th, 0.5)) ** 2
    c.put((e <= 1.0) & (inner > 1.0), mat, shade)


def capsule(c, x0, y0, x1, y1, r, mat, mirror=False, r1=None):
    if r1 is None:
        r1 = r
    for m in ([False, True] if mirror else [False]):
        ax, bx = _mx(x0, m), _mx(x1, m)
        vx, vy = bx - ax, y1 - y0
        L2 = vx * vx + vy * vy + 1e-6
        t = np.clip(((xx - ax) * vx + (yy - y0) * vy) / L2, 0, 1)
        px_, py_ = ax + t * vx, y0 + t * vy
        rr = r + (r1 - r) * t
        dx, dy = (xx - px_) / rr, (yy - py_) / rr
        d = dx * dx + dy * dy
        mask = d <= 1
        nz = np.sqrt(np.clip(1 - d, 0, 1))
        c.put(mask, mat, sh_from_normal(dx, dy, nz))


def poly(c, pts, mat, mirror=False, facet=True):
    for m in ([False, True] if mirror else [False]):
        P = [(_mx(x, m), y) for (x, y) in pts]
        mask = np.zeros((S, S), bool)
        n = len(P)
        for i in range(n):
            x1, y1 = P[i]
            x2, y2 = P[(i + 1) % n]
            if y1 == y2:
                continue
            cond = ((y1 <= yy) & (yy < y2)) | ((y2 <= yy) & (yy < y1))
            xint = x1 + (yy - y1) * (x2 - x1) / (y2 - y1)
            mask ^= cond & (xx < xint)
        cx = sum(p[0] for p in P) / n
        cy = sum(p[1] for p in P) / n
        far = max(P, key=lambda p: (p[0] - cx) ** 2 + (p[1] - cy) ** 2)
        ax, ay = far[0] - cx, far[1] - cy
        side = (xx - cx) * ay - (yy - cy) * ax
        if facet:
            light_side = 1 if (ay * LIGHT[0] - ax * LIGHT[1]) < 0 else -1
            shade = np.where(side * light_side > 0, 0.86, 0.42).astype(np.float32)
        else:
            shade = np.clip(0.62 - (xx - cx) * 0.03 - (yy - cy) * 0.03, 0.1, 1).astype(np.float32)
        c.put(mask, mat, shade)


def rectp(c, x0, y0, x1, y1, mat, mirror=False, bevel=1.5):
    for m in ([False, True] if mirror else [False]):
        a, b = ((S - x1, S - x0) if m else (x0, x1))
        mask = (xx >= a) & (xx <= b) & (yy >= y0) & (yy <= y1)
        dl, dr, dt, db = xx - a, b - xx, yy - y0, y1 - yy
        shade = np.full((S, S), 0.55, np.float32)
        shade = np.where(dt < bevel, 0.95, shade)
        shade = np.where(dl < bevel, 0.8, shade)
        shade = np.where(dr < bevel, 0.28, shade)
        shade = np.where(db < bevel, 0.2, shade)
        c.put(mask, mat, shade)


# ------------------------------------------------------------------ small elements
def flame(c, x, y, s, mat=M_ACC, mirror=False, ang=0.0):
    pts = []
    for k in range(12):
        a = k / 12 * 2 * math.pi
        r = s * (0.55 + 0.45 * (math.sin(a * 3) * 0.3 + 0.7)) if k else s * 0.4
        px_ = math.sin(a) * r * 0.65
        py_ = -math.cos(a) * r
        if py_ < 0:
            py_ *= 1.6
        pts.append((x + px_ * math.cos(ang) - py_ * math.sin(ang), y + px_ * math.sin(ang) + py_ * math.cos(ang)))
    poly(c, pts, mat, mirror, facet=False)


def shard(c, x, y, l, w, ang, mat=M_SEC, mirror=False):
    dx, dy = math.sin(ang), -math.cos(ang)
    px_, py_ = -dy, dx
    pts = [(x + px_ * w, y + py_ * w), (x + dx * l * 0.7 + px_ * w * 0.8, y + dy * l * 0.7 + py_ * w * 0.8), (x + dx * l, y + dy * l),
           (x + dx * l * 0.7 - px_ * w * 0.8, y + dy * l * 0.7 - py_ * w * 0.8), (x - px_ * w, y - py_ * w)]
    poly(c, pts, mat, mirror)


def spike(c, x, y, l, w, ang, mat=M_SEC, mirror=False):
    dx, dy = math.sin(ang), -math.cos(ang)
    px_, py_ = -dy, dx
    poly(c, [(x + px_ * w, y + py_ * w), (x + dx * l, y + dy * l), (x - px_ * w, y - py_ * w)], mat, mirror)


def bolt(c, x, y, s, mat=M_ACC, mirror=False):
    pts = [(x, y), (x + s * 0.5, y - s * 0.5), (x + s * 0.15, y - s * 0.55), (x + s * 0.7, y - s * 1.2), (x - s * 0.1, y - s * 0.45),
           (x + s * 0.25, y - s * 0.42), (x - s * 0.3, y + s * 0.1)]
    poly(c, pts, mat, mirror, facet=False)


def leaf(c, x, y, s, ang, mat=M_SEC, mirror=False):
    ellipse(c, x + math.sin(ang) * s * 0.6, y - math.cos(ang) * s * 0.6, s * 0.35, s * 0.75, mat, rot=ang, mirror=mirror)


# ------------------------------------------------------------------ weapons
def sword(c, hx, hy, ang, L, W, energy=False, mirror=False):
    """blade starting at hand (hx,hy) pointing along ang (0 = up)."""
    dx, dy = math.sin(ang), -math.cos(ang)
    px_, py_ = -dy, dx
    b0x, b0y = hx + dx * 2.5, hy + dy * 2.5
    tipx, tipy = hx + dx * L, hy + dy * L
    blade = [(b0x + px_ * W, b0y + py_ * W), (tipx - dx * W * 2 + px_ * W * 0.9, tipy - dy * W * 2 + py_ * W * 0.9), (tipx, tipy),
             (tipx - dx * W * 2 - px_ * W * 0.9, tipy - dy * W * 2 - py_ * W * 0.9), (b0x - px_ * W, b0y - py_ * W)]
    poly(c, blade, M_ACC if energy else M_METAL, mirror)
    # guard and hilt
    capsule(c, b0x + px_ * W * 2.2, b0y + py_ * W * 2.2, b0x - px_ * W * 2.2, b0y - py_ * W * 2.2, max(1.0, W * 0.55), M_SEC, mirror)
    capsule(c, hx - dx * 1.5, hy - dy * 1.5, hx - dx * 4, hy - dy * 4, max(0.9, W * 0.45), M_DARK, mirror)


def lance(c, hx, hy, ang, L, W, mirror=False):
    dx, dy = math.sin(ang), -math.cos(ang)
    capsule(c, hx - dx * L * 0.35, hy - dy * L * 0.35, hx + dx * L * 0.7, hy + dy * L * 0.7, max(0.8, W * 0.4), M_DARK, mirror)
    spike(c, hx + dx * L * 0.6, hy + dy * L * 0.6, L * 0.45, W * 1.2, ang, M_METAL, mirror)
    ellipse(c, hx + dx * L * 0.62, hy + dy * L * 0.62, W * 0.9, W * 0.9, M_ACC, mirror=mirror)


def cannon(c, x, y, ang, L, R, mirror=False, housing=True):
    dx, dy = math.sin(ang), -math.cos(ang)
    ex, ey = x + dx * L, y + dy * L
    if housing:
        ellipse(c, x + dx * L * 0.15, y + dy * L * 0.15, R * 1.7, R * 1.5, M_SEC, rot=ang, mirror=mirror)
    capsule(c, x, y, ex, ey, R, M_METAL, mirror)
    capsule(c, x + dx * L * 0.55, y + dy * L * 0.55, x + dx * L * 0.62, y + dy * L * 0.62, R * 1.25, M_DARK, mirror)
    ellipse(c, ex, ey, R * 1.05, R * 1.05, M_DARK, mirror=mirror)
    ellipse(c, ex, ey, max(0.8, R * 0.55), max(0.8, R * 0.55), M_ACC, mirror=mirror)


def gatling(c, x, y, ang, L, R, mirror=False):
    dx, dy = math.sin(ang), -math.cos(ang)
    px_, py_ = -dy, dx
    ellipse(c, x, y, R * 2.2, R * 2.0, M_SEC, rot=ang, mirror=mirror)
    for k in (-1, 0, 1):
        ox, oy = px_ * k * R * 0.9, py_ * k * R * 0.9
        capsule(c, x + ox, y + oy, x + ox + dx * L, y + oy + dy * L, max(0.7, R * 0.45), M_METAL, mirror)
    capsule(c, x + dx * L * 0.7 - px_ * R * 1.4, y + dy * L * 0.7 - py_ * R * 1.4, x + dx * L * 0.7 + px_ * R * 1.4, y + dy * L * 0.7 + py_ * R * 1.4,
            max(0.8, R * 0.5), M_DARK, mirror)


def shield(c, x, y, w, h, mirror=False):
    pts = [(x - w, y - h * 0.5), (x + w, y - h * 0.5), (x + w * 0.9, y + h * 0.15), (x, y + h * 0.6), (x - w * 0.9, y + h * 0.15)]
    poly(c, pts, M_METAL, mirror)
    s = 0.72
    pts2 = [(x - w * s, y - h * 0.38), (x + w * s, y - h * 0.38), (x + w * s * 0.9, y + h * 0.1), (x, y + h * 0.45), (x - w * s * 0.9, y + h * 0.1)]
    poly(c, pts2, M_SEC, mirror)
    poly(c, [(x, y - h * 0.25), (x + w * 0.25, y), (x, y + h * 0.22), (x - w * 0.25, y)], M_ACC, mirror)


def missile_pod(c, x, y, w, h, mirror=False):
    rectp(c, x - w / 2, y - h / 2, x + w / 2, y + h / 2, M_METAL, mirror)
    nx = 2 if w < 9 else 3
    for i in range(nx):
        for j in range(2):
            cx_ = x - w / 2 + (i + 0.5) * w / nx
            cy_ = y - h / 2 + (j + 0.5) * h / 2
            ellipse(c, cx_, cy_, max(0.9, w / nx * 0.32), max(0.9, h / 4 * 0.7), M_DARK, mirror=mirror)
            ellipse(c, cx_, cy_ - 0.3, max(0.6, w / nx * 0.18), max(0.6, h / 4 * 0.4), M_ACC, mirror=mirror)


def pauldron(c, x, y, r, spikes=0, mirror=False):
    ellipse(c, x, y, r * 1.15, r * 0.85, M_METAL, mirror=mirror)
    ellipse(c, x, y + r * 0.15, r * 0.8, r * 0.45, M_SEC, mirror=mirror)
    for k in range(spikes):
        spike(c, x + (k - (spikes - 1) / 2) * r * 0.6, y - r * 0.55, r * 0.9, r * 0.22, (k - (spikes - 1) / 2) * 0.5 - (0.3 if x < 32 else -0.3), M_METAL, mirror)


def bat_wing(c, sx, sy, span, height, mat=M_SEC, mirror=True):
    pts = [(sx, sy), (sx - span * 0.55, sy - height), (sx - span, sy - height * 0.75), (sx - span * 0.92, sy - height * 0.1),
           (sx - span * 0.75, sy + height * 0.25), (sx - span * 0.6, sy + height * 0.05), (sx - span * 0.45, sy + height * 0.35),
           (sx - span * 0.3, sy + height * 0.12), (sx - span * 0.12, sy + height * 0.4)]
    poly(c, pts, mat, mirror, facet=False)
    for k in range(2):
        capsule(c, sx, sy, sx - span * (0.55 + 0.4 * k), sy - height * (1.0 - 0.25 * k), 0.7, M_PRI, mirror)


def feather_wing(c, sx, sy, span, height, pairs=1, mat=M_BELLY, mirror=True):
    for p in range(pairs):
        sc = 1.0 - p * 0.22
        for k in range(5):
            a = 0.15 + k * 0.28 + p * 0.35          # upward angle from horizontal
            ln = span * sc * (1.0 - abs(k - 1.5) * 0.12)
            dx, dy = -math.cos(a), -math.sin(a)
            cx, cy = sx + dx * ln * 0.55, sy + p * height * 0.3 + dy * ln * 0.55
            ellipse(c, cx, cy, ln * 0.55, max(1.5, height * 0.17 * sc), mat, rot=math.atan2(dy, dx), mirror=mirror)


def mech_wing(c, sx, sy, span, height, mirror=True):
    pts = [(sx, sy - height * 0.2), (sx - span, sy - height), (sx - span * 1.05, sy - height * 0.7), (sx - span * 0.35, sy + height * 0.3)]
    poly(c, pts, M_METAL, mirror)
    pts2 = [(sx - span * 0.2, sy - height * 0.15), (sx - span * 0.9, sy - height * 0.85), (sx - span * 0.95, sy - height * 0.68),
            (sx - span * 0.35, sy + height * 0.1)]
    poly(c, pts2, M_SEC, mirror)
    capsule(c, sx - span * 0.35, sy - height * 0.3, sx - span * 0.85, sy - height * 0.75, 0.8, M_ACC, mirror)


TYPE_TIP = {}


def tip_element(c, t, x, y, s, ang=0.0, mirror=False):
    # 0 PL 1 HY 2 BI 3 VO 4 CY 5 KR 6 NA 7 GR 8 PS 9 TO 10 ME 11 VD 12 DR 13 SA 14 DA 15 BE 16 AE 17 AR 18 VI 19 KO
    if t in (0, 14):
        flame(c, x, y, s * 1.1, M_ACC, mirror, ang)
    elif t == 1:
        ellipse(c, x, y, s * 0.45, s * 0.6, M_SEC, mirror=mirror)
        spike(c, x, y - s * 0.3, s * 0.7, s * 0.3, ang, M_SEC, mirror)
    elif t in (2, 15):
        leaf(c, x, y, s, ang - 0.4, M_SEC, mirror)
        leaf(c, x, y, s * 0.8, ang + 0.5, M_SEC, mirror)
    elif t == 3:
        bolt(c, x, y, s * 1.2, M_ACC, mirror)
    elif t in (4, 18):
        rectp(c, x - s * 0.4, y - s * 0.4, x + s * 0.4, y + s * 0.4, M_ACC, mirror)
    elif t == 5:
        shard(c, x, y, s * 1.4, s * 0.35, ang, M_SEC, mirror)
    elif t == 6:
        ellipse(c, x, y, s * 0.35, s * 0.35, M_ACC, mirror=mirror)
    elif t == 7:
        ellipse(c, x, y, s * 0.6, s * 0.5, M_DARK, mirror=mirror)
    elif t in (8, 19, 13):
        ellipse(c, x, y, s * 0.45, s * 0.45, M_ACC, mirror=mirror)
    elif t == 9:
        ellipse(c, x, y, s * 0.4, s * 0.4, M_ACC, mirror=mirror)
        ellipse(c, x + s * 0.5, y - s * 0.6, s * 0.22, s * 0.22, M_ACC, mirror=mirror)
    elif t in (10, 17):
        spike(c, x, y, s * 1.3, s * 0.3, ang, M_METAL, mirror)
    elif t == 11:
        flame(c, x, y, s, M_ACC, mirror, ang + 0.3)
    elif t == 12:
        spike(c, x, y, s * 1.4, s * 0.4, ang, M_SEC, mirror)
    elif t == 16:
        spike(c, x, y, s * 1.2, s * 0.5, ang - 0.6, M_BELLY, mirror)
        spike(c, x, y, s * 1.0, s * 0.4, ang + 0.4, M_BELLY, mirror)


class Head:
    def __init__(s, x, y, r, kind='round'):
        s.x, s.y, s.r, s.kind = x, y, r, kind


# ------------------------------------------------------------------ helmet / heads
def helmet(c, h, g, tier):
    r = h.r
    style = g['helm']
    if style == 'vfin' or tier >= 5:
        spike(c, h.x - r * 0.15, h.y - r * 0.5, r * (1.2 + 0.3 * (tier >= 5)), r * 0.2, -0.75, M_ACC, mirror=True)
    if style == 'horns':
        spike(c, h.x - r * 0.55, h.y - r * 0.55, r * 1.3, r * 0.25, -0.5, M_METAL, mirror=True)
    if style == 'crest':
        spike(c, h.x, h.y - r * 0.7, r * 1.4, r * 0.3, 0.0, M_SEC)
    if style == 'plume':
        for k in range(3):
            spike(c, h.x + (k - 1) * r * 0.25, h.y - r * 0.7, r * (1.0 + 0.3 * (k == 1)), r * 0.18, (k - 1) * 0.35, M_ACC)
    ellipse(c, h.x, h.y, r * 1.0, r * 1.05, M_METAL)
    rectp(c, h.x - r * 0.95, h.y - r * 0.1, h.x + r * 0.95, h.y + r * 0.85, M_PRI)
    # face plate
    poly(c, [(h.x - r * 0.7, h.y + r * 0.15), (h.x + r * 0.7, h.y + r * 0.15), (h.x + r * 0.35, h.y + r * 0.95), (h.x - r * 0.35, h.y + r * 0.95)], M_METAL)


def eyes_list(h, style):
    return [(h, style)]


# ------------------------------------------------------------------ main construction
def build(L, stage, tier, seed):
    """Returns canvas, head list, body tuple, genes."""
    rl = random.Random(seed)
    t = L['t']
    t2 = L['t2'] if (L['t2'] >= 0 and tier >= L['t2tier']) else -1
    sc = TIER_SCALE[tier]
    g = dict(L['genes'])
    c = Canvas()
    U = 27 * sc
    ground = 61
    heads = []
    body = None

    if tier <= 1:
        # ---------------- baby blobs (Bit / Byte)
        r = U * (0.85 if tier == 0 else 0.78)
        cy = ground - r * 0.95
        ellipse(c, 32, ground - 0.5, r * 0.85, r * 0.2, M_SHADOW)
        if tier == 1:
            # tiny feet / ears / tail by gene
            ellipse(c, 32 - r * 0.45, ground - r * 0.15, r * 0.3, r * 0.2, M_DARK, mirror=True)
            if g['ears'] in ('ears', 'horns', 'antenna'):
                ellipse(c, 32 - r * 0.55, cy - r * 0.8, r * 0.25, r * 0.45, M_PRI, rot=-0.4, mirror=True)
            else:
                spike(c, 32, cy - r * 0.8, r * 0.7, r * 0.25, 0, M_SEC)
            tip_element(c, t, 32 + r * 0.95, cy + r * 0.1, r * 0.45, 1.2)
        else:
            if g['ears'] in ('antenna', 'crest'):
                capsule(c, 32, cy - r * 0.8, 32, cy - r * 1.35, 0.8, M_DARK)
                ellipse(c, 32, cy - r * 1.4, 1.6, 1.6, M_ACC)
        squash = 0.85 if tier == 0 else 0.95
        ellipse(c, 32, cy, r, r * squash, M_PRI)
        ellipse(c, 32, cy + r * 0.35, r * 0.6, r * 0.4, M_BELLY)
        h = Head(32, cy - r * 0.1, r * 0.75)
        heads.append(h)
        body = (32, cy, r, r)
        return c, heads, body, g, t2

    arch = L['base'] if tier <= 3 else L['final']
    champ = tier == 3
    mega = tier == 5
    young = 1.0 if tier == 2 else 0.4

    def back_feature(bx, by, bw_, bh_):
        k = g['back']
        grow = 0.5 + 0.25 * (tier - 2)
        if k == 'spikes':
            nsp = 2 + tier
            for i in range(nsp):
                a = -0.9 + 1.8 * (i + 0.5) / nsp
                x = bx + math.sin(a) * bw_ * 0.85
                y = by - math.cos(a) * bh_ * 0.85
                if t == 5:
                    shard(c, x, y, U * 0.45 * grow, U * 0.12, a * 1.2, M_SEC)
                elif t in (0, 14) and i % 2 == 0:
                    flame(c, x, y, U * 0.35 * grow, M_ACC, False, a)
                else:
                    spike(c, x, y, U * 0.42 * grow, U * 0.12, a * 1.2, M_SEC)
        elif k == 'shell':
            ellipse(c, bx, by - bh_ * 0.15, bw_ * 1.15, bh_ * 1.1, M_SEC)
        elif k == 'plates':
            for i in range(tier - 1):
                spike(c, bx - bw_ * (0.3 + i * 0.12), by - bh_ * 0.7 + i * 3, U * 0.38 * grow, U * 0.16, -0.5 - i * 0.25, M_METAL, mirror=True)

    def animal_head_features(h):
        k = g['ears']
        r = h.r
        if k == 'horns':
            ln = r * (0.7 + 0.4 * (tier - 2))
            if t == 5:
                shard(c, h.x - r * 0.5, h.y - r * 0.6, ln * 1.1, r * 0.2, -0.5, M_SEC, mirror=True)
            elif t in (0, 14):
                flame(c, h.x - r * 0.55, h.y - r * 0.6, ln * 0.7, M_ACC, True, -0.5)
            else:
                spike(c, h.x - r * 0.5, h.y - r * 0.65, ln, r * 0.22, -0.45, M_SEC, mirror=True)
        elif k == 'ears':
            ellipse(c, h.x - r * 0.7, h.y - r * 0.75, r * 0.33, r * 0.7, M_PRI, rot=-0.4, mirror=True)
            ellipse(c, h.x - r * 0.7, h.y - r * 0.7, r * 0.15, r * 0.45, M_SEC, rot=-0.4, mirror=True)
        elif k == 'antenna':
            capsule(c, h.x - r * 0.35, h.y - r * 0.7, h.x - r * 0.75, h.y - r * 1.6, 1.0, M_DARK, mirror=True)
            ellipse(c, h.x - r * 0.75, h.y - r * 1.6, 2.2, 2.2, M_ACC, mirror=True)
        elif k == 'crest':
            if t in (0, 11, 14):
                flame(c, h.x, h.y - r * 0.8, r * 0.9, M_ACC)
            elif t == 3:
                bolt(c, h.x - r * 0.2, h.y - r * 0.6, r * 1.1)
            else:
                spike(c, h.x, h.y - r * 0.6, r * 1.1, r * 0.3, 0, M_SEC)
        elif k == 'fins':
            spike(c, h.x - r * 0.8, h.y, r * 0.9, r * 0.3, -1.5, M_SEC, mirror=True)
        elif k == 'spikes':
            for i in range(2):
                spike(c, h.x - r * (0.2 + i * 0.3), h.y - r * 0.8, r * 0.8, r * 0.15, -0.2 - i * 0.4, M_SEC, mirror=True)

    def tail(xb, yb, length, thick):
        kind = g['tail']
        if kind == 'none':
            return
        n = 3 if kind != 'thick' else 2
        px_, py_ = xb, yb
        ang = -0.9
        for k in range(n):
            nx_ = px_ + math.cos(ang) * length / n * 1.4
            ny_ = py_ + math.sin(ang) * length / n * 1.4
            r0 = thick * (1 - k / (n + 1))
            r1 = thick * (1 - (k + 1) / (n + 1))
            capsule(c, px_, py_, nx_, ny_, max(1.2, r0), M_PRI, r1=max(1.0, r1))
            px_, py_ = nx_, ny_
            ang -= 0.45
        tip_element(c, t2 if t2 >= 0 else t, px_, py_, max(3, thick * 1.8), ang + 1.6)

    # ======================================================== ROOKIE / CHAMPION animal bodies
    if arch == 'biped':
        bw_ = U * 0.5 * g['bw']
        bh_ = U * 0.55 * g['bh']
        legh = U * 0.38
        bcy = ground - legh - bh_ * 0.85
        tail(32 + bw_ * 0.6, bcy + bh_ * 0.4, U * 0.9, U * 0.14 * g['limb'])
        if champ:
            back_feature(32, bcy, bw_, bh_)
        capsule(c, 32 - bw_ * 0.45, bcy + bh_ * 0.5, 32 - bw_ * 0.55, ground - 2, U * 0.14 * g['limb'], M_PRI, mirror=True)
        ellipse(c, 32 - bw_ * 0.6, ground - 2, U * 0.2, U * 0.1, M_DARK, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        body = (32, bcy, bw_, bh_)
        if g['belly']:
            ellipse(c, 32, bcy + bh_ * 0.25, bw_ * 0.62, bh_ * 0.62, M_BELLY)
        # arms (champion: weapon in hand)
        hx, hy = 32 - bw_ - U * 0.28, bcy + bh_ * 0.25
        capsule(c, 32 - bw_ * 0.8, bcy - bh_ * 0.3, hx, hy, U * 0.12 * g['limb'], M_PRI, mirror=True)
        if champ:
            w = g['weapon']
            if w in ('sword', 'lance', 'scythe'):
                sword(c, hx, hy, -0.45, U * 1.15, U * 0.08)
            elif w in ('armcannon', 'gatling', 'backcannons', 'pods'):
                cannon(c, hx + 1, hy - 2, -1.9, U * 0.55, U * 0.1)
            else:
                spike(c, hx, hy, U * 0.45, U * 0.07, -2.2, M_METAL)
                spike(c, _mx(hx, True), hy, U * 0.45, U * 0.07, 2.2, M_METAL)
            pauldron(c, 32 - bw_ * 0.8, bcy - bh_ * 0.45, U * 0.17, 0, mirror=True)
        ellipse(c, hx, hy, U * 0.13, U * 0.13, M_DARK, mirror=True)
        hr = U * 0.42 * g['hr'] * (1.0 + 0.35 * young)
        h = Head(32, bcy - bh_ - hr * 0.45, hr)
        animal_head_features(h)
        ellipse(c, 32, h.y, hr, hr * 0.92, M_PRI)
        if champ:
            rectp(c, 32 - hr * 0.9, h.y - hr * 0.95, 32 + hr * 0.9, h.y - hr * 0.45, M_METAL)
        heads.append(h)
    elif arch == 'quad':
        bw_ = U * 0.75 * g['bw']
        bh_ = U * 0.42 * g['bh']
        legh = U * 0.35
        bcy = ground - legh - bh_ * 0.7
        tail(32 + bw_ * 0.7, bcy - bh_ * 0.3, U * 0.8, U * 0.13 * g['limb'])
        if champ and g['weapon'] in ('backcannons', 'pods', 'gatling', 'armcannon'):
            cannon(c, 32 - bw_ * 0.4, bcy - bh_ * 0.6, -0.45, U * 0.7, U * 0.1, mirror=True)
        else:
            back_feature(32, bcy - bh_ * 0.2, bw_ * 0.8, bh_)
        capsule(c, 32 - bw_ * 0.75, bcy, 32 - bw_ * 0.8, ground - 3, U * 0.12 * g['limb'], M_DARK, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        body = (32, bcy, bw_, bh_)
        if champ:
            ellipse(c, 32, bcy - bh_ * 0.35, bw_ * 0.7, bh_ * 0.45, M_METAL)
        capsule(c, 32 - bw_ * 0.38, bcy + bh_ * 0.3, 32 - bw_ * 0.4, ground - 2, U * 0.14 * g['limb'], M_PRI, mirror=True)
        ellipse(c, 32 - bw_ * 0.4, ground - 2, U * 0.17, U * 0.09, M_DARK, mirror=True)
        hr = U * 0.4 * g['hr'] * (1.0 + 0.3 * young)
        h = Head(32, bcy - bh_ * 0.35 - hr * 0.3, hr)
        animal_head_features(h)
        ellipse(c, 32, h.y, hr * 1.05, hr * 0.9, M_PRI)
        ellipse(c, 32, h.y + hr * 0.45, hr * 0.5, hr * 0.35, M_BELLY)
        heads.append(h)
    elif arch == 'serpent':
        r0 = U * 0.6 * g['bw']
        back_feature(32, ground - r0 * 1.3, r0, r0 * 0.6)
        ellipse(c, 32, ground - r0 * 0.35, r0 * 1.2, r0 * 0.38, M_PRI)
        ellipse(c, 32 + r0 * 0.15, ground - r0 * 0.85, r0 * 0.9, r0 * 0.33, M_PRI)
        if g['belly']:
            ellipse(c, 32, ground - r0 * 0.3, r0 * 0.8, r0 * 0.2, M_BELLY)
        nx_, ny_ = 32 - r0 * 0.1, ground - r0 * 2.2 - U * 0.2
        capsule(c, 32 + r0 * 0.2, ground - r0 * 0.9, nx_, ny_, r0 * 0.3, M_PRI, r1=r0 * 0.25)
        if champ:
            spike(c, 32 - r0 * 0.9, ground - r0 * 0.9, r0 * 0.9, r0 * 0.2, -1.2, M_METAL, mirror=True)
        body = (32, ground - r0 * 0.7, r0, r0 * 0.5)
        hr = U * 0.38 * g['hr'] * (1.0 + 0.3 * young)
        h = Head(nx_, ny_ - hr * 0.5, hr)
        animal_head_features(h)
        ellipse(c, h.x, h.y, hr * 1.1, hr * 0.85, M_PRI)
        heads.append(h)
        tip_element(c, t, 32 + r0 * 1.15, ground - r0 * 0.5, U * 0.2, 1.2)
    elif arch == 'avian':
        bw_ = U * 0.45 * g['bw']
        bh_ = U * 0.55 * g['bh']
        bcy = ground - U * 0.3 - bh_
        wl = U * (0.75 + 0.15 * (tier - 2))
        pts = [(32 - bw_ * 0.5, bcy - bh_ * 0.4), (32 - bw_ - wl, bcy - bh_ - wl * 0.3), (32 - bw_ - wl * 1.1, bcy - bh_ * 0.2),
               (32 - bw_ - wl * 0.8, bcy + bh_ * 0.1), (32 - bw_ - wl * 0.5, bcy + bh_ * 0.35), (32 - bw_ * 0.6, bcy + bh_ * 0.4)]
        poly(c, pts, M_SEC, mirror=True)
        if champ:
            for k in range(2):
                spike(c, 32 - bw_ - wl * (0.3 + 0.25 * k), bcy - bh_ * 0.2 + k * 2, wl * 0.5, U * 0.08, -2.3, M_METAL, mirror=True)
        spike(c, 32, bcy + bh_ * 0.5, U * 0.7, U * 0.18, 3.0, M_SEC)
        capsule(c, 32 - bw_ * 0.3, bcy + bh_ * 0.6, 32 - bw_ * 0.35, ground - 1, U * 0.07, M_DARK, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        body = (32, bcy, bw_, bh_)
        if g['belly']:
            ellipse(c, 32, bcy + bh_ * 0.2, bw_ * 0.6, bh_ * 0.65, M_BELLY)
        hr = U * 0.36 * g['hr'] * (1.0 + 0.35 * young)
        h = Head(32, bcy - bh_ - hr * 0.3, hr)
        animal_head_features(h)
        ellipse(c, 32, h.y, hr, hr * 0.95, M_PRI)
        poly(c, [(32 - hr * 0.3, h.y + hr * 0.25), (32 + hr * 0.3, h.y + hr * 0.25), (32, h.y + hr * 0.9)], M_ACC)
        heads.append(h)
    elif arch == 'insect':
        ab = U * 0.5 * g['bw']
        acy = ground - U * 0.25 - ab * 0.9
        ellipse(c, 32 - ab * 0.9, acy - ab * 1.3, ab * 0.8, ab * 0.35, M_ACC, rot=-0.6, mirror=True)
        for k in range(3):
            y0 = acy - ab * 0.7 + k * ab * 0.35
            capsule(c, 32 - ab * 0.4, y0, 32 - ab * 1.3, y0 + ab * 0.5 + k * 2, 1.1, M_DARK, mirror=True)
            capsule(c, 32 - ab * 1.3, y0 + ab * 0.5 + k * 2, 32 - ab * 1.45, ground - 1 if k == 2 else y0 + ab * 1.1, 1.0, M_DARK, mirror=True)
        if champ:
            sword(c, 32 - ab * 1.2, acy - ab * 0.9, -0.9, U * 0.8, U * 0.07, mirror=True)
        ellipse(c, 32, acy, ab, ab * 0.9, M_PRI)
        ellipse(c, 32, acy - ab * 1.0, ab * 0.55, ab * 0.45, M_SEC)
        body = (32, acy, ab, ab * 0.9)
        hr = U * 0.32 * g['hr'] * (1.0 + 0.3 * young)
        h = Head(32, acy - ab * 1.35 - hr * 0.6, hr)
        capsule(c, 32 - hr * 0.3, h.y - hr * 0.6, 32 - hr * 1.0, h.y - hr * 1.9, 0.8, M_DARK, mirror=True)
        tip_element(c, t, 32 - hr * 1.0, h.y - hr * 1.9, 3, mirror=True)
        ellipse(c, 32, h.y, hr * 1.05, hr * 0.85, M_PRI)
        heads.append(h)
    elif arch == 'mech':
        bw_ = U * 0.5 * g['bw']
        bh_ = U * 0.5 * g['bh']
        legh = U * 0.4
        top = ground - legh - bh_ * 2
        rectp(c, 32 - bw_ * 0.75, ground - legh, 32 - bw_ * 0.25, ground - 1, M_DARK, mirror=True)
        rectp(c, 32 - bw_ * 0.85, ground - 4, 32 - bw_ * 0.15, ground - 1, M_SEC, mirror=True)
        capsule(c, 32 - bw_ - 1, top + 3, 32 - bw_ - U * 0.25, top + bh_ * 1.6, U * 0.11 * g['limb'], M_DARK, mirror=True)
        if champ:
            cannon(c, 32 - bw_ - U * 0.25, top + bh_ * 1.6, 3.14, U * 0.45, U * 0.1)
        rectp(c, 32 - bw_, top, 32 + bw_, top + bh_ * 2, M_PRI)
        rectp(c, 32 - bw_ * 0.6, top + bh_ * 0.3, 32 + bw_ * 0.6, top + bh_ * 1.2, M_METAL)
        body = (32, top + bh_, bw_, bh_)
        hr = U * 0.34 * g['hr'] * (1.0 + 0.3 * young)
        h = Head(32, top - hr * 0.8, hr, 'box')
        rectp(c, 32 - hr, h.y - hr * 0.85, 32 + hr, h.y + hr * 0.85, M_PRI)
        g['eyes'] = 'visor'
        heads.append(h)
    elif arch == 'fish':
        bw_ = U * 0.8 * g['bw']
        bh_ = U * 0.42 * g['bh']
        cy = 36
        ellipse(c, 32, ground - 1, bw_ * 0.7, 2.2, M_SHADOW)
        spike(c, 32 - bw_ * 0.6, cy + bh_ * 0.3, U * 0.8, U * 0.22, -2.3, M_SEC, mirror=True)
        spike(c, 32, cy - bh_ * 0.6, U * 0.8, U * 0.25, 0.0, M_SEC)
        if champ:
            cannon(c, 32 - bw_ * 0.8, cy, -2.2, U * 0.5, U * 0.08, mirror=True)
        ellipse(c, 32, cy, bw_, bh_, M_PRI)
        if g['belly']:
            ellipse(c, 32, cy + bh_ * 0.4, bw_ * 0.7, bh_ * 0.45, M_BELLY)
        body = (32, cy, bw_, bh_)
        h = Head(32, cy - bh_ * 0.15, bh_ * 0.95)
        heads.append(h)

    # ======================================================== ULTRA / MEGA warrior forms
    elif arch in ('knight', 'gunner', 'angel', 'demon', 'insectwarrior'):
        H = U * 2.15
        hip = ground - H * 0.42
        sh = hip - H * 0.33
        sw = H * (0.2 if arch != 'demon' else 0.24) * g['bw']
        hr = H * 0.11
        hy = sh - hr * 1.05
        # behind: cape / wings / back weapons
        if arch == 'angel':
            feather_wing(c, 32 - sw * 0.4, sh + H * 0.03, H * 0.42, H * 0.3, pairs=2 if mega else 1)
            if mega:
                feather_wing(c, 32 - sw * 0.3, sh + H * 0.22, H * 0.3, H * 0.2, pairs=1, mat=M_ACC)
        elif arch == 'demon':
            bat_wing(c, 32 - sw * 0.5, sh + H * 0.02, H * 0.45, H * 0.3, M_SEC)
            capsule(c, 32 + sw * 0.3, hip, 32 + sw * 2.0, hip + H * 0.12, H * 0.035, M_PRI, r1=1.0)
            spike(c, 32 + sw * 2.0, hip + H * 0.12, H * 0.12, H * 0.04, 1.6, M_ACC)
        elif arch == 'insectwarrior':
            ellipse(c, 32 - sw * 1.2, sh - H * 0.05, H * 0.24, H * 0.09, M_ACC, rot=-0.7, mirror=True)
            ellipse(c, 32 - sw * 1.1, sh + H * 0.1, H * 0.2, H * 0.07, M_ACC, rot=0.3, mirror=True)
        if g['cape'] and arch in ('knight', 'demon', 'gunner'):
            poly(c, [(32 - sw * 0.9, sh), (32 + sw * 0.9, sh), (32 + sw * 1.6, ground - 2), (32, ground - 5), (32 - sw * 1.6, ground - 2)], M_SEC)
        if g['wings'] and arch in ('knight', 'gunner'):
            mech_wing(c, 32 - sw * 0.5, sh + H * 0.05, H * 0.38, H * 0.28)
        if arch == 'gunner' and mega:
            missile_pod(c, 32 - sw * 1.0, sh - H * 0.1, H * 0.16, H * 0.14, mirror=True)
        elif arch == 'gunner' or (arch == 'knight' and mega and g['weapon'] in ('armcannon', 'gatling')):
            cannon(c, 32 + sw * 0.7, sh, -0.35, H * 0.32, H * 0.045)
        # legs
        if arch == 'angel':
            poly(c, [(32 - sw * 0.75, hip - H * 0.02), (32 + sw * 0.75, hip - H * 0.02), (32 + sw * 1.3, ground - 1), (32 - sw * 1.3, ground - 1)], M_BELLY)
            poly(c, [(32 - sw * 0.35, hip), (32 + sw * 0.35, hip), (32 + sw * 0.55, ground - 1), (32 - sw * 0.55, ground - 1)], M_SEC)
        else:
            lw = H * (0.07 if arch != 'demon' else 0.085)
            capsule(c, 32 - sw * 0.45, hip, 32 - sw * 0.55, hip + H * 0.2, lw, M_PRI, mirror=True)
            capsule(c, 32 - sw * 0.55, hip + H * 0.2, 32 - sw * 0.6, ground - 3, lw * 1.05, M_METAL if arch != 'demon' else M_PRI, mirror=True)
            ellipse(c, 32 - sw * 0.55, hip + H * 0.2, lw * 1.15, lw * 0.95, M_METAL, mirror=True)
            ellipse(c, 32 - sw * 0.65, ground - 2, lw * 1.5, lw * 0.75, M_DARK, mirror=True)
            if arch == 'demon':
                spike(c, 32 - sw * 0.85, ground - 2, lw * 1.2, lw * 0.3, -1.9, M_ACC, mirror=True)
        # torso
        tor = [(32 - sw, sh), (32 + sw, sh), (32 + sw * 0.62, hip + H * 0.02), (32 - sw * 0.62, hip + H * 0.02)]
        poly(c, tor, M_PRI)
        if arch == 'insectwarrior':
            ellipse(c, 32, hip - H * 0.08, sw * 0.8, H * 0.12, M_SEC)
            for k in range(3):
                rectp(c, 32 - sw * 0.6, sh + H * 0.05 + k * H * 0.08, 32 + sw * 0.6, sh + H * 0.08 + k * H * 0.08, M_METAL)
        else:
            poly(c, [(32 - sw * 0.85, sh + 1), (32 + sw * 0.85, sh + 1), (32 + sw * 0.5, sh + H * 0.2), (32, sh + H * 0.26), (32 - sw * 0.5, sh + H * 0.2)], M_METAL)
            ellipse(c, 32, sh + H * 0.11, H * 0.045, H * 0.045, M_ACC)
        rectp(c, 32 - sw * 0.65, hip - H * 0.04, 32 + sw * 0.65, hip + H * 0.01, M_DARK)
        # off-hand arm (screen right) + shield / weapon
        ohx, ohy = 32 + sw + H * 0.05, hip - H * 0.02
        capsule(c, 32 + sw * 0.85, sh + 2, ohx, ohy, H * 0.055, M_PRI)
        capsule(c, ohx - 0.5, ohy - H * 0.1, ohx, ohy, H * 0.06, M_METAL)
        w = g['weapon']
        off = g['offhand']
        # main arm (screen left)
        mhx, mhy = 32 - sw - H * 0.08, hip - H * 0.06
        if arch == 'gunner' and w in ('armcannon', 'gatling'):
            capsule(c, 32 - sw * 0.85, sh + 2, 32 - sw - H * 0.03, sh + H * 0.12, H * 0.06, M_PRI)
            if w == 'gatling':
                gatling(c, 32 - sw - H * 0.05, sh + H * 0.14, 3.6, H * 0.3, H * 0.035)
            else:
                cannon(c, 32 - sw - H * 0.05, sh + H * 0.12, 3.5, H * 0.34, H * 0.055)
        else:
            capsule(c, 32 - sw * 0.85, sh + 2, mhx, mhy, H * 0.055, M_PRI)
            capsule(c, mhx + 0.5, mhy - H * 0.1, mhx, mhy, H * 0.06, M_METAL)
            if arch == 'demon':
                for k in range(3):
                    spike(c, mhx + (k - 1) * 1.2, mhy + 1, H * 0.08, H * 0.015, 3.14 + (k - 1) * 0.3, M_ACC)
                if mega:
                    sword(c, mhx, mhy - H * 0.08, -2.6, H * 0.35, H * 0.03, energy=True)
            elif arch == 'insectwarrior':
                sword(c, mhx, mhy, -0.6, H * 0.4, H * 0.035)
                sword(c, ohx, ohy, 0.6, H * 0.4, H * 0.035)
            elif w == 'lance' or arch == 'angel':
                lance(c, mhx, mhy, -0.15, H * 0.75, H * 0.04)
            elif w in ('sword', 'scythe', 'claws', 'backcannons', 'pods', 'gatling', 'armcannon'):
                sword(c, mhx, mhy, -0.5, H * (0.55 if mega else 0.48), H * 0.04, energy=(mega and g['energy']))
            ellipse(c, mhx, mhy, H * 0.045, H * 0.045, M_DARK)
        if arch in ('knight', 'angel') and off == 'shield':
            shield(c, ohx + H * 0.02, ohy - H * 0.08, H * 0.12, H * 0.26)
        elif arch == 'gunner' and off == 'cannon':
            cannon(c, ohx, ohy - H * 0.05, 2.7, H * 0.25, H * 0.04)
        elif arch == 'demon':
            for k in range(3):
                spike(c, ohx + (k - 1) * 1.2, ohy + 1, H * 0.08, H * 0.015, 3.14 + (k - 1) * 0.3, M_ACC)
        else:
            ellipse(c, ohx, ohy, H * 0.045, H * 0.045, M_DARK)
        # shoulders
        pauldron(c, 32 - sw * 0.95, sh + 1, H * 0.085 * (1.25 if mega else 1.0), 2 if mega else (1 if arch in ('demon', 'knight') else 0), mirror=True)
        # head
        h = Head(32, hy, hr, 'helm')
        if arch == 'demon':
            spike(c, 32 - hr * 0.6, hy - hr * 0.5, hr * (2.0 if mega else 1.5), hr * 0.35, -0.7, M_METAL, mirror=True)
            ellipse(c, 32, hy, hr * 1.05, hr, M_PRI)
            poly(c, [(32 - hr * 0.8, hy + hr * 0.3), (32 + hr * 0.8, hy + hr * 0.3), (32, hy + hr * 1.2)], M_PRI)
            h.kind = 'round'
        elif arch == 'insectwarrior':
            capsule(c, 32 - hr * 0.3, hy - hr * 0.6, 32 - hr * 1.2, hy - hr * 2.0, 0.8, M_DARK, mirror=True)
            ellipse(c, 32, hy, hr * 1.1, hr * 0.9, M_PRI)
            spike(c, 32 - hr * 0.4, hy + hr * 0.6, hr * 0.8, hr * 0.2, 2.8, M_METAL, mirror=True)
            h.kind = 'round'
        else:
            helmet(c, h, g, tier)
        if arch == 'angel' or (mega and arch == 'knight' and g['halo']):
            ring(c, 32, hy - hr * 1.6, hr * 1.1, hr * 0.35, 1.2, M_ACC)
        heads.append(h)
        body = (32, (sh + hip) / 2, sw, (hip - sh) / 2)
        if arch == 'gunner':
            g['eyes'] = 'visor'
        elif arch in ('knight', 'angel'):
            g['eyes'] = 'visor' if g['eyes'] in ('visor', 'normal') else 'glow'
    elif arch == 'titan':
        H = U * 2.1
        legh = H * 0.32
        top = ground - legh - H * 0.36
        bw_ = H * 0.26 * g['bw']
        if mega:
            missile_pod(c, 32 - bw_ * 0.9, top - H * 0.08, H * 0.2, H * 0.16, mirror=True)
        else:
            cannon(c, 32 - bw_ * 0.6, top + 1, -0.25, H * 0.28, H * 0.05, mirror=True)
        rectp(c, 32 - bw_ * 0.85, ground - legh, 32 - bw_ * 0.25, ground - 1, M_DARK, mirror=True)
        rectp(c, 32 - bw_ * 0.95, ground - legh * 0.45, 32 - bw_ * 0.15, ground - 1, M_METAL, mirror=True)
        rectp(c, 32 - bw_ * 1.05, ground - 4, 32 - bw_ * 0.05, ground - 1, M_SEC, mirror=True)
        # arms
        capsule(c, 32 - bw_ - 2, top + 4, 32 - bw_ - H * 0.12, top + H * 0.33, H * 0.06, M_DARK)
        gatling(c, 32 - bw_ - H * 0.12, top + H * 0.36, 3.14, H * 0.24, H * 0.04)
        capsule(c, 32 + bw_ + 2, top + 4, 32 + bw_ + H * 0.12, top + H * 0.33, H * 0.06, M_DARK)
        cannon(c, 32 + bw_ + H * 0.12, top + H * 0.36, 3.14, H * 0.24, H * 0.06)
        rectp(c, 32 - bw_, top, 32 + bw_, top + H * 0.36, M_PRI)
        rectp(c, 32 - bw_ * 0.7, top + H * 0.05, 32 + bw_ * 0.7, top + H * 0.22, M_METAL)
        ellipse(c, 32, top + H * 0.14, H * 0.05, H * 0.05, M_ACC)
        rectp(c, 32 - bw_ * 1.3, top - 2, 32 - bw_ * 0.75, top + H * 0.12, M_SEC, mirror=True)
        hr = H * 0.09
        h = Head(32, top - hr * 0.6, hr, 'box')
        if g['helm'] in ('horns', 'vfin'):
            spike(c, 32 - hr * 0.4, top - hr * 1.2, hr * 1.4, hr * 0.25, -0.6, M_ACC, mirror=True)
        rectp(c, 32 - hr * 1.1, h.y - hr, 32 + hr * 1.1, h.y + hr * 0.8, M_PRI)
        g['eyes'] = 'visor'
        heads.append(h)
        body = (32, top + H * 0.18, bw_, H * 0.18)
    elif arch == 'dragon':
        H = U * 2.1
        bw_ = H * 0.23 * g['bw']
        bh_ = H * 0.22
        bcy = ground - H * 0.22 - bh_
        # wings & tail
        bat_wing(c, 32 - bw_ * 0.5, bcy - bh_ * 0.7, H * (0.55 if mega else 0.46), H * 0.36, M_SEC)
        capsule(c, 32 + bw_ * 0.5, bcy + bh_ * 0.6, 32 + bw_ * 1.9, ground - 3, H * 0.06, M_PRI, r1=1.2)
        tip_element(c, t, 32 + bw_ * 2.0, ground - 4, H * 0.07, 1.3)
        if mega:
            cannon(c, 32 - bw_ * 0.7, bcy - bh_ * 0.7, -0.35, H * 0.3, H * 0.045, mirror=True)
        # legs
        capsule(c, 32 - bw_ * 0.6, bcy + bh_ * 0.4, 32 - bw_ * 0.75, ground - 3, H * 0.08, M_PRI, mirror=True)
        ellipse(c, 32 - bw_ * 0.85, ground - 2, H * 0.09, H * 0.04, M_DARK, mirror=True)
        for k in range(3):
            spike(c, 32 - bw_ * 0.85 - H * 0.05 + k * 2, ground - 2, H * 0.05, H * 0.012, -2.4 + k * 0.4, M_BELLY, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        ellipse(c, 32, bcy + bh_ * 0.2, bw_ * 0.6, bh_ * 0.7, M_BELLY)
        if tier >= 4:
            poly(c, [(32 - bw_ * 0.9, bcy - bh_ * 0.6), (32 + bw_ * 0.9, bcy - bh_ * 0.6), (32 + bw_ * 0.5, bcy + bh_ * 0.1), (32 - bw_ * 0.5, bcy + bh_ * 0.1)], M_METAL)
            ellipse(c, 32, bcy - bh_ * 0.25, H * 0.03, H * 0.03, M_ACC)
        # arms with claws
        capsule(c, 32 - bw_ * 0.8, bcy - bh_ * 0.4, 32 - bw_ * 1.3, bcy + bh_ * 0.3, H * 0.045, M_PRI, mirror=True)
        for k in range(3):
            spike(c, 32 - bw_ * 1.3 + (k - 1), bcy + bh_ * 0.3, H * 0.06, H * 0.012, 3.0 + (k - 1) * 0.3, M_BELLY, mirror=True)
        # neck + head
        hy = bcy - bh_ - H * 0.1
        capsule(c, 32, bcy - bh_ * 0.6, 32, hy, H * 0.08, M_PRI)
        hr = H * 0.12
        h = Head(32, hy, hr)
        spike(c, 32 - hr * 0.6, hy - hr * 0.5, hr * 1.9, hr * 0.3, -0.6, M_METAL if tier >= 4 else M_SEC, mirror=True)
        if mega:
            spike(c, 32 - hr * 0.9, hy - hr * 0.1, hr * 1.4, hr * 0.25, -1.3, M_METAL, mirror=True)
        ellipse(c, 32, hy, hr * 1.25, hr * 1.0, M_PRI)
        ellipse(c, 32, hy + hr * 0.95, hr * 0.95, hr * 0.55, M_DARK)
        ellipse(c, 32, hy + hr * 0.55, hr * 0.8, hr * 0.55, M_PRI)
        for k in range(5):
            spike(c, 32 - hr * 0.7 + k * hr * 0.35, hy + hr * 0.95, hr * 0.38, hr * 0.09, 3.14, M_BELLY)
        ellipse(c, 32 - hr * 0.3, hy + hr * 0.35, hr * 0.12, hr * 0.09, M_DARK, mirror=True)
        if tier >= 4:
            rectp(c, 32 - hr * 0.9, hy - hr * 0.95, 32 + hr * 0.9, hy - hr * 0.35, M_METAL)
        heads.append(h)
        body = (32, bcy, bw_, bh_)
    elif arch == 'cannonbeast':
        H = U * 2.0
        bw_ = H * 0.36 * g['bw']
        bh_ = H * 0.18
        bcy = ground - H * 0.17 - bh_
        n = 2 if not mega else 4
        for k in range(n):
            cannon(c, 32 - bw_ * (0.25 + 0.35 * (k // 2)), bcy - bh_ * 0.8, -0.25 - 0.25 * (k // 2), H * (0.42 - 0.08 * (k // 2)), H * 0.05, mirror=True)
        capsule(c, 32 - bw_ * 0.75, bcy, 32 - bw_ * 0.85, ground - 3, H * 0.06, M_DARK, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        poly(c, [(32 - bw_ * 0.85, bcy - bh_ * 0.5), (32 + bw_ * 0.85, bcy - bh_ * 0.5), (32 + bw_ * 0.6, bcy + bh_ * 0.2), (32 - bw_ * 0.6, bcy + bh_ * 0.2)], M_METAL)
        if mega:
            missile_pod(c, 32 - bw_ * 0.95, bcy - bh_ * 0.2, H * 0.12, H * 0.1, mirror=True)
        capsule(c, 32 - bw_ * 0.38, bcy + bh_ * 0.3, 32 - bw_ * 0.42, ground - 2, H * 0.07, M_PRI, mirror=True)
        ellipse(c, 32 - bw_ * 0.42, ground - 2, H * 0.08, H * 0.04, M_METAL, mirror=True)
        hr = H * 0.13
        h = Head(32, bcy - bh_ * 0.1, hr)
        spike(c, 32 - hr * 0.5, h.y - hr * 0.5, hr * 1.4, hr * 0.28, -0.5, M_METAL, mirror=True)
        ellipse(c, 32, h.y, hr * 1.1, hr * 0.95, M_PRI)
        rectp(c, 32 - hr * 0.95, h.y - hr * 0.9, 32 + hr * 0.95, h.y - hr * 0.3, M_METAL)
        ellipse(c, 32, h.y + hr * 0.55, hr * 0.55, hr * 0.35, M_BELLY)
        heads.append(h)
        body = (32, bcy, bw_, bh_)
    elif arch == 'serpentdragon':
        r0 = U * 0.75 * g['bw']
        if mega:
            bat_wing(c, 32 - r0 * 0.3, ground - r0 * 2.0, r0 * 1.4, r0 * 0.9, M_SEC)
        ellipse(c, 32, ground - r0 * 0.3, r0 * 1.25, r0 * 0.33, M_PRI)
        ellipse(c, 32 + r0 * 0.2, ground - r0 * 0.75, r0 * 0.95, r0 * 0.3, M_PRI)
        for k in range(2 if not mega else 3):
            cannon(c, 32 - r0 * (0.6 + 0.3 * k), ground - r0 * (0.6 + 0.3 * k), -1.0 - 0.3 * k, r0 * 0.7, r0 * 0.08, mirror=True)
        ellipse(c, 32, ground - r0 * 0.3, r0 * 0.85, r0 * 0.15, M_BELLY)
        nx_, ny_ = 32, ground - r0 * 2.1
        capsule(c, 32 + r0 * 0.2, ground - r0 * 0.85, nx_, ny_, r0 * 0.28, M_PRI, r1=r0 * 0.22)
        for k in range(4):
            spike(c, 32 + r0 * 0.25 - k * 1.0, ground - r0 * (1.0 + 0.3 * k), r0 * 0.35, r0 * 0.08, 0.9, M_METAL)
        hr = r0 * 0.33
        h = Head(nx_, ny_ - hr * 0.3, hr)
        spike(c, nx_ - hr * 0.6, h.y - hr * 0.5, hr * 1.8, hr * 0.3, -0.6, M_METAL, mirror=True)
        ellipse(c, nx_, h.y, hr * 1.3, hr * 0.9, M_PRI)
        ellipse(c, nx_, h.y + hr * 0.7, hr * 0.9, hr * 0.4, M_DARK)
        for k in range(4):
            spike(c, nx_ - hr * 0.6 + k * hr * 0.4, h.y + hr * 0.4, hr * 0.35, hr * 0.1, 3.14, M_BELLY)
        heads.append(h)
        body = (32, ground - r0 * 0.6, r0, r0 * 0.4)
    elif arch == 'mechbird':
        H = U * 2.0
        bw_ = H * 0.13
        bh_ = H * 0.26
        bcy = ground - H * 0.42
        mech_wing(c, 32 - bw_ * 0.5, bcy - bh_ * 0.2, H * 0.5, H * 0.32)
        for k in range(1 if not mega else 2):
            cannon(c, 32 - bw_ - H * (0.12 + 0.12 * k), bcy + bh_ * 0.1, 3.0, H * 0.22, H * 0.035, mirror=True, housing=False)
        poly(c, [(32 - bw_ * 0.6, bcy + bh_), (32 + bw_ * 0.6, bcy + bh_), (32 + bw_ * 1.6, ground - 1), (32, ground - 5), (32 - bw_ * 1.6, ground - 1)], M_SEC)
        if mega:
            ellipse(c, 32 - bw_ * 0.6, bcy + bh_ * 1.0, H * 0.04, H * 0.06, M_ACC, mirror=True)
        ellipse(c, 32, bcy, bw_, bh_, M_PRI)
        poly(c, [(32 - bw_ * 0.8, bcy - bh_ * 0.4), (32 + bw_ * 0.8, bcy - bh_ * 0.4), (32, bcy + bh_ * 0.6)], M_METAL)
        hr = H * 0.09
        h = Head(32, bcy - bh_ - hr * 0.2, hr)
        spike(c, 32 - hr * 0.3, h.y - hr * 0.6, hr * 1.6, hr * 0.25, -0.45, M_ACC, mirror=True)
        ellipse(c, 32, h.y, hr, hr * 0.95, M_PRI)
        poly(c, [(32 - hr * 0.35, h.y + hr * 0.3), (32 + hr * 0.35, h.y + hr * 0.3), (32, h.y + hr * 1.3)], M_METAL)
        g['eyes'] = 'visor'
        heads.append(h)
        body = (32, bcy, bw_, bh_)

    if body is None:
        body = (32, 40, 10, 10)
    bx, by, bw_, bh_ = body
    if L.get('legend') is not None and heads:
        hd = heads[0]
        # crown of energy spikes and a double halo
        for k in range(5):
            spike(c, hd.x + (k - 2) * hd.r * 0.45, hd.y - hd.r * 0.9, hd.r * (1.0 + 0.5 * (k == 2)), hd.r * 0.13, (k - 2) * 0.35, M_ACC)
        ring(c, hd.x, hd.y - hd.r * 2.1, hd.r * 1.6, hd.r * 0.45, 1.2, M_ACC)
    if t2 >= 0 and tier >= 3 and arch not in ('titan', 'knight', 'gunner', 'angel'):
        tip_element(c, t2, bx - bw_ * 0.75, by - bh_ * 0.6, 3 + tier * 0.6, -0.6, mirror=True)
    return c, heads, body, g, t2


# ------------------------------------------------------------------ palette
TYPE_COL = {
    0: (12, .85, .95), 1: (212, .75, .85), 2: (128, .65, .8), 3: (50, .9, 1.0), 4: (178, .7, .85), 5: (196, .35, .97),
    6: (85, .7, .85), 7: (28, .45, .7), 8: (316, .55, .9), 9: (278, .65, .78), 10: (220, .18, .82), 11: (258, .55, .5),
    12: (352, .75, .8), 13: (48, .22, 1.0), 14: (300, .6, .45), 15: (25, .55, .8), 16: (168, .35, .95), 17: (85, .35, .62),
    18: (330, .8, .72), 19: (238, .7, .62),
}
NEON = {0: [45, 52, 40], 1: [185, 170, 300], 2: [80, 320, 55], 3: [55, 190, 60], 4: [300, 190, 330], 5: [190, 300, 170],
        6: [120, 25, 190], 7: [25, 40, 160], 8: [300, 55, 190], 9: [100, 120, 330], 10: [190, 25, 330], 11: [280, 330, 160],
        12: [40, 20, 190], 13: [55, 190, 45], 14: [0, 330, 120], 15: [40, 160, 0], 16: [190, 60, 300], 17: [0, 40, 120],
        18: [120, 300, 0], 19: [290, 190, 55]}
METAL = {13: [(150, 110, 30), (220, 175, 60), (255, 240, 170)], 14: [(40, 30, 50), (90, 75, 105), (170, 150, 185)],
         17: [(55, 62, 45), (115, 125, 95), (190, 200, 165)], 12: [(70, 50, 55), (150, 120, 125), (225, 205, 205)]}


def make_palette(L, rl):
    t = L['t']
    base_h, sat, val = TYPE_COL[t]
    h = (base_h + rl.uniform(-16, 16)) % 360
    s = min(1, max(0.1, sat + rl.uniform(-0.12, 0.12)))
    v = min(1, max(0.35, val + rl.uniform(-0.1, 0.05)))

    def ramp(h, s, v, n):
        out = []
        for i in range(n):
            k = i / (n - 1)
            hh = (h + (k - 0.5) * 22 * (1 if 60 < h < 240 else -1)) % 360
            ss = min(1, s * (1.15 - 0.45 * k)) if s > 0.25 else s
            vv = v * (0.46 + 0.56 * k)
            if k == 1:
                vv = min(1, v * 1.08)
            r, g, b = colorsys.hsv_to_rgb(hh / 360, ss, vv)
            out.append((r * 255, g * 255, b * 255))
        return out
    pri = ramp(h, s, v, 4)
    choice = rl.random()
    if choice < 0.35:
        sh, ss, sv = (h + 180 + rl.uniform(-30, 30)) % 360, s * 0.9, min(1, v * 1.05)
    elif choice < 0.65:
        sh, ss, sv = (h + rl.choice([-40, 40])) % 360, s, v * 0.9
    elif choice < 0.85:
        sh, ss, sv = h, s * 0.2, min(1, v * 1.15 + 0.1)
    else:
        sh, ss, sv = h, s * 0.6, v * 0.45
    sec = ramp(sh, ss, max(0.3, sv), 3)
    neon = NEON[t]
    ah = rl.choice([n for n in neon if min(abs(n - h), 360 - abs(n - h)) > 30] or neon)
    acc = []
    for (ss2, vv2) in [(1, .65), (.85, 1), (.25, 1)]:
        r, g, b = colorsys.hsv_to_rgb(ah / 360, ss2, vv2)
        acc.append((r * 255, g * 255, b * 255))
    if t in METAL:
        metal = METAL[t]
    else:
        mh = (h + 200) % 360
        metal = []
        for vv, ssx in ((0.33, 0.25), (0.62, 0.15), (0.92, 0.07)):
            r, g, b = colorsys.hsv_to_rgb(mh / 360, ssx, vv)
            metal.append((r * 255, g * 255, b * 255))
    r, g, b = colorsys.hsv_to_rgb(h / 360, 0.7, 0.12)
    outline = (r * 255, g * 255, b * 255)
    belly = ramp(sh if choice < 0.65 else (h + 20) % 360, ss * 0.5, min(1, sv * 1.1 + 0.1), 3)
    pal = [(0, 0, 0), outline] + pri + sec + acc + metal + [(255, 255, 255)]
    return pal, belly


# ------------------------------------------------------------------ render
def render(L, stage, tier):
    c, heads, body, g, t2 = build(L, stage, tier, 7000 + L['idx'])
    pal, belly = make_palette(L, random.Random(8000 + L['idx']))
    img = np.zeros((S, S), np.uint8)
    f = c.fill
    sh = c.shade
    m = c.mat
    bayer = np.array([[0, 2], [3, 1]], np.float32) / 4.0 - 0.375
    q = sh + np.tile(bayer, (S // 2, S // 2)) * 0.12
    pri_idx = np.select([q < 0.3, q < 0.55, q < 0.8], [2, 3, 4], 5)
    sec_idx = np.select([q < 0.38, q < 0.72], [6, 7], 8)
    acc_idx = np.select([q < 0.3, q < 0.85], [9, 10], 11)
    met_idx = np.select([q < 0.36, q < 0.74, q < 0.95], [12, 13, 14], 15)
    for mat, idx in ((M_PRI, pri_idx), (M_SEC, sec_idx), (M_ACC, acc_idx), (M_METAL, met_idx), (M_BELLY, sec_idx)):
        sel = f & (m == mat)
        img[sel] = idx[sel]
    sel = f & (m == M_DARK)
    img[sel] = np.select([q < 0.45, q < 0.8], [1, 12], 13)[sel]
    if not (L['t'] in (13, 16) or tier <= 1):
        pass
    # belly uses a lighter secondary when secondary is dark
    bx, by, bw_, bh_ = body
    onp = f & (m == M_PRI)
    pat = g['pattern']
    r2 = random.Random(31 + L['idx'])
    if pat == 'stripes' and tier >= 2:
        per = max(3, int(bh_ * 0.5))
        band = ((yy.astype(int) - int(by)) % per == 0) & (np.abs(xx - bx) < bw_) & (np.abs(yy - by) < bh_)
        img[onp & band] = 6
    elif pat == 'spots':
        for k in range(3 + tier):
            sx = bx + r2.uniform(-0.7, 0.7) * bw_
            sy = by + r2.uniform(-0.7, 0.4) * bh_
            rr = max(1.0, bw_ * 0.14)
            for mm in (False, True):
                X = (S - sx) if mm else sx
                d = (xx - X) ** 2 + (yy - sy) ** 2 <= rr * rr
                img[onp & d] = 7
    elif pat == 'circuit' and tier >= 2:
        for k in range(tier):
            sx = bx - bw_ * r2.uniform(0.15, 0.7)
            sy = by + bh_ * r2.uniform(-0.6, 0.4)
            ln = int(bw_ * 0.6)
            for mm in (False, True):
                X = int(round((S - sx) if mm else sx))
                Y = int(round(sy))
                for i in range(ln):
                    xi = X + (i if not mm else -i)
                    yi = Y
                    if i > ln // 2:
                        yi = Y + (i - ln // 2)
                        xi = X + ((ln // 2) if not mm else -(ln // 2))
                    if 0 <= xi < S and 0 <= yi < S and onp[yi, xi]:
                        img[yi, xi] = 10
    if pat == 'core' and tier in (2, 3):
        rr = max(1.5, min(bw_, bh_) * 0.3)
        d = np.sqrt((xx - bx) ** 2 + (yy - (by + bh_ * 0.05)) ** 2)
        img[(d <= rr) & f & (m == M_PRI)] = 10
        img[(d <= rr * 0.55) & f & (m == M_PRI)] = 11
    # inner lines between overlapping parts of the same material
    pid = c.pid
    inner = np.zeros((S, S), bool)
    for dy_, dx_ in ((0, 1), (1, 0), (0, -1), (-1, 0)):
        sp = np.roll(np.roll(pid, dy_, 0), dx_, 1)
        sf = np.roll(np.roll(f, dy_, 0), dx_, 1)
        smat = np.roll(np.roll(m, dy_, 0), dx_, 1)
        inner |= f & sf & (sp < pid) & (smat == m) & (m != M_ACC)
    img[inner & (img >= 2) & (img <= 5)] = 2
    img[inner & (img >= 6) & (img <= 8)] = 6
    img[inner & (img >= 12) & (img <= 15)] = 12
    # eyes
    for hd in heads:
        er = max(1.2, hd.r * 0.24)
        ex = hd.r * 0.42
        ey = hd.y + hd.r * 0.08
        style = g['eyes']
        if hd.kind == 'helm':
            ey = hd.y + hd.r * 0.35
        if style == 'visor':
            band = (np.abs(yy - ey) <= max(1.0, hd.r * 0.16)) & (np.abs(xx - hd.x) <= hd.r * 0.72) & f
            img[band] = 10
            hl = (np.abs(yy - (ey - max(1.0, hd.r * 0.16) + 0.5)) <= 0.5) & (np.abs(xx - hd.x) <= hd.r * 0.55) & f
            img[hl] = 11
            continue
        if style == 'mono':
            er2 = er * 1.8
            d = np.sqrt((xx - hd.x) ** 2 + (yy - ey) ** 2)
            img[(d <= er2 + 0.8) & f] = 1
            img[d <= er2] = 15
            img[d <= er2 * 0.6] = 10
            img[d <= er2 * 0.28] = 1
            continue
        for sgn in (-1, 1):
            cx = hd.x + sgn * ex
            d = ((xx - cx) / (er * 0.9)) ** 2 + ((yy - ey) / er) ** 2
            if style == 'glow' or hd.kind == 'helm':
                img[(d <= 1.6) & f] = 1
                img[d <= 1.0] = 10
                img[d <= 0.35] = 11
            else:
                img[(d <= 1.5) & f] = 1
                img[d <= 1.0] = 15
                pd = ((xx - (cx - sgn * er * 0.15)) / (er * 0.6)) ** 2 + ((yy - (ey + er * 0.15)) / (er * 0.75)) ** 2
                img[pd <= 1.0] = 1
                gx, gy = int(cx - er * 0.35), int(ey - er * 0.45)
                if 0 <= gx < S and 0 <= gy < S:
                    img[gy, gx] = 15
        if g['mouth'] and hd.kind == 'round' and tier <= 3:
            my = int(round(hd.y + hd.r * 0.5))
            for x in range(int(hd.x - hd.r * 0.18), int(hd.x + hd.r * 0.18) + 1):
                if 0 <= my < S and f[my, x]:
                    img[my, x] = 2
    # rim light and outline
    filled = img > 0
    out = np.zeros((S, S), bool)
    for dy_, dx_ in ((0, 1), (1, 0), (0, -1), (-1, 0)):
        out |= np.roll(np.roll(filled, dy_, 0), dx_, 1)
    out &= ~filled
    edge_r = filled & ~np.roll(filled, -1, 1)
    edge_b = filled & ~np.roll(filled, -1, 0)
    rim = (edge_r | edge_b) & (img >= 2) & (img <= 4)
    img[rim] = 9
    img[out] = 1
    img[c.shadow & (img == 0)] = 1
    if tier == 5:
        e = ((xx - 32) / 29.0) ** 2 + ((yy - 60.5) / 3.6) ** 2
        rg = (e <= 1.0) & (e >= 0.55) & (img == 0)
        img[rg] = 9
        img[rg & (yy < 60)] = 10
        for k in range(6):
            a = k / 6 * 2 * math.pi
            px_, py_ = int(32 + math.cos(a) * 29), int(30 + math.sin(a) * 25)
            for (dx_, dy_) in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
                X, Y = px_ + dx_, py_ + dy_
                if 0 <= X < S and 0 <= Y < S and img[Y, X] == 0:
                    img[Y, X] = 11 if (dx_, dy_) == (0, 0) else 10
    elif tier == 4:
        e = ((xx - 32) / 26.0) ** 2 + ((yy - 60.5) / 3.2) ** 2
        rg = (e <= 1.0) & (e >= 0.6) & (img == 0)
        img[rg] = 9
    return img, pal
