#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "game.h"
#ifndef __NDS__
#include <stdio.h>
#include <stdlib.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

SaveData G;
u32 key_held, key_down, key_rep;
int touch_down, touch_held, tx, ty;
u32 frame_count;
int top_dirty = 1, bot_dirty = 1;
static u32 rep_timer;
static u32 rng_state = 0x12345678;

const ItemData items[NUM_ITEMS] = {
    { "Datenkapsel", "Fängt wilde Vexel. Standard.", 200 },
    { "Superkapsel", "Bessere Fangquote als die Datenkapsel.", 600 },
    { "Hyperkapsel", "Sehr hohe Fangquote.", 1200 },
    { "Omegakapsel", "Fängt jeden Vexel. Prototyp.", 0 },
    { "Reparaturkit", "Stellt 30 KP wieder her.", 300 },
    { "Superkit", "Stellt 80 KP wieder her.", 700 },
    { "Maxkit", "Stellt alle KP wieder her.", 1500 },
    { "Reboot", "Belebt einen Vexel mit 50% KP wieder.", 1500 },
    { "Antivirus", "Heilt jedes Statusproblem (auch Schlaf und Frost).", 250 },
    { "Energiezelle", "Füllt die AP aller Attacken auf.", 800 },
    { "Schutzfeld", "Hält 100 Schritte lang schwache wilde Vexel fern.", 350 },
    { "Megaschutz", "Hält 250 Schritte lang schwache wilde Vexel fern.", 650 },
};

const char *const status_short[7] = { "", "BRD", "GLT", "VIR", "SLF", "FRS", "TOX" };

const ItemData held_items[NUM_HELD] = {
    { "-", "", 0 },
    { "Nanoreparatur", "Getragen: heilt jede Runde 1/16 KP.", 4000 },
    { "Datenbeere", "Getragen: heilt einmal 1/4 KP, wenn die KP unter die Hälfte fallen.", 300 },
    { "Klarbeere", "Getragen: heilt einmal jedes Statusproblem und Verwirrung.", 300 },
    { "Notfallchip", "Getragen: übersteht bei vollen KP einmal einen K.O.-Treffer.", 3000 },
    { "Kraftmodul", "Getragen: physische Attacken +20%.", 2500 },
    { "Technikmodul", "Getragen: Technik-Attacken +20%.", 2500 },
    { "Überlastkern", "Getragen: Schaden +30%, kostet aber 1/10 KP pro Angriff.", 6000 },
    { "Zielchip", "Getragen: höhere Volltreffer-Chance.", 2000 },
    { "Turbochip", "Getragen: greift manchmal zuerst an.", 2000 },
    { "Saugmodul", "Getragen: heilt 1/8 des verursachten Schadens.", 3000 },
    { "Stasiskern", "Getragen: verhindert die Entwicklung.", 500 },
    { "Lernchip", "Getragen: +50% Erfahrung.", 5000 },
    { "Creditchip", "Getragen: doppeltes Preisgeld gegen Trainer.", 4000 },
    { "Nebelkapsel", "Getragen: Flucht vor wilden Vexel gelingt immer.", 800 },
    { "Klimakern", "Getragen: selbst gerufenes Wetter hält 8 statt 5 Runden.", 2000 },
    { "Panzerweste", "Getragen: erlittener Schaden -15%.", 3500 },
};

// natures: +10% on stat n/4, -10% on stat n%4 (Angriff, Verteidigung, Technik, Initiative)
const char *const nature_names[NUM_NATURES] = {
    "Ausgeglichen", "Draufgängerisch", "Wagemutig", "Ungestüm",
    "Standfest", "Gelassen", "Bedacht", "Träge",
    "Clever", "Behutsam", "Logisch", "Grüblerisch",
    "Eilig", "Flink", "Nervös", "Ruhelos",
};
static const char *const nat_stat[4] = { "Angriff", "Verteid.", "Technik", "Initiat." };
const char *nature_desc(int n, char *buf)
{
    if (n / 4 == n % 4) fmt(buf, "%s (neutral)", nature_names[n]);
    else fmt(buf, "%s (+%s -%s)", nature_names[n], nat_stat[n / 4], nat_stat[n % 4]);
    return buf;
}

