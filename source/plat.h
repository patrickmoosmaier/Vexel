// Platform abstraction: DS (libnds) or PC test harness.
#pragma once
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

#define SCR_W 256
#define SCR_H 192

// same bit layout as libnds
#define K_A      (1 << 0)
#define K_B      (1 << 1)
#define K_SELECT (1 << 2)
#define K_START  (1 << 3)
#define K_RIGHT  (1 << 4)
#define K_LEFT   (1 << 5)
#define K_UP     (1 << 6)
#define K_DOWN   (1 << 7)
#define K_R      (1 << 8)
#define K_L      (1 << 9)
#define K_X      (1 << 10)
#define K_Y      (1 << 11)
#define K_TOUCH  (1 << 12)

extern u16 *fb_top;   // 256x192 RGB15 back buffers (bit 15 set = opaque)
extern u16 *fb_bot;

void plat_init(void);
// Present buffers whose dirty flag is set, wait for the next frame.
void plat_present(int top_dirty, int bot_dirty);
u32 plat_keys(void);
int plat_touch(int *x, int *y);
u32 plat_seed(void);
int plat_minutes(void);   // local time of day, 0..1439
u32 plat_vblanks(void);
u32 plat_ticks(void);   // 33.5 MHz bus ticks

// Sound: 3 PSG voices (0,1 square, 2 noise). freq in Hz, vol 0..127, duty 0..7
void plat_tone(int voice, int freq, int vol, int duty);
void plat_tone_off(int voice);
void plat_tone_vol(int voice, int vol);

// Spotlight: darken everything outside a circle (hardware window on DS)
void plat_light(int cx, int cy, int r, int level);
void plat_light_off(void);
void plat_light_hole(int x, int y, int w, int h);  // rectangle excluded from darkening

// Save memory: returns 1 on success
int plat_save_write(const void *data, int len);
int plat_save_read(void *data, int len);
