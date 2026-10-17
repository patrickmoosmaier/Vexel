// Tiny 3-voice chiptune sequencer driven once per frame.
#include <string.h>
#include "game.h"

typedef struct { u8 tempo; u8 loop; const char *ch[3]; } Song;

#define DR4 "k:4 h:2 h:2 s:4 h:2 h:2 "
#define DR8 "k:2 h:2 k:2 h:2 s:2 h:2 k:2 h:2 "
#define DRF "k:2 h:1 h:1 s:2 h:1 k:1 k:2 h:1 h:1 s:2 h:2 "
#define B8(n, o, p) n #o ":2 " n #p ":2 " n #o ":2 " n #p ":2 " n #o ":2 " n #p ":2 " n #o ":2 " n #p ":2 "
#define B16(n, o, p) B8(n, o, p) B8(n, o, p)
#define BF(n, o, p) n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 " n #o ":1 " n #p ":1 "

static const Song songs[] = {
    { 0, 0, { "", "", "" } },
    // TITLE
    { 7, 1, {
        "E5:4 A4:2 C5:2 E5:4 D5:2 C5:2 C5:6 A4:2 F4:4 A4:4 G4:4 C5:2 E5:2 G5:4 E5:4 D5:6 B4:2 G4:4 B4:4 "
        "A5:4 G5:2 E5:2 A5:4 C6:4 A5:6 G5:2 F5:4 E5:4 E5:4 D5:2 C5:2 E5:4 G5:4 D5:8 B4:4 r:4 ",
        B8("A", 2, 3) B8("F", 2, 3) B8("C", 3, 4) B8("G", 2, 3) B8("A", 2, 3) B8("F", 2, 3) B8("C", 3, 4) B8("G", 2, 3),
        DR4 DR4 DR4 DR4 DR4 DR4 DR4 DR4 } },
    // TOWN
    { 8, 1, {
        "E5:4 G5:4 C6:4 G5:4 A5:4 E5:4 C5:8 F5:4 A5:4 C6:4 A5:4 G5:12 r:4 "
        "E5:2 F5:2 G5:4 E5:4 C5:4 A4:4 C5:4 E5:8 F5:4 E5:4 D5:4 C5:4 D5:12 r:4 ",
        "C3:4 G3:4 C3:4 G3:4 A2:4 E3:4 A2:4 E3:4 F2:4 C3:4 F2:4 C3:4 G2:4 D3:4 G2:4 D3:4 "
        "C3:4 G3:4 C3:4 G3:4 A2:4 E3:4 A2:4 E3:4 F2:4 C3:4 F2:4 C3:4 G2:4 D3:4 G2:4 D3:4 ",
        "k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:4 "
        "k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:4 k:4 h:4 s:4 h:2 h:2 " } },
    // ROUTE
    { 6, 1, {
        "D5:2 F5:2 A5:4 G5:2 F5:2 E5:4 F5:2 E5:2 D5:4 A4:8 D5:2 F5:2 A5:4 C6:4 A5:4 G5:8 E5:8 "
        "F5:2 G5:2 A5:4 A#5:4 A5:4 G5:2 F5:2 E5:4 C5:8 D5:2 E5:2 F5:2 E5:2 D5:4 C#5:4 D5:16 ",
        B8("D", 2, 3) B8("A#", 1, 2) B8("C", 2, 3) B8("A", 1, 2) B8("D", 2, 3) B8("A#", 1, 2) B8("A", 1, 2) B8("D", 2, 3),
        DR8 DR8 DR8 DR8 DR8 DR8 DR8 DR8 } },
    // BATTLE
    { 5, 1, {
        "E5:2 r:1 E5:1 G5:2 E5:2 B5:4 A5:2 G5:2 F#5:2 G5:2 A5:2 F#5:2 D5:4 B4:4 "
        "E5:2 r:1 E5:1 G5:2 E5:2 B5:4 D6:2 C6:2 B5:2 A5:2 G5:2 A5:2 B5:8 "
        "C6:4 B5:2 A5:2 G5:4 E5:4 A5:4 G5:2 F#5:2 E5:4 D5:4 C5:2 D5:2 E5:4 F#5:4 G5:4 F#5:2 G5:2 A5:4 B5:8 r:4 ",
        BF("E", 2, 3) BF("D", 2, 3) BF("E", 2, 3) BF("G", 2, 3) BF("C", 2, 3) BF("A", 1, 2) BF("C", 2, 3) BF("B", 1, 2),
        DRF DRF DRF DRF DRF DRF DRF DRF } },
    // BOSS
    { 5, 1, {
        "C5:2 D#5:2 G5:2 F#5:2 G5:4 D#5:4 C5:2 D#5:2 G5:2 A#5:2 G#5:4 G5:4 "
        "C6:2 A#5:2 G#5:2 G5:2 F5:2 D#5:2 D5:2 C5:2 B4:4 D5:4 G5:8 "
        "C5:1 D#5:1 G5:1 C6:1 D#6:4 D6:4 C6:2 A#5:2 G#5:4 G5:4 D#5:4 F5:2 G5:2 G#5:4 B5:4 C6:8 ",
        BF("C", 2, 3) BF("C", 2, 3) BF("G#", 1, 2) BF("G", 1, 2) BF("C", 2, 3) BF("A#", 1, 2) BF("G#", 1, 2) BF("G", 1, 2),
        DRF DRF DRF DRF DRF DRF DRF DRF } },
    // VICTORY
    { 6, 0, { "C5:2 E5:2 G5:2 C6:6 A#5:2 C6:2 D6:2 C6:12 r:8 ", "C3:4 G3:4 C3:4 A#2:4 C3:12 r:8 ", "k:4 s:4 k:4 s:4 k:2 s:2 s:2 s:6 r:8 " } },
    // CAVE
    { 10, 1, {
        "A4:8 E5:8 D5:4 C5:4 B4:8 A4:8 G4:4 B4:4 C5:16 r:8 E5:8 F5:4 E5:4 D5:4 B4:4 A4:16 r:16 ",
        "A2:16 A2:16 G2:16 G2:16 F2:16 F2:16 E2:16 E2:16 ",
        "k:8 h:8 k:8 h:8 k:8 h:8 k:8 h:8 k:8 h:8 k:8 h:8 k:8 h:8 k:8 h:4 h:4 " } },
    // TOWER
    { 6, 1, {
        "A4:2 A4:2 C5:2 A4:2 D#5:4 D5:4 A4:2 A4:2 C5:2 A4:2 G5:4 F5:4 "
        "E5:2 F5:2 E5:2 D5:2 C5:4 B4:4 A4:8 G#4:8 ",
        B8("A", 1, 2) B8("A", 1, 2) B8("F", 1, 2) B8("E", 1, 2),
        DR8 DR8 DR8 DR8 } },
    // HEAL
    { 6, 0, { "C5:2 E5:2 G5:2 C6:4 G5:2 C6:8 r:4 ", "C3:4 G3:4 C4:8 r:4 ", "r:24 " } },
    // EVOLVE
    { 4, 1, { "C5:2 E5:2 G5:2 C6:2 D5:2 F#5:2 A5:2 D6:2 E5:2 G#5:2 B5:2 E6:2 F#5:2 A#5:2 C#6:2 F#6:2 ",
              "C3:8 D3:8 E3:8 F#3:8 ", "h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 h:2 " } },
    // FINAL
    { 4, 1, {
        "A5:2 E5:2 A5:2 C6:2 B5:2 G5:2 E5:2 G5:2 A5:2 E5:2 A5:2 C6:2 D6:4 B5:4 "
        "F5:2 A5:2 C6:2 F6:2 E6:2 C6:2 G5:2 E5:2 D5:2 F5:2 A5:2 D6:2 E6:8 "
        "A6:4 G6:2 E6:2 C6:4 A5:4 F6:4 E6:2 C6:2 A5:4 F5:4 G5:2 A5:2 B5:2 C6:2 D6:4 E6:4 E6:16 ",
        BF("A", 1, 2) BF("G", 1, 2) BF("F", 1, 2) BF("E", 1, 2) BF("A", 1, 2) BF("F", 1, 2) BF("G", 1, 2) BF("E", 1, 2),
        DRF DRF DRF DRF DRF DRF DRF DRF } },
    // ENDING
    { 9, 1, {
        "E5:4 A4:2 C5:2 E5:4 D5:2 C5:2 C5:6 A4:2 F4:4 A4:4 G4:4 C5:2 E5:2 G5:4 E5:4 D5:6 B4:2 G4:4 B4:4 "
        "A5:8 E5:8 F5:8 C5:8 G5:8 E5:8 G5:12 r:4 ",
        "A2:8 E3:8 F2:8 C3:8 C3:8 G3:8 G2:8 D3:8 A2:8 E3:8 F2:8 C3:8 C3:8 G3:8 G2:8 D3:8 ",
        "k:8 s:8 k:8 s:8 k:8 s:8 k:8 s:8 k:8 s:8 k:8 s:8 k:8 s:8 k:8 s:8 " } },
};

