#pragma once
#include "plat.h"

#define RGB(r, g, b) ((u16)(0x8000 | ((r) & 31) | (((g) & 31) << 5) | (((b) & 31) << 10)))
#define C_BLACK RGB(0, 0, 0)
#define C_WHITE RGB(31, 31, 31)
#define C_CYAN RGB(0, 28, 31)
#define C_MAGENTA RGB(31, 6, 24)
#define C_YELLOW RGB(31, 28, 6)
#define C_RED RGB(31, 6, 8)
#define C_GREEN RGB(6, 30, 14)
#define C_GREY RGB(16, 17, 21)
#define C_DARK RGB(3, 3, 7)
#define C_PANEL RGB(4, 5, 11)
#define C_TEXT RGB(29, 30, 31)
#define C_TEXT2 RGB(17, 22, 28)

void gfx_init(void);
void fill(u16 *fb, int x, int y, int w, int h, u16 c);
void fill_blend(u16 *fb, int x, int y, int w, int h, u16 c, int alpha); // alpha 0..16
void darken(u16 *fb, int x, int y, int w, int h, int amount);           // 0..16
void hline(u16 *fb, int x, int y, int w, u16 c);
void vline(u16 *fb, int x, int y, int h, u16 c);
void rect_outline(u16 *fb, int x, int y, int w, int h, u16 c);
void gradient_v(u16 *fb, int x, int y, int w, int h, u16 c0, u16 c1);
u16 lerp_col(u16 a, u16 b, int t, int tmax);
void pset(u16 *fb, int x, int y, u16 c);
void line(u16 *fb, int x0, int y0, int x1, int y1, u16 c);
void circle_fill(u16 *fb, int cx, int cy, int r, u16 c);

void draw_tile(u16 *fb, int tile, int frame_counter, int x, int y);
void gfx_set_tint(int t);   // 0 none, 1 morning, 2 evening, 3 night (overworld lighting)
u16 tint_col(u16 c, int t);
void draw_chr(u16 *fb, int chr, int frame, int x, int y, int flip);
// scale in 1/2 steps: 2 = 1x, 3 = 1.5x, 4 = 2x. mode: 0 normal, 1 white silhouette, 2 dark silhouette, 3 tinted
void draw_mon(u16 *fb, int species, int x, int y, int scale2, int flip, int mode);
void draw_mon_clip(u16 *fb, int species, int x, int y, int scale2, int flip, int mode, int clip_y0, int clip_y1);

int text_width(const char *s);
int draw_text(u16 *fb, int x, int y, const char *s, u16 c);
int draw_text_sh(u16 *fb, int x, int y, const char *s, u16 c, u16 shadow);
void draw_text_big(u16 *fb, int x, int y, const char *s, u16 c, u16 shadow, int scale);
// word wrap into box, returns number of lines; maxchars limits drawn glyphs (-1 = all)
int draw_text_wrap(u16 *fb, int x, int y, int w, const char *s, u16 c, u16 shadow, int maxchars, int line_h);
int text_glyph_count(const char *s);
const char *utf8_next(const char *s, int *cp);

void panel(u16 *fb, int x, int y, int w, int h, u16 border);
void panel_solid(u16 *fb, int x, int y, int w, int h, u16 bg, u16 border);
void button(u16 *fb, int x, int y, int w, int h, const char *label, u16 col, int selected);
void hpbar(u16 *fb, int x, int y, int w, int hp, int maxhp);
void type_badge(u16 *fb, int x, int y, int type);
u16 type_color(int type);
void draw_capsule_icon(u16 *fb, int x, int y, int kind);
void light_cone(u16 *fb, int cx, int cy);
const u8 *mon_pixels(int species);
