#include <string.h>
#include "gfx.h"
#include "gen_gfx.h"
#include "gen_mon.h"

static short glyph_lo[256];

void gfx_init(void)
{
    for (int i = 0; i < 256; i++) glyph_lo[i] = -1;
    for (int i = 0; i < font_count; i++)
        if (font_cp[i] < 256) glyph_lo[font_cp[i]] = i;
}

static int glyph_of(int cp)
{
    if (cp < 256) return glyph_lo[cp];
    for (int i = 0; i < font_count; i++)
        if (font_cp[i] == cp) return i;
    return -1;
}

const char *utf8_next(const char *s, int *cp)
{
    const unsigned char *u = (const unsigned char *)s;
    if (u[0] < 0x80) { *cp = u[0]; return s + 1; }
    if ((u[0] & 0xE0) == 0xC0 && u[1]) { *cp = ((u[0] & 0x1F) << 6) | (u[1] & 0x3F); return s + 2; }
    if ((u[0] & 0xF0) == 0xE0 && u[1] && u[2]) { *cp = ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F); return s + 3; }
    *cp = '?';
    return s + 1;
}

void pset(u16 *fb, int x, int y, u16 c)
{
    if ((unsigned)x < SCR_W && (unsigned)y < SCR_H) fb[y * SCR_W + x] = c;
}

void fill(u16 *fb, int x, int y, int w, int h, u16 c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        u16 *p = fb + (y + j) * SCR_W + x;
        for (int i = 0; i < w; i++) p[i] = c;
    }
}

static inline u16 blend(u16 a, u16 b, int t)
{
    int ar = a & 31, ag = (a >> 5) & 31, ab = (a >> 10) & 31;
    int br = b & 31, bg = (b >> 5) & 31, bb = (b >> 10) & 31;
    return 0x8000 | ((ar + (((br - ar) * t) >> 4)) & 31) | (((ag + (((bg - ag) * t) >> 4)) & 31) << 5) |
           (((ab + (((bb - ab) * t) >> 4)) & 31) << 10);
}

u16 lerp_col(u16 a, u16 b, int t, int tmax)
{
    if (tmax <= 0) return a;
    int ar = a & 31, ag = (a >> 5) & 31, ab = (a >> 10) & 31;
    int br = b & 31, bg = (b >> 5) & 31, bb = (b >> 10) & 31;
    return RGB(ar + (br - ar) * t / tmax, ag + (bg - ag) * t / tmax, ab + (bb - ab) * t / tmax);
}

void fill_blend(u16 *fb, int x, int y, int w, int h, u16 c, int alpha)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        u16 *p = fb + (y + j) * SCR_W + x;
        for (int i = 0; i < w; i++) p[i] = blend(p[i], c, alpha);
    }
}

void darken(u16 *fb, int x, int y, int w, int h, int amount)
{
    fill_blend(fb, x, y, w, h, C_BLACK, amount);
}

void hline(u16 *fb, int x, int y, int w, u16 c) { fill(fb, x, y, w, 1, c); }
void vline(u16 *fb, int x, int y, int h, u16 c) { fill(fb, x, y, 1, h, c); }

void rect_outline(u16 *fb, int x, int y, int w, int h, u16 c)
{
    hline(fb, x, y, w, c); hline(fb, x, y + h - 1, w, c);
    vline(fb, x, y, h, c); vline(fb, x + w - 1, y, h, c);
}

void gradient_v(u16 *fb, int x, int y, int w, int h, u16 c0, u16 c1)
{
    for (int j = 0; j < h; j++) fill(fb, x, y + j, w, 1, lerp_col(c0, c1, j, h - 1));
}