typedef struct { const char *p; int left; int note_frames; int vol; int freq; } Chan;
static Chan ch[3];
static int cur_song, tick_timer;
static int sfx_t, sfx_id;

static int note_freq(const char **pp, int *is_rest, int *drum)
{
    const char *p = *pp;
    static const int semis[7] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    *is_rest = 0; *drum = 0;
    int f = 0;
    if (*p == 'r') { *is_rest = 1; p++; }
    else if (*p == 'k' || *p == 's' || *p == 'h') { *drum = *p; p++; }
    else {
        int s = semis[*p - 'A']; p++;
        if (*p == '#') { s++; p++; }
        int oct = *p - '0'; p++;
        int n = s + (oct + 1) * 12;   // MIDI
        // freq = 440 * 2^((n-69)/12)
        static const int base[12] = { 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 }; // octave 8
        int o = n / 12 - 1;
        f = base[n % 12];
        for (int k = o; k < 8; k++) f >>= 1;
        if (f < 20) f = 20;
    }
    *pp = p;
    return f;
}

static void chan_next(int i)
{
    Chan *c = &ch[i];
    while (*c->p == ' ') c->p++;
    if (!*c->p) {
        if (!songs[cur_song].loop) { c->left = 999999; plat_tone_off(i); return; }
        c->p = songs[cur_song].ch[i];
        while (*c->p == ' ') c->p++;
        if (!*c->p) { c->left = 999999; return; }
    }
    int rest, drum;
    int f = note_freq(&c->p, &rest, &drum);
    int len = 1;
    if (*c->p == ':') { c->p++; len = 0; while (*c->p >= '0' && *c->p <= '9') len = len * 10 + (*c->p++ - '0'); }
    c->left = len;
    c->note_frames = 0;
    if (sfx_t > 0 && (i == 0 || (i == 2 && sfx_id >= SFX_HIT))) return;
    if (!G.music_on && cur_song != MUS_NONE) { plat_tone_off(i); return; }
    if (rest) { plat_tone_off(i); return; }
    if (drum) {
        int nf = drum == 'k' ? 180 : drum == 's' ? 3000 : 14000;
        c->vol = drum == 'h' ? 26 : 60;
        plat_tone(2, nf, c->vol, 0);
        c->freq = -1;
        return;
    }
    c->freq = f;
    c->vol = i == 0 ? 52 : 46;
    plat_tone(i, f, c->vol, i == 0 ? 2 : 4);
}