const char *const phase_names[4] = { "Morgen", "Tag", "Abend", "Nacht" };
int clock_minutes(void) { return plat_minutes(); }
int day_phase(void)
{
    if (G.clock_off) return DAY_DAY;
    int h = plat_minutes() / 60;
    if (h >= 5 && h < 10) return DAY_MORNING;
    if (h >= 10 && h < 18) return DAY_DAY;
    if (h >= 18 && h < 21) return DAY_EVENING;
    return DAY_NIGHT;
}

u32 rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

int rnd_range(int a, int b)
{
    if (b <= a) return a;
    return a + (int)(rnd() % (u32)(b - a + 1));
}

static int fps_val;
u32 prof_mark;
int prof_work, prof_present;
void frame(void)
{
    static u32 last_v, frames_acc, t_end;
    u32 t0 = plat_ticks();
    prof_work = (t0 - t_end) / 3351;   // 0.1 ms units
    if (key_held & K_SELECT) {
        char b[40];
        fmt(b, "FPS %d W%d P%d", fps_val, prof_work, prof_present);
        fill(fb_top, 150, 0, 106, 11, C_BLACK);
        draw_text(fb_top, 154, 1, b, C_GREEN);
        top_dirty = 1;
    }
    u32 tp = plat_ticks();
    plat_present(top_dirty, bot_dirty);
    t_end = plat_ticks();
    prof_present = (t_end - tp) / 3351;
    frames_acc++;
    u32 v = plat_vblanks();
    if (v - last_v >= 60) { fps_val = frames_acc * 60 / (v - last_v); frames_acc = 0; last_v = v; }
    top_dirty = bot_dirty = 0;
    frame_count++;
    G.playtime++;
    music_tick();
    u32 k = plat_keys();
    key_down = k & ~key_held;
    key_held = k;
    key_rep = key_down;
    if (k & (K_UP | K_DOWN | K_LEFT | K_RIGHT)) {
        if (key_down & (K_UP | K_DOWN | K_LEFT | K_RIGHT)) rep_timer = 0;
        rep_timer++;
        if (rep_timer > 18 && (rep_timer % 5) == 0) key_rep |= k & (K_UP | K_DOWN | K_LEFT | K_RIGHT);
    }
    int x, y;
    int t = plat_touch(&x, &y);
    touch_down = t && !touch_held;
    touch_held = t;
    if (t) { tx = x; ty = y; }
    rnd();
}

void wait_frames(int n)
{
    while (n-- > 0) frame();
}

int touched(int x, int y, int w, int h)
{
    return touch_down && tx >= x && tx < x + w && ty >= y && ty < y + h;
}

// ------------------------------------------------------------------ flags & dex
int flag(int f) { return (G.flags[f >> 3] >> (f & 7)) & 1; }
void set_flag(int f) { G.flags[f >> 3] |= 1 << (f & 7); }
void clr_flag(int f) { G.flags[f >> 3] &= ~(1 << (f & 7)); }
// Vexikon bits: species 0..1023 in seen/caught, the rest in seen_hi/caught_hi
static u8 *seen_byte(int sp) { return sp < 1024 ? &G.seen[sp >> 3] : sp < 1536 ? &G.seen_hi[(sp - 1024) >> 3] : &G.seen_x[(sp - 1536) >> 3]; }
static u8 *caught_byte(int sp) { return sp < 1024 ? &G.caught[sp >> 3] : sp < 1536 ? &G.caught_hi[(sp - 1024) >> 3] : &G.caught_x[(sp - 1536) >> 3]; }
void mark_seen(int sp) { *seen_byte(sp) |= 1 << (sp & 7); }
void mark_caught(int sp) { mark_seen(sp); *caught_byte(sp) |= 1 << (sp & 7); }
int is_seen(int sp) { return (*seen_byte(sp) >> (sp & 7)) & 1; }
int is_caught(int sp) { return (*caught_byte(sp) >> (sp & 7)) & 1; }
int count_caught(void)
{
    int n = 0;
    for (int i = 0; i < NUM_SPECIES; i++) n += is_caught(i);
    return n;
}
int count_seen(void)
{
    int n = 0;
    for (int i = 0; i < NUM_SPECIES; i++) n += is_seen(i);
    return n;
}

