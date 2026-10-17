// DS platform layer
#ifdef __NDS__
#include <nds.h>
#include <string.h>
#include "plat.h"

static u16 buf_top[SCR_W * SCR_H] __attribute__((aligned(32)));
static u16 buf_bot[SCR_W * SCR_H] __attribute__((aligned(32)));
u16 *fb_top = buf_top;
u16 *fb_bot = buf_bot;

static int shown_bank;
static volatile u32 vbl_count;
static u16 win_tab[2][SCR_H + 1] __attribute__((aligned(32)));
static volatile int win_back, win_new, win_on;
static void vbl_irq(void)
{
    vbl_count++;
    if (win_new) { win_back ^= 1; win_new = 0; }
    DMA_CR(0) = 0;
    if (win_on) {
        const u16 *t = win_tab[win_back ^ 1];
        REG_WIN0H = t[0];
        DMA_SRC(0) = (u32)&t[1];
        DMA_DEST(0) = (u32)&REG_WIN0H;
        DMA_CR(0) = DMA_ENABLE | DMA_START_HBL | DMA_REPEAT | DMA_16_BIT | DMA_DST_FIX | DMA_SRC_INC | 1;
    }
}

static int hole_x, hole_y, hole_w, hole_h;
void plat_light_hole(int x, int y, int w, int h)
{
    hole_x = x; hole_y = y; hole_w = w; hole_h = h;
    if (!win_on) return;
    if (w > 0) {
        int x1 = x + w > 255 ? 255 : x + w, y1 = y + h > 192 ? 192 : y + h;
        REG_WIN1H = (x << 8) | x1;
        REG_WIN1V = (y << 8) | y1;
        REG_DISPCNT |= DISPLAY_WIN1_ON;
    } else REG_DISPCNT &= ~DISPLAY_WIN1_ON;
}

void plat_light(int cx, int cy, int r, int level)
{
    static u8 isq[160 * 160 + 1];
    static int init;
    if (!init) { int s = 0; for (int i = 0; i <= 160 * 160; i++) { while ((s + 1) * (s + 1) <= i) s++; isq[i] = s; } init = 1; }
    if (r > 159) r = 159;
    u16 *t = win_tab[win_back];   // back table (not shown)
    for (int y = 0; y <= SCR_H; y++) {
        int dy = y - cy, v = r * r - dy * dy;
        u16 e = 0;
        if (v > 0) {
            int h = isq[v];
            int l = cx - h, rr = cx + h;
            if (l < 0) l = 0;
            if (rr > 255) rr = 255;
            if (l < rr) e = (l << 8) | rr;
        }
        t[y] = e;
    }
    DC_FlushRange(t, sizeof(win_tab[0]));
    REG_WIN0V = (0 << 8) | SCR_H;
    REG_WININ = 0x0808;            // inside win0/win1: BG3, no effect
    win_on = 1;
    plat_light_hole(hole_x, hole_y, hole_w, hole_h);
    REG_WINOUT = 0x0028;           // outside: BG3 + colour effect
    REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG3;
    REG_BLDY = level;
    REG_DISPCNT |= DISPLAY_WIN0_ON;
    win_new = 1;
    win_on = 1;
}

void plat_light_off(void)
{
    win_on = 0;
    REG_DISPCNT &= ~(DISPLAY_WIN0_ON | DISPLAY_WIN1_ON);
    REG_BLDCNT = 0;
    REG_BLDY = 0;
}
u32 plat_vblanks(void) { return vbl_count; }
u32 plat_ticks(void) { return cpuGetTiming(); }   // 0 = bank A shown, 1 = bank B shown
static int voice_id[3] = { -1, -1, -1 };

#define VRAM_BANK_A ((u16 *)0x06000000)
#define VRAM_BANK_B ((u16 *)0x06020000)
#define VRAM_SUB    ((u16 *)0x06200000)

void plat_init(void)
{
    powerOn(POWER_ALL_2D);
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000, VRAM_C_SUB_BG_0x06200000, VRAM_D_LCD);

    REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(0);
    REG_BG3PA = 256; REG_BG3PB = 0; REG_BG3PC = 0; REG_BG3PD = 256;
    REG_BG3X = 0; REG_BG3Y = 0;
    REG_BG3CNT_SUB = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(0);
    REG_BG3PA_SUB = 256; REG_BG3PB_SUB = 0; REG_BG3PC_SUB = 0; REG_BG3PD_SUB = 256;
    REG_BG3X_SUB = 0; REG_BG3Y_SUB = 0;

    memset(VRAM_BANK_A, 0, 256 * 256 * 2);
    memset(VRAM_BANK_B, 0, 256 * 256 * 2);
    memset(VRAM_SUB, 0, 256 * 256 * 2);
    lcdMainOnTop();
    shown_bank = 0;
    soundEnable();
    cpuStartTiming(2);
    irqSet(IRQ_VBLANK, vbl_irq);
    irqEnable(IRQ_VBLANK);
}