void music_play(int song)
{
    if (song == cur_song && song != MUS_VICTORY && song != MUS_HEAL) return;
    cur_song = song;
    for (int i = 0; i < 3; i++) { ch[i].p = songs[song].ch[i]; ch[i].left = 0; plat_tone_off(i); }
    tick_timer = 0;
    if (song == MUS_NONE) for (int i = 0; i < 3; i++) ch[i].left = 999999;
}

void music_tick(void)
{
    if (sfx_t > 0) {
        sfx_t--;
        static const int sweep[][4] = {
            { 1760, 0, 2, 3 }, { 1320, 1760, 2, 6 }, { 660, 440, 2, 6 }, { 0, 0, 0, 0 },
        };
        (void)sweep;
        int f = 0;
        switch (sfx_id) {
        case SFX_SELECT: f = 1800; break;
        case SFX_CONFIRM: f = sfx_t > 3 ? 1320 : 1980; break;
        case SFX_CANCEL: f = sfx_t > 3 ? 880 : 590; break;
        case SFX_LEVEL: f = 880 + (12 - sfx_t) * 90; break;
        case SFX_HEAL: f = 660 + ((12 - sfx_t) % 4) * 220; break;
        case SFX_SAVE: f = 1000 + (sfx_t & 2) * 400; break;
        case SFX_CATCH: f = 2000 - sfx_t * 60; break;
        case SFX_SHAKE: f = sfx_t & 2 ? 300 : 220; break;
        case SFX_ENCOUNTER: f = 400 + (sfx_t % 6) * 160; break;
        case SFX_BUMP: f = 110; break;
        default: break;
        }
        if (sfx_id >= SFX_HIT && sfx_id <= SFX_WEAK) {
            plat_tone(2, sfx_id == SFX_SUPER ? 1200 : sfx_id == SFX_WEAK ? 6000 : 2500, 90, 0);
        } else if (f) plat_tone(0, f, 60, 2);
        if (sfx_t == 0) { plat_tone_off(0); if (sfx_id >= SFX_HIT) plat_tone_off(2); }
    }
    if (cur_song == MUS_NONE) return;
    if (++tick_timer >= songs[cur_song].tempo) {
        tick_timer = 0;
        for (int i = 0; i < 3; i++) {
            if (--ch[i].left <= 0) chan_next(i);
        }
    }
    // envelopes
    for (int i = 0; i < 3; i++) {
        Chan *c = &ch[i];
        c->note_frames++;
        if (sfx_t > 0 && i == 0) continue;
        if (i == 2 && c->freq == -1 && c->note_frames == 3) plat_tone_off(2);
        if (i == 0 && c->freq > 0 && (c->note_frames & 3) == 0 && c->vol > 24) {
            c->vol -= 4;
            if (G.music_on) plat_tone_vol(0, c->vol);
        }
    }
}

void sfx(int id)
{
    sfx_id = id;
    sfx_t = (id == SFX_SELECT) ? 3 : (id == SFX_LEVEL || id == SFX_HEAL || id == SFX_ENCOUNTER) ? 24 : (id == SFX_SHAKE ? 10 : 7);
}