int give_mon(const Mon *m)
{
    mark_caught(m->species);
    if (G.party_n < PARTY_MAX) { G.party[G.party_n++] = *m; return 0; }
    if (G.box_n < BOX_MAX) { G.box[G.box_n++] = *m; return 1; }
    return -1;
}

// ------------------------------------------------------------------ progression
int badge_count(void)
{
    int n = 0;
    for (int i = 0; i < NUM_GYMS; i++) n += flag(F_BADGE_BASE + i);
    return n;
}

// Kai's starter line. Old saves (before random starters) used the classic rule: the next of the three.
// alternative evolution conditions (see tools/gen_mon.py ALT_COND_NAMES)
const char *const alt_cond_names[5] = { "", "nachts", "wenn Angriff > Technik", "tagsüber", "bei Code-Mutation" };
int alt_cond_met(const Mon *m, int cond)
{
    int ph = day_phase();
    switch (cond) {
    case 1: return ph == DAY_NIGHT;
    case 2: return m->atk > m->tec;
    case 3: return ph == DAY_MORNING || ph == DAY_DAY;
    case 4: return ((m->iv[0] + m->iv[1] + m->iv[2] + m->iv[3] + m->iv[4]) & 1) != 0;
    }
    return 0;
}

int rival_line(void)
{
    if (G.rival_starter && G.rival_starter <= NUM_STARTERS) return starter_lines[G.rival_starter - 1];
    return starter_lines[(G.starter + 1) % 3];
}

// Opponent levels grow with the number of badges so every area stays challenging.
int scaled_level(int base, int idx, int boss)
{
    int b = badge_count();
    int lv = 5 + b * 3 + idx + (boss ? 4 : 0);
    if (base >= 50 && lv < base) lv = base;   // post-game areas keep their minimum
    if (lv > 100) lv = 100;
    return lv;
}

// Picks the evolution stage of a line that fits the given level.
int species_for_level(int line, int level)
{
    const LineData *L = &mlines[line];
    int sp = L->first;
    for (int k = 0; k + 1 < L->nstages; k++) {
        int e = species[sp].evo;
        if (e && level >= e + 2) sp++;
        else break;
    }
    return sp;
}

// ------------------------------------------------------------------ creatures
u32 exp_for_level(int lv)
{
    if (lv <= 1) return 0;
    return (u32)lv * lv * lv * 4 / 5;
}

void mon_calc(Mon *m)
{
    const SpeciesData *s = &species[m->species];
    int L = m->level;
    int oldmax = m->maxhp;
    m->maxhp = (2 * s->base[0] + m->iv[0]) * L / 100 + L + 10;
    m->atk = (2 * s->base[1] + m->iv[1]) * L / 100 + 5;
    m->def = (2 * s->base[2] + m->iv[2]) * L / 100 + 5;
    m->tec = (2 * s->base[3] + m->iv[3]) * L / 100 + 5;
    m->spd = (2 * s->base[4] + m->iv[4]) * L / 100 + 5;
    int up = (m->nature % NUM_NATURES) / 4, dn = m->nature % 4;
    if (up != dn) {
        u16 *st[4] = { &m->atk, &m->def, &m->tec, &m->spd };
        *st[up] = *st[up] * 11 / 10;
        *st[dn] = *st[dn] * 9 / 10;
    }
    if (oldmax && m->hp > 0) {
        int nh = m->hp + (m->maxhp - oldmax);
        m->hp = nh < 1 ? 1 : nh;
    }
    if (m->hp > m->maxhp) m->hp = m->maxhp;
}

int mon_nmoves(const Mon *m)
{
    int n = 0;
    for (int i = 0; i < 4; i++) if (m->moves[i] != NO_MOVE) n++;
    return n;
}