void plat_present(int top_dirty, int bot_dirty)
{
    if (top_dirty) {
        u16 *dst = shown_bank ? VRAM_BANK_A : VRAM_BANK_B;
        DC_FlushRange(buf_top, sizeof(buf_top));
        dmaCopyWords(3, buf_top, dst, sizeof(buf_top));
    }
    if (bot_dirty)
        DC_FlushRange(buf_bot, sizeof(buf_bot));
    swiWaitForVBlank();
    if (top_dirty) {
        shown_bank ^= 1;
        REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(shown_bank ? 8 : 0) | BG_PRIORITY(0);
    }
    if (bot_dirty)
        dmaCopyWords(3, buf_bot, VRAM_SUB, sizeof(buf_bot));
}

u32 plat_keys(void)
{
    scanKeys();
    return keysHeld();
}

int plat_touch(int *x, int *y)
{
    if (!(keysHeld() & KEY_TOUCH)) return 0;
    touchPosition t;
    touchRead(&t);
    *x = t.px; *y = t.py;
    return 1;
}

#include <time.h>
int plat_minutes(void)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    return tm ? tm->tm_hour * 60 + tm->tm_min : 12 * 60;
}

u32 plat_seed(void)
{
    return REG_VCOUNT ^ (TIMER0_DATA << 8) ^ 0x5eed1234;
}

void plat_tone(int voice, int freq, int vol, int duty)
{
    if (voice_id[voice] >= 0) soundKill(voice_id[voice]);
    if (freq <= 0) { voice_id[voice] = -1; return; }
    if (voice == 2)
        voice_id[voice] = soundPlayNoise(freq, vol, 64);
    else
        voice_id[voice] = soundPlayPSG((DutyCycle)duty, freq, vol, voice == 0 ? 54 : 74);
}

void plat_tone_vol(int voice, int vol)
{
    if (voice_id[voice] >= 0) soundSetVolume(voice_id[voice], vol);
}

void plat_tone_off(int voice)
{
    if (voice_id[voice] >= 0) soundKill(voice_id[voice]);
    voice_id[voice] = -1;
}

// ---------------------------------------------------------------- save (card SPI EEPROM, 2-byte address)
static int spi_wait(void)
{
    int t = 200000;
    while (REG_AUXSPICNT & CARD_SPI_BUSY)
        if (--t == 0) return 0;
    return 1;
}

static u8 spi_x(u8 v)
{
    REG_AUXSPIDATA = v;
    spi_wait();
    return REG_AUXSPIDATA;
}

static void spi_begin(void) { REG_AUXSPICNT = CARD_ENABLE | CARD_SPI_ENABLE | CARD_SPI_HOLD; }
static void spi_end(void) { REG_AUXSPICNT = 0; }

// last byte of a transfer: clear HOLD so chip select is released (commits the command)
static u8 spi_last(u8 v)
{
    REG_AUXSPICNT = CARD_ENABLE | CARD_SPI_ENABLE;
    REG_AUXSPIDATA = v;
    spi_wait();
    u8 r = REG_AUXSPIDATA;
    spi_end();
    return r;
}

static int eep_wait_ready(void)
{
    for (int i = 0; i < 20000; i++) {
        spi_begin();
        spi_x(SPI_EEPROM_RDSR);
        u8 st = spi_last(0);
        if (!(st & 1)) return 1;
        if (st == 0xFF) return 0;   // no chip
    }
    return 0;
}

static void eep_read(u32 addr, u8 *d, int len)
{
    spi_begin();
    spi_x(SPI_EEPROM_READ);
    spi_x((addr >> 8) & 0xFF);
    if (len <= 0) { spi_last(addr & 0xFF); return; }
    spi_x(addr & 0xFF);
    for (int i = 0; i < len - 1; i++) d[i] = spi_x(0);
    d[len - 1] = spi_last(0);
}

static int eep_write(u32 addr, const u8 *d, int len)
{
    while (len > 0) {
        int n = 32 - (addr & 31);
        if (n > len) n = len;
        spi_begin(); spi_last(SPI_EEPROM_WREN);
        spi_begin();
        spi_x(SPI_EEPROM_PP);
        spi_x((addr >> 8) & 0xFF);
        spi_x(addr & 0xFF);
        for (int i = 0; i < n - 1; i++) spi_x(d[i]);
        spi_last(d[n - 1]);
        if (!eep_wait_ready()) return 0;
        addr += n; d += n; len -= n;
    }
    return 1;
}

int plat_save_write(const void *data, int len)
{
    sysSetCardOwner(BUS_OWNER_ARM9);
    if (!eep_write(0, (const u8 *)data, len)) return 0;
    // verify
    static u8 tmp[8192];
    if (len > (int)sizeof(tmp)) return 0;
    eep_read(0, tmp, len);
    return memcmp(tmp, data, len) == 0;
}

int plat_save_read(void *data, int len)
{
    sysSetCardOwner(BUS_OWNER_ARM9);
    eep_read(0, (u8 *)data, len);
    return 1;
}
#endif
