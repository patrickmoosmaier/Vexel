// PC test harness: scripted input, screenshots as PPM. Not part of the DS build.
#ifndef __NDS__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "plat.h"

static u16 buf_top[SCR_W * SCR_H];
static u16 buf_bot[SCR_W * SCR_H];
u16 *fb_top = buf_top;
u16 *fb_bot = buf_bot;

static FILE *script;
static int hold_frames;
static u32 hold_keys;
static int touch_on, touch_x, touch_y;
static long frame_no;
static const char *outdir = ".";
static int light_on, light_x, light_y, light_r, light_lvl;
void plat_light(int cx, int cy, int r, int level) { light_on = 1; light_x = cx; light_y = cy; light_r = r; light_lvl = level; }
void plat_light_off(void) { light_on = 0; }
void plat_light_hole(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }

static void shot(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n256 384\n255\n");
    for (int s = 0; s < 2; s++) {
        u16 *b = s ? buf_bot : buf_top;
        for (int i = 0; i < SCR_W * SCR_H; i++) {
            u16 c = b[i];
            unsigned char px[3] = { (c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3 };
            if (s == 0 && light_on) {
                int x = i % SCR_W, y = i / SCR_W, dx = x - light_x, dy = y - light_y;
                if (dx * dx + dy * dy >= light_r * light_r)
                    for (int k = 0; k < 3; k++) px[k] = px[k] * (16 - light_lvl) / 16;
            }
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
}

void plat_init(void)
{
    const char *s = getenv("VX_SCRIPT");
    if (s) script = fopen(s, "r");
    const char *o = getenv("VX_OUT");
    if (o) outdir = o;
}

static u32 parse_keys(const char *k)
{
    u32 r = 0;
    if (strchr(k, 'A')) r |= K_A;
    if (strchr(k, 'B')) r |= K_B;
    if (strchr(k, 'X')) r |= K_X;
    if (strchr(k, 'Y')) r |= K_Y;
    if (strchr(k, 'S')) r |= K_START;
    if (strchr(k, 'E')) r |= K_SELECT;
    if (strchr(k, 'U')) r |= K_UP;
    if (strchr(k, 'D')) r |= K_DOWN;
    if (strchr(k, 'L')) r |= K_LEFT;
    if (strchr(k, 'R')) r |= K_RIGHT;
    if (strchr(k, 'l')) r |= K_L;
    if (strchr(k, 'r')) r |= K_R;
    return r;
}

static void next_cmd(void)
{
    char line[256];
    while (hold_frames <= 0) {
        if (!script || !fgets(line, sizeof(line), script)) {
            shot("final");
            fprintf(stderr, "script end at frame %ld\n", frame_no);
            exit(0);
        }
        char a[64] = "", b[64] = "";
        int n = 0;
        touch_on = 0;
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "shot %63s", a) == 1) { shot(a); continue; }
        if (sscanf(line, "touch %d %d %d", &touch_x, &touch_y, &n) == 3) { touch_on = 1; hold_keys = K_TOUCH; hold_frames = n; continue; }
        int c = sscanf(line, "%d %63s", &n, b);
        if (c >= 1) { hold_frames = n; hold_keys = (c == 2 && strcmp(b, "-")) ? parse_keys(b) : 0; }
    }
}

void plat_present(int top_dirty, int bot_dirty)
{
    (void)top_dirty; (void)bot_dirty;
    frame_no++;
    static long maxf = -1;
    if (maxf < 0) { const char *m = getenv("VX_MAXFRAMES"); maxf = m ? atol(m) : 0; }
    if (maxf && frame_no > maxf) { shot("final"); fprintf(stderr, "max frames reached\n"); exit(0); }
    hold_frames--;
}

u32 plat_keys(void)
{
    next_cmd();
    return hold_keys;
}

int plat_touch(int *x, int *y)
{
    if (!touch_on) return 0;
    *x = touch_x; *y = touch_y;
    return 1;
}

u32 plat_seed(void) { return 12345; }
#include <time.h>
int plat_minutes(void)
{
    const char *h = getenv("VX_HOUR");   // fixed hour for reproducible tests
    if (h) return atoi(h) * 60;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    return tm->tm_hour * 60 + tm->tm_min;
}
u32 plat_vblanks(void) { return (u32)frame_no; }
u32 plat_ticks(void) { return 0; }
void plat_tone(int voice, int freq, int vol, int duty) { (void)voice; (void)freq; (void)vol; (void)duty; }
void plat_tone_off(int voice) { (void)voice; }
void plat_tone_vol(int voice, int vol) { (void)voice; (void)vol; }

int plat_save_write(const void *data, int len)
{
    FILE *f = fopen("/tmp/vexel_save.bin", "wb");
    if (!f) return 0;
    fwrite(data, 1, len, f);
    fclose(f);
    return 1;
}

int plat_save_read(void *data, int len)
{
    FILE *f = fopen("/tmp/vexel_save.bin", "rb");
    if (!f) { memset(data, 0xFF, len); return 1; }
    size_t r = fread(data, 1, len, f);
    (void)r;
    fclose(f);
    return 1;
}
#endif