int mon_learn_at(const Mon *m, int level, int *out, int maxn)
{
    const LineData *L = &mlines[species[m->species].line];
    int n = 0;
    for (int i = 0; i < L->learn_n && n < maxn; i++) {
        const u16 *e = learn_data[L->learn_ofs + i];
        if (e[0] == level) out[n++] = e[1];
    }
    return n;
}

void mon_create(Mon *m, int sp, int level)
{
    memset(m, 0, sizeof(*m));
    m->species = sp;
    m->level = level;
    for (int i = 0; i < 5; i++) m->iv[i] = rnd() % 32;
    m->nature = rnd() % NUM_NATURES;
    if (rnd() % 512 == 0) m->mflags |= MF_SHINY;
    m->exp = exp_for_level(level);
    for (int i = 0; i < 4; i++) { m->moves[i] = NO_MOVE; m->pp[i] = 0; }
    // last 4 moves learned up to level
    const LineData *L = &mlines[species[sp].line];
    int known[4] = { -1, -1, -1, -1 }, kn = 0;
    for (int i = 0; i < L->learn_n; i++) {
        const u16 *e = learn_data[L->learn_ofs + i];
        if (e[0] > level) break;
        int dup = 0;
        for (int k = 0; k < kn; k++) if (known[k] == e[1]) dup = 1;
        if (dup) continue;
        if (kn < 4) known[kn++] = e[1];
        else { known[0] = known[1]; known[1] = known[2]; known[2] = known[3]; known[3] = e[1]; }
    }
    for (int i = 0; i < kn; i++) { m->moves[i] = known[i]; m->pp[i] = moves[known[i]].pp; }
    mon_calc(m);
    m->hp = m->maxhp;
}

void mon_heal(Mon *m)
{
    mon_calc(m);
    m->hp = m->maxhp;
    m->status = 0;
    for (int i = 0; i < 4; i++) if (m->moves[i] != NO_MOVE) m->pp[i] = moves[m->moves[i]].pp;
}