void line(u16 *fb, int x0, int y0, int x1, int y1, u16 c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pset(fb, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void circle_fill(u16 *fb, int cx, int cy, int r, u16 c)
{
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r) pset(fb, cx + x, cy + y, c);
}

// ------------------------------------------------------------------ day / night lighting
#define MAX_TILE_FRAMES 96
static u16 tint_tiles[3][MAX_TILE_FRAMES][256] __attribute__((aligned(4)));
static u8 tint_ready[3];
static int cur_tint;

u16 tint_col(u16 c, int t)
{
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (t == 1) { r = (r * 13 + 31 * 3) / 16; g = (g * 13 + 22 * 3) / 16; b = (b * 13 + 22 * 3) / 16; }
    else if (t == 2) { r = (r * 12 + 31 * 4) / 16 * 14 / 16; g = (g * 12 + 12 * 4) / 16 * 13 / 16; b = (b * 12 + 6 * 4) / 16 * 12 / 16; }
    else if (t == 3) {
        if (mx >= 26) { r = r * 14 / 16; g = g * 14 / 16; b = b * 15 / 16; }   // neon and lamps keep glowing
        else { r = (r * 6 + 1 * 10) / 16; g = (g * 6 + 2 * 10) / 16; b = (b * 7 + 7 * 9) / 16; }
    } else if (t == 4) { r = r * 11 / 16; g = g * 11 / 16; b = (b * 12 + 10 * 4) / 16; }   // sprites at night
    return (u16)(r | (g << 5) | (b << 10) | (c & 0x8000));
}

void gfx_set_tint(int t)
{
    if (t < 0 || t > 3) t = 0;
    cur_tint = t;
    if (!t || tint_ready[t - 1]) return;
    int frames = 0;
    for (int i = 0; i < tile_count; i++) if (tile_info[i][0] + tile_info[i][1] > frames) frames = tile_info[i][0] + tile_info[i][1];
    if (frames > MAX_TILE_FRAMES) frames = MAX_TILE_FRAMES;
    for (int f = 0; f < frames; f++)
        for (int k = 0; k < 256; k++) tint_tiles[t - 1][f][k] = tint_col(tile_gfx[f][k], t);
    tint_ready[t - 1] = 1;
}

void draw_tile(u16 *fb, int tile, int fc, int x, int y)
{
    int first = tile_info[tile][0], n = tile_info[tile][1];
    int fr = first + (n > 1 ? (fc / 16) % n : 0);
    const u16 *src = cur_tint && fr < MAX_TILE_FRAMES ? tint_tiles[cur_tint - 1][fr] : tile_gfx[fr];
    if (x >= 0 && y >= 0 && x + 16 <= SCR_W && y + 16 <= SCR_H && !(x & 1)) {
        // fast path: fully visible, word aligned
        u32 *d = (u32 *)(fb + y * SCR_W + x);
        const u32 *s = (const u32 *)src;
        for (int j = 0; j < 16; j++) {
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
            d += SCR_W / 2; s += 8;
        }
        return;
    }
    int x0 = 0, x1 = 16, y0 = 0, y1 = 16;
    if (x < 0) x0 = -x;
    if (y < 0) y0 = -y;
    if (x + 16 > SCR_W) x1 = SCR_W - x;
    if (y + 16 > SCR_H) y1 = SCR_H - y;
    if (x0 >= x1 || y0 >= y1) return;
    for (int j = y0; j < y1; j++)
        memcpy(fb + (y + j) * SCR_W + x + x0, src + j * 16 + x0, (x1 - x0) * 2);
}

void draw_chr(u16 *fb, int chr, int frame, int x, int y, int flip)
{
    const u8 *g = chr_gfx[chr][frame];
    const u16 *pal = chr_pal[chr];
    u16 tp[16];
    if (cur_tint) {
        for (int k = 0; k < 16; k++) tp[k] = tint_col(pal[k], cur_tint == 3 ? 4 : cur_tint);   // sprites a bit lighter than the map
        pal = tp;
    }
    for (int j = 0; j < 24; j++) {
        int yy = y + j;
        if ((unsigned)yy >= SCR_H) continue;
        for (int i = 0; i < 16; i++) {
            int xx = x + i;
            if ((unsigned)xx >= SCR_W) continue;
            u8 v = g[j * 16 + (flip ? 15 - i : i)];
            if (v) fb[yy * SCR_W + xx] = pal[v];
        }
    }
}

// ------------------------------------------------------------------ compressed creature sprites
static void lz_decode(const u8 *src, u8 *dst, int n)
{
    int o = 0;
    while (o < n) {
        u8 flags = *src++;
        for (int bit = 0; bit < 8 && o < n; bit++) {
            if (flags & (1 << bit)) {
                int len = (src[0] >> 4) + 3;
                int off = (((src[0] & 15) << 8) | src[1]) + 1;
                src += 2;
                for (int k = 0; k < len && o < n; k++, o++) dst[o] = dst[o - off];
            } else dst[o++] = *src++;
        }
    }
}

#define MON_CACHE 32
static u8 mon_cache[MON_CACHE][2048] __attribute__((aligned(4)));
static s16 mon_cache_sp[MON_CACHE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
static u32 mon_cache_use[MON_CACHE], mon_cache_clock;

const u8 *mon_pixels(int sp)
{
    int victim = 0;
    mon_cache_clock++;
    for (int i = 0; i < MON_CACHE; i++) {
        if (mon_cache_sp[i] == sp) { mon_cache_use[i] = mon_cache_clock; return mon_cache[i]; }
        if (mon_cache_use[i] < mon_cache_use[victim]) victim = i;
    }
    lz_decode(mon_lz + mon_lz_ofs[sp], mon_cache[victim], 2048);
    mon_cache_sp[victim] = sp;
    mon_cache_use[victim] = mon_cache_clock;
    return mon_cache[victim];
}

void draw_mon_clip(u16 *fb, int sp, int x, int y, int scale2, int flip, int mode, int cy0, int cy1)
{
    const u8 *g = mon_pixels(sp);
    const u16 *pal = mon_pal[sp];
    int size = 32 * scale2;
    // 16.16 fixed point source step (no divisions in the inner loop)
    static const int steps[9] = { 0, 2 << 16, 1 << 16, 43691, 1 << 15, 26214, 21845, 18725, 1 << 14 };
    int step = steps[scale2 & 7];
    u16 lut[16];
    int shiny = mode & 0x10;
    mode &= 15;
    for (int k = 1; k < 16; k++) {
        u16 c = pal[k];
        if (shiny) c = (u16)(((c & 31) << 10) | ((c >> 5) & 31) | (((c >> 10) & 31) << 5));   // rotate colour channels
        if (mode == 1) c = C_WHITE;
        else if (mode == 2) c = RGB(2, 2, 6);
        else if (mode == 3) c = blend(c, C_WHITE, 9);
        else if (mode == 4) c = blend(c, RGB(31, 0, 8), 8);
        lut[k] = c;
    }
    if (scale2 == 4 && !flip && x >= 0 && x + 128 <= SCR_W) {
        // fast 2x path: each source pixel -> 2x2 block
        for (int sy = 0; sy < 64; sy++) {
            int yy = y + sy * 2;
            if (yy + 1 < cy0 || yy >= cy1) continue;
            const u8 *row = g + sy * 32;
            u16 *d0 = fb + yy * SCR_W + x;
            int r0 = yy >= cy0 && yy < cy1, r1 = yy + 1 >= cy0 && yy + 1 < cy1;
            for (int sx = 0; sx < 32; sx++) {
                u8 b = row[sx];
                if (!b) continue;
                u8 v0 = b & 15, v1 = b >> 4;
                u16 *p = d0 + sx * 4;
                if (v0) { u16 c = lut[v0]; if (r0) { p[0] = c; p[1] = c; } if (r1) { p[SCR_W] = c; p[SCR_W + 1] = c; } }
                if (v1) { u16 c = lut[v1]; if (r0) { p[2] = c; p[3] = c; } if (r1) { p[SCR_W + 2] = c; p[SCR_W + 3] = c; } }
            }
        }
        return;
    }
    if (scale2 == 4 && flip && x >= 0 && x + 128 <= SCR_W) {
        for (int sy = 0; sy < 64; sy++) {
            int yy = y + sy * 2;
            if (yy + 1 < cy0 || yy >= cy1) continue;
            const u8 *row = g + sy * 32;
            u16 *d0 = fb + yy * SCR_W + x + 126;
            int r0 = yy >= cy0 && yy < cy1, r1 = yy + 1 >= cy0 && yy + 1 < cy1;
            for (int sx = 0; sx < 32; sx++) {
                u8 b = row[sx];
                if (!b) continue;
                u8 v0 = b & 15, v1 = b >> 4;
                u16 *p = d0 - sx * 4;
                if (v0) { u16 c = lut[v0]; if (r0) { p[0] = c; p[1] = c; } if (r1) { p[SCR_W] = c; p[SCR_W + 1] = c; } }
                if (v1) { u16 c = lut[v1]; if (r0) { p[-2] = c; p[-1] = c; } if (r1) { p[SCR_W - 2] = c; p[SCR_W - 1] = c; } }
            }
        }
        return;
    }
    int i0 = 0, i1 = size;
    if (x < 0) i0 = -x;
    if (x + size > SCR_W) i1 = SCR_W - x;
    if (cy0 < 0) cy0 = 0;
    if (cy1 > SCR_H) cy1 = SCR_H;
    int sy_fp = 0;
    for (int j = 0; j < size; j++, sy_fp += step) {
        int yy = y + j;
        if (yy < cy0 || yy >= cy1) continue;
        const u8 *row = g + (sy_fp >> 16) * 32;
        u16 *dst = fb + yy * SCR_W + x;
        int sx_fp = i0 * step;
        for (int i = i0; i < i1; i++, sx_fp += step) {
            int sx = sx_fp >> 16;
            if (flip) sx = 63 - sx;
            u8 b = row[sx >> 1];
            u8 v = (sx & 1) ? (b >> 4) : (b & 15);
            if (v) dst[i] = lut[v];
        }
    }
}

void draw_mon(u16 *fb, int sp, int x, int y, int scale2, int flip, int mode)
{
    draw_mon_clip(fb, sp, x, y, scale2, flip, mode, 0, SCR_H);
}

static int glyph_draw(u16 *fb, int x, int y, int g, u16 c)
{
    const u8 *b = font_bits[g];
    int w = font_w[g];
    for (int j = 0; j < 10; j++) {
        u8 r = b[j];
        if (!r) continue;
        for (int i = 0; i < w; i++)
            if (r & (1 << i)) pset(fb, x + i, y + j, c);
    }
    return w + 1;
}

int text_width(const char *s)
{
    int w = 0, cp;
    while (*s) {
        s = utf8_next(s, &cp);
        if (cp == '\n') break;
        int g = glyph_of(cp);
        if (g >= 0) w += font_w[g] + 1;
    }
    return w > 0 ? w - 1 : 0;
}

int draw_text(u16 *fb, int x, int y, const char *s, u16 c)
{
    int cp, x0 = x;
    while (*s) {
        s = utf8_next(s, &cp);
        if (cp == '\n') { y += 11; x = x0; continue; }
        int g = glyph_of(cp);
        if (g >= 0) x += glyph_draw(fb, x, y, g, c);
    }
    return x;
}

int draw_text_sh(u16 *fb, int x, int y, const char *s, u16 c, u16 sh)
{
    draw_text(fb, x + 1, y + 1, s, sh);
    return draw_text(fb, x, y, s, c);
}

void draw_text_big(u16 *fb, int x, int y, const char *s, u16 c, u16 sh, int sc)
{
    int cp;
    while (*s) {
        s = utf8_next(s, &cp);
        int g = glyph_of(cp);
        if (g < 0) continue;
        const u8 *b = font_bits[g];
        int w = font_w[g];
        for (int j = 0; j < 10; j++)
            for (int i = 0; i < w; i++)
                if (b[j] & (1 << i)) {
                    if (sh != c) fill(fb, x + i * sc + sc / 2 + 1, y + j * sc + sc / 2 + 1, sc, sc, sh);
                    fill(fb, x + i * sc, y + j * sc, sc, sc, c);
                }
        x += (w + 1) * sc;
    }
}

int text_glyph_count(const char *s)
{
    int n = 0, cp;
    while (*s) { s = utf8_next(s, &cp); n++; }
    return n;
}

// word wrap
int draw_text_wrap(u16 *fb, int x, int y, int w, const char *s, u16 c, u16 sh, int maxchars, int lh)
{
    int cx = x, cy = y, lines = 1, drawn = 0;
    const char *p = s;
    while (*p) {
        if (*p == '\n') { cx = x; cy += lh; lines++; p++; drawn++; continue; }
        if (*p == ' ') {
            // measure next word
            const char *q = p + 1;
            int ww = 0, cp;
            while (*q && *q != ' ' && *q != '\n') {
                q = utf8_next(q, &cp);
                int g = glyph_of(cp);
                if (g >= 0) ww += font_w[g] + 1;
            }
            if (cx + 4 + ww > x + w) { cx = x; cy += lh; lines++; p++; drawn++; continue; }
        }
        int cp;
        p = utf8_next(p, &cp);
        int g = glyph_of(cp);
        if (g >= 0) {
            if (maxchars < 0 || drawn < maxchars) {
                if (fb) {
                    if (sh != c) glyph_draw(fb, cx + 1, cy + 1, g, sh);
                    glyph_draw(fb, cx, cy, g, c);
                }
            }
            cx += font_w[g] + 1;
        }
        drawn++;
    }
    return lines;
}

void panel(u16 *fb, int x, int y, int w, int h, u16 border)
{
    fill_blend(fb, x + 1, y + 1, w - 2, h - 2, C_PANEL, 13);
    // cut corners, neon border
    hline(fb, x + 3, y, w - 6, border);
    hline(fb, x + 3, y + h - 1, w - 6, border);
    vline(fb, x, y + 3, h - 6, border);
    vline(fb, x + w - 1, y + 3, h - 6, border);
    pset(fb, x + 1, y + 2, border); pset(fb, x + 2, y + 1, border);
    pset(fb, x + w - 2, y + 2, border); pset(fb, x + w - 3, y + 1, border);
    pset(fb, x + 1, y + h - 3, border); pset(fb, x + 2, y + h - 2, border);
    pset(fb, x + w - 2, y + h - 3, border); pset(fb, x + w - 3, y + h - 2, border);
    // glow
    fill_blend(fb, x + 3, y + 1, w - 6, 1, border, 5);
    hline(fb, x + 6, y + h - 3, 10, border);
}

static void panel_border(u16 *fb, int x, int y, int w, int h, u16 border)
{
    hline(fb, x + 3, y, w - 6, border);
    hline(fb, x + 3, y + h - 1, w - 6, border);
    vline(fb, x, y + 3, h - 6, border);
    vline(fb, x + w - 1, y + 3, h - 6, border);
    pset(fb, x + 1, y + 2, border); pset(fb, x + 2, y + 1, border);
    pset(fb, x + w - 2, y + 2, border); pset(fb, x + w - 3, y + 1, border);
    pset(fb, x + 1, y + h - 3, border); pset(fb, x + 2, y + h - 2, border);
    pset(fb, x + w - 2, y + h - 3, border); pset(fb, x + w - 3, y + h - 2, border);
    hline(fb, x + 6, y + h - 3, 10, border);
}

void panel_solid(u16 *fb, int x, int y, int w, int h, u16 bg, u16 border)
{
    fill(fb, x + 1, y + 1, w - 2, h - 2, blend(bg, C_PANEL, 13));
    hline(fb, x + 3, y + 1, w - 6, blend(blend(bg, C_PANEL, 13), border, 5));
    panel_border(fb, x, y, w, h, border);
}

void button(u16 *fb, int x, int y, int w, int h, const char *label, u16 col, int sel)
{
    u16 dark = lerp_col(col, C_BLACK, 12, 16);
    u16 mid = lerp_col(col, C_BLACK, 7, 16);
    if (sel) { dark = lerp_col(col, C_BLACK, 6, 16); mid = lerp_col(col, C_WHITE, 2, 16); }
    gradient_v(fb, x + 1, y + 1, w - 2, h - 2, blend(mid, C_PANEL, 6), blend(dark, C_PANEL, 6));
    panel_border(fb, x, y, w, h, sel ? C_WHITE : col);
    if (sel) {
        fill_blend(fb, x + 2, y + 2, w - 4, 2, C_WHITE, 6);
    }
    int tw = text_width(label);
    draw_text_sh(fb, x + (w - tw) / 2, y + (h - 9) / 2, label, C_WHITE, RGB(2, 2, 6));
}

void hpbar(u16 *fb, int x, int y, int w, int hp, int maxhp)
{
    fill(fb, x, y, w, 5, RGB(2, 2, 5));
    rect_outline(fb, x - 1, y - 1, w + 2, 7, RGB(10, 12, 18));
    int fw = maxhp > 0 ? (hp * w + maxhp - 1) / maxhp : 0;
    if (hp <= 0) fw = 0;
    u16 c = hp * 2 > maxhp ? RGB(4, 30, 16) : (hp * 5 > maxhp ? RGB(31, 26, 4) : RGB(31, 6, 6));
    fill(fb, x, y, fw, 5, c);
    fill(fb, x, y, fw, 1, lerp_col(c, C_WHITE, 8, 16));
    fill(fb, x, y + 4, fw, 1, lerp_col(c, C_BLACK, 6, 16));
}

static const u16 TYPE_COL[NUM_TYPES] = {
    RGB(31, 12, 4), RGB(6, 16, 31), RGB(6, 26, 10), RGB(31, 28, 4), RGB(4, 26, 26), RGB(18, 26, 31),
    RGB(18, 28, 6), RGB(22, 15, 8), RGB(30, 10, 26), RGB(20, 8, 28), RGB(20, 22, 25), RGB(12, 6, 22),
    RGB(26, 4, 10), RGB(31, 29, 18), RGB(18, 2, 16), RGB(24, 14, 6), RGB(14, 28, 26), RGB(15, 18, 8),
    RGB(31, 6, 18), RGB(10, 8, 28),
};

u16 type_color(int t) { return TYPE_COL[t % NUM_TYPES]; }

void type_badge(u16 *fb, int x, int y, int t)
{
    u16 c = TYPE_COL[t % NUM_TYPES];
    fill(fb, x, y, 44, 11, lerp_col(c, C_BLACK, 6, 16));
    hline(fb, x, y, 44, lerp_col(c, C_WHITE, 6, 16));
    const char *n = type_names[t % NUM_TYPES];
    draw_text_sh(fb, x + (44 - text_width(n)) / 2, y + 1, n, C_WHITE, RGB(1, 1, 3));
}

void draw_capsule_icon(u16 *fb, int x, int y, int kind)
{
    u16 top = kind == 0 ? RGB(0, 26, 31) : kind == 1 ? RGB(31, 8, 24) : kind == 2 ? RGB(31, 26, 4) : RGB(20, 6, 31);
    circle_fill(fb, x + 4, y + 4, 4, RGB(2, 2, 6));
    circle_fill(fb, x + 4, y + 4, 3, RGB(26, 27, 30));
    fill(fb, x + 1, y + 1, 7, 3, top);
    fill(fb, x + 2, y, 5, 1, top);
    hline(fb, x, y + 4, 9, RGB(2, 2, 6));
    pset(fb, x + 4, y + 4, C_WHITE);
    pset(fb, x + 2, y + 1, C_WHITE);
}

// Fast light cone: concentric rings, darkened with shifts on two pixels at once.
static inline u32 dk(u32 v, int lvl)
{
    switch (lvl) {
    case 1: return (((v >> 1) & 0x3DEF3DEF) + ((v >> 2) & 0x1CE71CE7)) | 0x80008000;
    case 2: return ((v >> 1) & 0x3DEF3DEF) | 0x80008000;
    case 3: return ((v >> 2) & 0x1CE71CE7) | 0x80008000;
    default: return ((v >> 3) & 0x0C630C63) | 0x80008000;
    }
}

static inline void dk_span(u32 *row, int a, int b, int lvl)
{
    if (a < 0) a = 0;
    if (b > SCR_W) b = SCR_W;
    a &= ~1; b = (b + 1) & ~1;
    for (int x = a; x < b; x += 2) row[x >> 1] = dk(row[x >> 1], lvl);
}

void light_cone(u16 *fb, int cx, int cy)
{
    static const int R[4] = { 50, 64, 64, 64 };
    static u8 isq[64 * 64 + 1];
    static int init;
    if (!init) {
        int s = 0;
        for (int i = 0; i <= 64 * 64; i++) { while ((s + 1) * (s + 1) <= i) s++; isq[i] = s; }
        init = 1;
    }
    for (int y = 0; y < SCR_H; y++) {
        int dy = y - cy, dy2 = dy * dy;
        u32 *row = (u32 *)(fb + y * SCR_W);
        int L[4], Rr[4];
        for (int k = 0; k < 4; k++) {
            int r2 = R[k] * R[k] - dy2;
            int h = r2 >= 0 ? isq[r2] : -1;
            L[k] = h < 0 ? cx : cx - h;
            Rr[k] = h < 0 ? cx : cx + h;
        }
        dk_span(row, L[1], L[0], 1);
        dk_span(row, Rr[0], Rr[1], 1);
    }
}