// ------------------------------------------------------------------ save
#define SAVE_MAGIC 0x5645584C
static u32 checksum(const SaveData *s)
{
    const u8 *p = (const u8 *)s;
    u32 h = 2166136261u;
    for (unsigned i = 0; i < sizeof(SaveData) - 4; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}

int save_game(void)
{
    G.magic = SAVE_MAGIC;
    G.version = 5;
    G.checksum = checksum(&G);
    return plat_save_write(&G, sizeof(G));
}


// Older save layouts. v1 ended right after the box, v2 (champion update) had 10 item slots.
typedef struct {
    u32 magic; u16 version; u8 map, dir, x, y, ret_map, ret_x, ret_y, heal_map, heal_x, heal_y;
    u8 party_n, box_n, starter, textspeed, music_on; u32 money, playtime;
    u8 flags[NUM_FLAGS / 8]; u8 seen[128]; u8 caught[128]; u8 cur_gym; u8 pad2[3];
    u16 items[10]; Mon party[PARTY_MAX]; Mon box[BOX_MAX];
    u8 rematch[32]; u16 defenses, chal_wins; u8 pad3[28]; u32 checksum;
} SaveV2;

_Static_assert(offsetof(SaveV2, items) == offsetof(SaveData, items), "save prefix");
_Static_assert(sizeof(SaveData) <= 8192, "save too big");
static union { SaveData n; SaveV2 o; u8 raw[sizeof(SaveData)]; } tmp;

static u32 fnv(const u8 *p, unsigned n)
{
    u32 h = 2166136261u;
    for (unsigned i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}

// fills fields a fresh game needs
void save_init_new_fields(SaveData *s)
{
    for (int i = 0; i < 3; i++) s->eggs[i].species = 0xFFFF;
    s->follow = 1;
    s->chal_target = rnd_range(750, 2500);
}

// returns the layout version found in tmp (0 = no valid save)
static int detect_save(void)
{
    plat_save_read(tmp.raw, sizeof(tmp.raw));
    if (tmp.n.magic != SAVE_MAGIC) return 0;
    if (tmp.n.version == 5 && tmp.n.checksum == checksum(&tmp.n)) return 5;
    if (tmp.n.version == 4) {   // v6 layout: ended right before seen_x
        unsigned at = (offsetof(SaveData, seen_x) + 3) & ~3u;
        u32 stored;
        memcpy(&stored, tmp.raw + at, 4);
        if (stored == fnv(tmp.raw, at)) return 4;
    }
    if (tmp.n.version == 3) {   // v5 layout: ended right before seen_hi (checksum 4-byte aligned)
        unsigned at = (offsetof(SaveData, seen_hi) + 3) & ~3u;
        u32 stored;
        memcpy(&stored, tmp.raw + at, 4);
        if (stored == fnv(tmp.raw, at)) return 3;
    }
    if (tmp.o.version == 2 && tmp.o.checksum == fnv(tmp.raw, sizeof(SaveV2) - 4)) return 2;
    if (tmp.o.version == 1) {
        u32 stored;
        memcpy(&stored, tmp.raw + offsetof(SaveV2, rematch), 4);
        if (stored == fnv(tmp.raw, offsetof(SaveV2, rematch))) return 1;
    }
    return 0;
}

int save_exists(void) { return detect_save() != 0; }

int load_game(void)
{
    int v = detect_save();
    if (!v) return 0;
    if (v >= 3) {
        if (v == 3) memset(tmp.raw + offsetof(SaveData, seen_hi), 0, sizeof(SaveData) - offsetof(SaveData, seen_hi));
        if (v == 4) memset(tmp.raw + offsetof(SaveData, seen_x), 0, sizeof(SaveData) - offsetof(SaveData, seen_x));
        memcpy(&G, &tmp.n, sizeof(G));
        return 1;
    }
    SaveV2 o = tmp.o;
    if (v == 1) memset((u8 *)&o + offsetof(SaveV2, rematch), 0, sizeof(SaveV2) - offsetof(SaveV2, rematch));
    memset(&G, 0, sizeof(G));
    memcpy(&G, &o, offsetof(SaveV2, items));
    memcpy(G.items, o.items, sizeof(o.items));
    memcpy(G.party, o.party, sizeof(o.party));
    memcpy(G.box, o.box, sizeof(o.box));
    memcpy(G.rematch, o.rematch, sizeof(o.rematch));
    G.defenses = o.defenses; G.chal_wins = o.chal_wins;
    save_init_new_fields(&G);
    return 1;
}

// ------------------------------------------------------------------ misc formatting
void fmt(char *out, const char *f, ...)
{
    va_list ap;
    va_start(ap, f);
    char *o = out;
    while (*f) {
        if (*f == '%' && f[1]) {
            f++;
            if (*f == 'd') {
                int v = va_arg(ap, int);
                char tmp[16]; int n = 0;
                if (v < 0) { *o++ = '-'; v = -v; }
                do { tmp[n++] = '0' + v % 10; v /= 10; } while (v);
                while (n) *o++ = tmp[--n];
            } else if (*f == 's') {
                const char *s = va_arg(ap, const char *);
                while (*s) *o++ = *s++;
            } else *o++ = *f;
            f++;
        } else *o++ = *f++;
    }
    *o = 0;
    va_end(ap);
}

void str_cat_num(char *buf, int v)
{
    fmt(buf + strlen(buf), "%d", v);
}

// ------------------------------------------------------------------ snapshots
static u16 snap_top[SCR_W * SCR_H];
static u16 snap_bot[SCR_W * SCR_H];
void top_snapshot(void) { memcpy(snap_top, fb_top, sizeof(snap_top)); }
void top_restore(void) { memcpy(fb_top, snap_top, sizeof(snap_top)); top_dirty = 1; }
void bot_snapshot(void) { memcpy(snap_bot, fb_bot, sizeof(snap_bot)); }
void bot_restore(void) { memcpy(fb_bot, snap_bot, sizeof(snap_bot)); bot_dirty = 1; }

void fade_out(int which)
{
    for (int i = 0; i < 10; i++) {
        if (which & 1) { darken(fb_top, 0, 0, SCR_W, SCR_H, 5); top_dirty = 1; }
        if (which & 2) { darken(fb_bot, 0, 0, SCR_W, SCR_H, 5); bot_dirty = 1; }
        frame();
    }
    if (which & 1) { fill(fb_top, 0, 0, SCR_W, SCR_H, C_BLACK); top_dirty = 1; }
    if (which & 2) { fill(fb_bot, 0, 0, SCR_W, SCR_H, C_BLACK); bot_dirty = 1; }
}

// ------------------------------------------------------------------ backgrounds
static u16 bot_bg_cache[SCR_W * SCR_H], bot_bg_dark[SCR_W * SCR_H];
static int bot_bg_ok;
static void build_bottom_bg(u16 *fb)
{
    gradient_v(fb, 0, 0, SCR_W, SCR_H, RGB(3, 3, 9), RGB(1, 1, 4));
    for (int y = 0; y < SCR_H; y += 16) fill_blend(fb, 0, y, SCR_W, 1, RGB(6, 10, 22), 8);
    for (int x = 0; x < SCR_W; x += 16) fill_blend(fb, x, 0, 1, SCR_H, RGB(6, 10, 22), 8);
    for (int i = 0; i < 6; i++) {
        int x = (i * 53 + 17) % 240, y = (i * 37 + 11) % 180;
        fill_blend(fb, x, y, 3, 3, C_CYAN, 4);
    }
}

static void ensure_bottom_bg(void)
{
    if (bot_bg_ok) return;
    build_bottom_bg(bot_bg_cache);
    memcpy(bot_bg_dark, bot_bg_cache, sizeof(bot_bg_dark));
    darken(bot_bg_dark, 0, 0, SCR_W, SCR_H, 3);
    bot_bg_ok = 1;
}

void draw_bottom_bg(void)
{
    ensure_bottom_bg();
    memcpy(fb_bot, bot_bg_cache, sizeof(bot_bg_cache));
    bot_dirty = 1;
}

void draw_bottom_bg_dark(void)
{
    ensure_bottom_bg();
    memcpy(fb_bot, bot_bg_dark, sizeof(bot_bg_dark));
    bot_dirty = 1;
}

static u16 bg_cache[8][SCR_W * SCR_H];
static int bg_cache_style = -1;
static void draw_battle_bg_raw(u16 *fb, int style, int t);
void draw_battle_bg(u16 *fb, int style, int t)
{
    style = (style & 0x80) | ((style & 0x7F) % 7);
    if (bg_cache_style != style) {
        for (int p = 0; p < 8; p++) draw_battle_bg_raw(bg_cache[p], style, p * 4);
        bg_cache_style = style;
    }
    memcpy(fb, bg_cache[(t >> 2) & 7], sizeof(bg_cache[0]));
}

static void draw_battle_bg_raw(u16 *fb, int style, int t)
{
    int no_platforms = style & 0x80;
    style &= 0x7F;
    static const u16 sky0[] = { RGB(4, 1, 10), RGB(1, 6, 8), RGB(1, 4, 14), RGB(3, 1, 8), RGB(10, 2, 8), RGB(14, 14, 18), RGB(0, 0, 2) };
    static const u16 sky1[] = { RGB(24, 4, 18), RGB(4, 20, 16), RGB(6, 18, 28), RGB(10, 4, 20), RGB(31, 14, 6), RGB(28, 28, 31), RGB(10, 2, 18) };
    static const u16 grid[] = { RGB(0, 28, 31), RGB(4, 31, 14), RGB(4, 26, 31), RGB(22, 10, 31), RGB(31, 8, 22), RGB(31, 4, 8), RGB(20, 6, 31) };
    static const u16 sunc[] = { RGB(31, 26, 4), RGB(20, 31, 20), RGB(26, 31, 31), RGB(26, 16, 31), RGB(31, 22, 6), RGB(31, 6, 10), RGB(31, 31, 31) };
    style %= 7;
    int hz = 100;
    gradient_v(fb, 0, 0, SCR_W, hz, sky0[style], sky1[style]);
    // stars
    if (style == 0 || style == 3 || style == 6)
        for (int i = 0; i < 40; i++) {
            int x = (i * 97 + 13) % 256, y = (i * 61 + 7) % (hz - 30);
            pset(fb, x, y, ((i + t / 20) % 5) ? RGB(20, 20, 28) : C_WHITE);
        }
    // sun
    int scx = 128, scy = hz - 18, r = 30;
    for (int y = -r; y <= 0; y++) {
        int yy = scy + y;
        int stripe = (y > -14) && (((y + (t / 4)) & 7) < 2 + (y + 14) / 5);
        if (stripe) continue;
        int w = 0;
        while (w * w + y * y <= r * r) w++;
        u16 c = lerp_col(sunc[style], RGB(31, 6, 20), -y > r ? 0 : r + y, r);
        fill(fb, scx - w, yy, w * 2, 1, c);
    }
    // skyline silhouettes
    for (int x = 0; x < SCR_W; x += 6) {
        int h = 6 + ((x * 7919) >> 3) % 22;
        if (style == 1) h = 4 + ((x * 31) % 13);
        if (style == 4 || style == 6) h = 2 + ((x * 17) % 7);
        fill(fb, x, hz - h, 6, h, lerp_col(sky0[style], C_BLACK, 8, 16));
        if (style == 0 && ((x * 13) % 5) == 0) pset(fb, x + 2, hz - h + 3, C_YELLOW);
    }
    // floor
    u16 f0 = lerp_col(sky0[style], C_BLACK, 6, 16);
    gradient_v(fb, 0, hz, SCR_W, SCR_H - hz, f0, lerp_col(grid[style], C_BLACK, 13, 16));
    hline(fb, 0, hz, SCR_W, grid[style]);
    // horizontal grid lines (perspective)
    int off = (t / 2) % 16;
    for (int k = 0; k < 18; k++) {
        int z = k * 16 + 16 - off;       // depth
        int y = hz + (92 * 16) / (z + 8) - 8;
        if (y <= hz || y >= SCR_H) continue;
        fill_blend(fb, 0, y, SCR_W, 1, grid[style], 4 + (y - hz) / 10);
    }
    for (int k = -12; k <= 12; k++) {
        int x0 = 128 + k * 10;
        int x1 = 128 + k * 60;
        line(fb, x0, hz, x1, SCR_H, lerp_col(grid[style], f0, 9, 16));
    }
    // battle platforms
    for (int k = 0; k < 2 && !no_platforms; k++) {
        int cx = k ? 64 : 192, cy = k ? 172 : 92, rx = k ? 56 : 46, ry = k ? 9 : 7;
        for (int y = -ry; y <= ry; y++) {
            int w = rx;
            int yy2 = y * y * rx * rx / (ry * ry);
            while (w > 0 && w * w > rx * rx - yy2) w--;
            fill_blend(fb, cx - w, cy + y, w * 2, 1, RGB(0, 0, 0), 9);
            if (y == -ry || y == ry || w < 3) continue;
            pset(fb, cx - w, cy + y, C_CYAN); pset(fb, cx + w - 1, cy + y, C_CYAN);
        }
    }
}

// ------------------------------------------------------------------ dialog
void message_box_draw(u16 *fb, const char *speaker, const char *text, int chars, int y)
{
    panel(fb, 4, y, 248, 54, C_CYAN);
    if (speaker && speaker[0]) {
        int w = text_width(speaker) + 12;
        panel_solid(fb, 10, y - 9, w, 13, RGB(4, 6, 14), C_MAGENTA);
        draw_text_sh(fb, 16, y - 7, speaker, C_YELLOW, RGB(2, 2, 4));
    }
    draw_text_wrap(fb, 12, y + 7, 232, text, C_TEXT, RGB(2, 3, 9), chars, 11);
}

static void say_impl(const char *speaker, const char *text, int wait_end)
{
    char page[400];
    const char *p = text;
    LOG("[say f%u] %s: %s\n", frame_count, speaker ? speaker : "-", text);
    plat_light_hole(0, 122, 256, 70);
#ifndef __NDS__
    if (getenv("VX_AUTOTEXT")) { top_snapshot(); message_box_draw(fb_top, speaker, text, -1, 134); frame(); if (wait_end) top_restore(); return; }
#endif
    top_snapshot();
    while (*p) {
        int n = 0;
        while (*p && *p != '|' && n < (int)sizeof(page) - 1) page[n++] = *p++;
        page[n] = 0;
        if (*p == '|') p++;
        int total = text_glyph_count(page);
        int shown = 0;
        int speed = G.textspeed == 0 ? 1 : (G.textspeed == 1 ? 2 : 4);
        int last_page = (*p == 0);
        int last_shown = -1, last_blink = -1;
        for (;;) {
            if (shown < total) {
                shown += speed;
                if ((key_down & (K_A | K_B)) || touch_down) shown = total;
                if (shown > total) shown = total;
            } else {
                if (!wait_end && last_page) break;
                if ((key_down & (K_A | K_B)) || touch_down) { sfx(SFX_SELECT); break; }
            }
            int blink = shown >= total && ((frame_count / 16) & 1);
            if (shown != last_shown || blink != last_blink) {
                top_restore();
                message_box_draw(fb_top, speaker, page, shown, 134);
                if (blink) draw_text(fb_top, 240, 178, "▼", C_CYAN);
                top_dirty = 1;
                last_shown = shown; last_blink = blink;
            }
            frame();
        }
    }
    if (wait_end) top_restore();
}

void say(const char *speaker, const char *text) { say_impl(speaker, text, 1); }
void say_keep(const char *speaker, const char *text) { say_impl(speaker, text, 0); }

int ask_yesno(const char *speaker, const char *text)
{
    say_keep(speaker, text);
    static const char *const yn[] = { "Ja", "Nein" };
    bot_snapshot();
    int r = menu_list(NULL, yn, 2, 0);
    bot_restore();
    top_restore();
    return r == 0;
}

int menu_list(const char *title, const char *const *opts, int n, int cur)
{
    const int vis = 7;
    int scroll = 0;
    key_down = key_rep = 0;   // never act on a press that opened this menu
    touch_down = 0;
    bot_snapshot();
    int redraw = 1;
    for (;;) {
        if (key_rep & K_UP) { cur = (cur + n - 1) % n; redraw = 1; sfx(SFX_SELECT); }
        if (key_rep & K_DOWN) { cur = (cur + 1) % n; redraw = 1; sfx(SFX_SELECT); }
        if (key_down & K_A) { sfx(SFX_CONFIRM); bot_restore(); LOG("[menu] %s -> %s\n", title ? title : "-", opts[cur]); return cur; }
        if (key_down & K_B) { sfx(SFX_CANCEL); bot_restore(); return -1; }
        if (cur < scroll) scroll = cur;
        if (cur >= scroll + vis) scroll = cur - vis + 1;
        int y0 = title ? 26 : 12;
        if (touch_down) {
            for (int i = 0; i < vis && scroll + i < n; i++)
                if (touched(40, y0 + i * 23, 176, 21)) { sfx(SFX_CONFIRM); bot_restore(); return scroll + i; }
            if (n > vis && touched(222, y0, 30, 60)) { scroll = scroll > 0 ? scroll - 1 : 0; cur = scroll; redraw = 1; }
            if (n > vis && touched(222, y0 + 100, 30, 60)) { if (scroll + vis < n) scroll++; cur = scroll; redraw = 1; }
        }
        if (redraw) {
            bot_restore();
            darken(fb_bot, 0, 0, SCR_W, SCR_H, 6);
            if (title) {
                panel_solid(fb_bot, 40, 4, 176, 18, RGB(4, 4, 12), C_MAGENTA);
                draw_text_sh(fb_bot, 128 - text_width(title) / 2, 8, title, C_YELLOW, C_BLACK);
            }
            for (int i = 0; i < vis && scroll + i < n; i++)
                button(fb_bot, 40, y0 + i * 23, 176, 21, opts[scroll + i], C_CYAN, scroll + i == cur);
            if (n > vis) {
                if (scroll > 0) draw_text(fb_bot, 232, y0 + 20, "▲", C_CYAN);
                if (scroll + vis < n) draw_text(fb_bot, 232, y0 + 140, "▼", C_CYAN);
            }
            bot_dirty = 1;
            redraw = 0;
        }
        frame();
    }
}
