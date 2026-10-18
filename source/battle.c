// Turn based battle system.
#include <string.h>
#include "game.h"
#ifndef __NDS__
#include <stdio.h>
#include <stdlib.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

static void setup_types(void) {}

static int type_mult4(int mt, int sp)   // returns multiplier * 4 (4 = neutral)
{
    int t1 = species[sp].type1, t2 = species[sp].type2;
    int m = type_eff[mt][t1] * 2;
    if (t2 != t1) m = m * type_eff[mt][t2] / 2;
    return m;
}

// Attribute triangle: Vakzin > Virus > Daten > Vakzin (Frei neutral). Returns percent.
static int attr_mult(int a, int d)
{
    if (a == 3 || d == 3) return 100;
    if ((a == 0 && d == 2) || (a == 2 && d == 1) || (a == 1 && d == 0)) return 125;
    if ((d == 0 && a == 2) || (d == 2 && a == 1) || (d == 1 && a == 0)) return 85;
    return 100;
}

static int abil_of(const Mon *m) { return species[m->species].ability; }
static int abil_kind(const Mon *m) { return abilities[abil_of(m)].kind; }

// ------------------------------------------------------------------ state
typedef struct {
    Mon *m;
    s8 stage[4];  // atk def tec spd
    int disp_hp;  // animated hp
    // volatile battle state (reset on switch)
    u8 confuse, flinch, protect, protect_chain, recharge, charging, seeded, trap, critup, toxic_n, moved;
} Side;

// weather: 0 none, 1 Plasmasonne, 2 Datenregen, 3 Glitchsturm, 4 Kristallhagel
enum { W_NONE, W_SUN, W_RAIN, W_SAND, W_HAIL };
static int weather, weather_turns;   // turns 0 = lasts the whole battle
static const char *const weather_names[5] = { "", "Plasmasonne", "Datenregen", "Glitchsturm", "Kristallhagel" };
static void reset_side(Side *s)
{
    memset(s->stage, 0, sizeof(s->stage));
    s->confuse = s->flinch = s->protect = s->protect_chain = s->recharge = s->charging = s->seeded = s->trap = s->critup = s->toxic_n = s->moved = 0;
}

static Side P, E;
static Mon enemy_team[6];
static int enemy_n, enemy_cur;
static int is_trainer, bg_style;
static const char *tr_name;
static int tr_money, tr_boss;
static int sturdy_used[2];
static int t_anim;
static int hide_p, hide_e;
static int shake_e, shake_p;
static int off_px, off_ex;
static int e_scale_override = -1, e_mode = 0, p_mode = 0;

static int part_mask;
static const char *bmsg_text;

static int mon_scale(int sp) { (void)sp; return 3; }

static void draw_info(u16 *fb, Side *s, int x, int y, int is_player)
{
    Mon *m = s->m;
    panel_solid(fb, x, y, 118, is_player ? 40 : 30, RGB(3, 3, 9), is_player ? C_CYAN : C_MAGENTA);
    draw_text_sh(fb, x + 6, y + 4, species[m->species].name, C_WHITE, C_BLACK);
    if (m->mflags & MF_SHINY) draw_text_sh(fb, x + 8 + text_width(species[m->species].name), y + 3, "*", C_YELLOW, C_BLACK);
    char buf[32];
    fmt(buf, "Lv%d", m->level);
    draw_text_sh(fb, x + 112 - text_width(buf), y + 4, buf, C_YELLOW, C_BLACK);
    draw_text(fb, x + 6, y + 17, "KP", C_YELLOW);
    hpbar(fb, x + 20, y + 18, 90, s->disp_hp, m->maxhp);
    if (STATUS(m)) {
        static const u16 sc[] = { 0, RGB(31, 10, 4), RGB(31, 28, 4), RGB(20, 6, 28), RGB(12, 12, 18), RGB(14, 26, 31), RGB(26, 4, 26) };
        fill(fb, x + 86, y + 3, 24, 9, sc[STATUS(m)]);
        draw_text(fb, x + 89, y + 3, status_short[STATUS(m)], C_WHITE);
    }
    if (is_player) {
        fmt(buf, "%d/%d", s->disp_hp, m->maxhp);
        draw_text_sh(fb, x + 110 - text_width(buf), y + 27, buf, C_WHITE, C_BLACK);
        // exp bar
        u32 a = exp_for_level(m->level), b = exp_for_level(m->level + 1);
        int w = b > a ? (int)((m->exp - a) * 60 / (b - a)) : 0;
        if (w > 60) w = 60;
        fill(fb, x + 6, y + 30, 60, 3, RGB(2, 2, 5));
        fill(fb, x + 6, y + 30, w, 3, RGB(6, 20, 31));
    }
}

static u32 last_scene_key = 0xFFFFFFFF;
static void draw_scene(void)
{
    u16 *fb = fb_top;
    t_anim++;
    last_scene_key = 0xFFFFFFFF;
    draw_battle_bg(fb, bg_style, t_anim);
    int bob = ((t_anim / 12) & 1);
    if (!hide_e && E.m) {
        int sc = e_scale_override >= 0 ? e_scale_override : mon_scale(E.m->species);
        int size = 32 * sc;
        int sx = 192 - size / 2 + off_ex + (shake_e ? ((t_anim & 2) ? 3 : -3) : 0);
        int sy = 94 - size + 4 + (sc == 3 ? 0 : 0) - bob;
        if (e_scale_override >= 0) sy = 94 - size / 2 - 24;
        draw_mon(fb, E.m->species, sx, sy, sc, 0, e_mode | SHINY(E.m));
    }
    if (!hide_p && P.m) {
        int sc = mon_scale(P.m->species) + 1;   // closer to camera
        int size = 32 * sc;
        int sx = 64 - size / 2 + off_px + (shake_p ? ((t_anim & 2) ? 3 : -3) : 0);
        int sy = 176 - size + 6 + bob;
        draw_mon_clip(fb, P.m->species, sx, sy, sc, 1, p_mode | SHINY(P.m), 0, SCR_H);
    }
    if (weather) {
        // simple weather particles
        for (int k = 0; k < 36; k++) {
            int x = (k * 53 + t_anim * (weather == W_RAIN ? 3 : 1)) % SCR_W, y = (k * 29 + t_anim * (weather == W_RAIN ? 7 : weather == W_HAIL ? 2 : 1)) % SCR_H;
            if (weather == W_RAIN) fill(fb, x, y, 1, 5, RGB(14, 20, 31));
            else if (weather == W_HAIL) fill(fb, x, y, 2, 2, C_WHITE);
            else if (weather == W_SAND) fill(fb, (x + (t_anim * 5)) % SCR_W, y, 3, 1, (k & 1) ? C_MAGENTA : C_CYAN);
        }
        if (weather == W_SUN) fill_blend(fb, 0, 0, SCR_W, SCR_H, RGB(31, 20, 6), 2);
        int w = text_width(weather_names[weather]) + 10;
        panel_solid(fb, SCR_W - w - 4, 50, w, 13, RGB(2, 2, 6), C_YELLOW);
        draw_text(fb, SCR_W - w + 1, 52, weather_names[weather], C_YELLOW);
    }
    if (E.m) draw_info(fb, &E, 6, 8, 0);
    if (P.m) draw_info(fb, &P, 132, 120, 1);
    if (is_trainer) {
        for (int i = 0; i < enemy_n; i++) {
            int alive = enemy_team[i].hp > 0;
            circle_fill(fb, 12 + i * 10, 44, 3, alive ? C_MAGENTA : RGB(6, 6, 8));
        }
    }
    top_dirty = 1;
}

static void draw_scene_lazy(void)
{
    // redraw only when something visible changed
    u32 key = ((t_anim + 1) >> 2) * 2654435761u;
    key ^= ((t_anim + 1) / 12) * 97;
    key ^= (u32)(P.m ? P.disp_hp * 131 + P.m->species * 7 + P.m->status * 3 + P.m->level * 5 : 0) << 1;
    key ^= (u32)(E.m ? E.disp_hp * 151 + E.m->species * 11 + E.m->status * 13 : 0) << 7;
    key ^= ((u32)hide_p << 1) ^ ((u32)hide_e << 2) ^ ((u32)shake_e << 3) ^ ((u32)shake_p << 4) ^ ((u32)(off_px & 255) << 8) ^ ((u32)(off_ex & 255) << 16);
    key ^= ((u32)e_mode << 5) ^ ((u32)p_mode << 6) ^ ((u32)(e_scale_override + 1) << 24);
    if (shake_e || shake_p) key ^= (u32)t_anim << 20;
    if (key == last_scene_key) { t_anim++; return; }
    draw_scene();
    last_scene_key = key;
}

static void bframe(void)
{
    // animate hp bars
    if (P.m && P.disp_hp != P.m->hp) P.disp_hp += P.disp_hp < P.m->hp ? 1 : -1;
    if (E.m && E.disp_hp != E.m->hp) E.disp_hp += E.disp_hp < E.m->hp ? 1 : -1;
    if (P.m && P.disp_hp != P.m->hp) P.disp_hp += P.disp_hp < P.m->hp ? 1 : -1;
    if (E.m && E.disp_hp != E.m->hp) E.disp_hp += E.disp_hp < E.m->hp ? 1 : -1;
    draw_scene_lazy();
    frame();
}

static void bwait(int n) { while (n-- > 0) bframe(); }

static void bottom_panel(void)
{
    draw_bottom_bg_dark();
}

// message on bottom screen
static void bmsg(const char *text)
{
    int total = text_glyph_count(text), shown = 0;
    bmsg_text = text;
    LOG("[battle] %s\n", text);
#ifndef __NDS__
    if (getenv("VX_AUTOTEXT")) { bframe(); while ((P.m && P.disp_hp != P.m->hp) || (E.m && E.disp_hp != E.m->hp)) bframe(); return; }
#endif
    int speed = G.textspeed == 0 ? 1 : (G.textspeed == 1 ? 2 : 4);
    int hold = 0, last_shown = -1, last_blink = -1;
    for (;;) {
        int blink = shown >= total && ((frame_count / 16) & 1);
        if (shown != last_shown || blink != last_blink) {
            bottom_panel();
            panel(fb_bot, 6, 6, 244, 70, C_CYAN);
            draw_text_wrap(fb_bot, 16, 16, 226, text, C_TEXT, RGB(2, 2, 6), shown, 12);
            if (blink) draw_text(fb_bot, 236, 62, "▼", C_CYAN);
            bot_dirty = 1;
            last_shown = shown; last_blink = blink;
        }
        bframe();
        if (shown < total) {
            shown += speed;
            if ((key_down & (K_A | K_B)) || touch_down) shown = total;
        } else {
            hold++;
            if ((key_down & (K_A | K_B)) || touch_down) break;
            if (P.disp_hp != (P.m ? P.m->hp : 0) || (E.m && E.disp_hp != E.m->hp)) continue;
            if (hold > 160) break;
        }
    }
    // settle hp bars
    while ((P.m && P.disp_hp != P.m->hp) || (E.m && E.disp_hp != E.m->hp)) bframe();
}

static int stat_mod(int v, int s)
{
    if (s >= 0) return v * (2 + s) / 2;
    return v * 2 / (2 - s);
}

static int eff_speed(Side *s)
{
    int v = stat_mod(s->m->spd, s->stage[3]);
    int k = abil_kind(s->m);
    if (k == AB_SPEED) v = v * 5 / 4;
    if ((k == AB_SWIM && weather == W_RAIN) || (k == AB_CHLORO && weather == W_SUN)) v *= 2;
    if (STATUS(s->m) == ST_PARA) v /= 4;
    return v;
}

static void particles(int from_player, int type, int phys)
{
    u16 c = type_color(type);
    int x0 = from_player ? 70 : 190, y0 = from_player ? 130 : 60;
    int x1 = from_player ? 190 : 70, y1 = from_player ? 60 : 130;
    if (phys) {
        for (int i = 0; i < 6; i++) { if (from_player) off_px = i * 4; else off_ex = -i * 4; bframe(); }
        for (int i = 6; i >= 0; i--) { if (from_player) off_px = i * 4; else off_ex = -i * 4; bframe(); }
        // impact burst
        for (int f = 0; f < 10; f++) {
            draw_scene();
            for (int k = 0; k < 8; k++) {
                int a = k * 32 + f * 4;
                static const s8 cs[8][2] = { { 10, 0 }, { 7, 7 }, { 0, 10 }, { -7, 7 }, { -10, 0 }, { -7, -7 }, { 0, -10 }, { 7, -7 } };
                int r = f * 2;
                circle_fill(fb_top, x1 + cs[k][0] * r / 10, y1 + cs[k][1] * r / 10, 3 - f / 4, f & 1 ? C_WHITE : c);
                (void)a;
            }
            frame();
        }
        return;
    }
    for (int f = 0; f < 22; f++) {
        draw_scene();
        for (int k = 0; k < 6; k++) {
            int t = f - k * 2;
            if (t < 0 || t > 14) continue;
            int x = x0 + (x1 - x0) * t / 14, y = y0 + (y1 - y0) * t / 14 - (t * (14 - t)) / 3;
            circle_fill(fb_top, x, y, 4 - (k > 3), k & 1 ? lerp_col(c, C_WHITE, 8, 16) : c);
        }
        frame();
    }
    for (int f = 0; f < 8; f++) {
        draw_scene();
        circle_fill(fb_top, x1, y1, 6 + f * 2, lerp_col(c, C_WHITE, f, 8));
        circle_fill(fb_top, x1, y1, 3 + f, C_WHITE);
        frame();
    }
}

static void blink(int player)
{
    for (int i = 0; i < 8; i++) {
        if (player) hide_p = i & 1; else hide_e = i & 1;
        bwait(3);
    }
    hide_p = hide_e = 0;
}

static const char *mname(Side *s, char *buf)
{
    if (s == &E) fmt(buf, is_trainer ? "%s (Gegner)" : "Wildes %s", species[s->m->species].name);
    else fmt(buf, "%s", species[s->m->species].name);
    return buf;
}

static void apply_stage(Side *s, int st, int d)
{
    static const char *const names[4] = { "Angriff", "Verteidigung", "Technik", "Initiative" };
    char nb[40], buf[120];
    mname(s, nb);
    int v = s->stage[st] + d;
    if (v > 6 || v < -6) { fmt(buf, "%s: %s kann nicht weiter %s!", nb, names[st], d > 0 ? "steigen" : "sinken"); bmsg(buf); return; }
    s->stage[st] = v;
    fmt(buf, "%s: %s %s!", nb, names[st], d > 0 ? "steigt" : "sinkt");
    sfx(d > 0 ? SFX_LEVEL : SFX_WEAK);
    bmsg(buf);
}

static int has_type(const Mon *m, int t) { return species[m->species].type1 == t || species[m->species].type2 == t; }
static void heal_hp(Mon *m, int h) { m->hp = m->hp + h > m->maxhp ? m->maxhp : m->hp + h; }
static void hurt_hp(Mon *m, int d) { m->hp = m->hp > d ? m->hp - d : 0; }

// Klarbeere: cures status and confusion right away
static void check_cure_berry(Side *s)
{
    char nb[40], buf[100];
    if (s->m->held != HI_KLARBEERE || s->m->hp == 0 || (!STATUS(s->m) && !s->confuse)) return;
    s->m->status = 0; s->confuse = 0; s->toxic_n = 0;
    s->m->held = 0;
    fmt(buf, "%s nutzt seine Klarbeere und ist wieder fit!", mname(s, nb));
    sfx(SFX_HEAL);
    bmsg(buf);
}

// Datenbeere: heals once below half HP
static void check_hp_berry(Side *s)
{
    char nb[40], buf[100];
    if (s->m->held != HI_DATENBEERE || s->m->hp == 0 || s->m->hp * 2 >= s->m->maxhp) return;
    heal_hp(s->m, s->m->maxhp / 4);
    s->m->held = 0;
    fmt(buf, "%s nutzt seine Datenbeere und heilt sich!", mname(s, nb));
    sfx(SFX_HEAL);
    bmsg(buf);
}

static void inflict(Side *s, int st)
{
    char nb[40], buf[120];
    if (STATUS(s->m) || s->m->hp == 0) return;
    int k = abil_kind(s->m);
    if (k == AB_IMMUNE) { bmsg("Der Antivirus-Kern blockt den Statuseffekt!"); return; }
    // type and ability immunities
    if (st == ST_BURN && has_type(s->m, T_PLASMA)) return;
    if ((st == ST_POISON || st == ST_TOXIC) && (has_type(s->m, T_TOXIN) || has_type(s->m, T_METALL))) return;
    if (st == ST_FREEZE && (has_type(s->m, T_KRISTALL) || weather == W_SUN || k == AB_NOFREEZE)) return;
    if (st == ST_SLEEP && k == AB_NOSLEEP) { fmt(buf, "%s kann wegen Schlaflos nicht einschlafen!", mname(s, nb)); bmsg(buf); return; }
    static const char *const msg[] = { "", "%s brennt jetzt!", "%s hat einen Glitch! Es kann sich kaum bewegen.", "%s wurde mit einem Virus infiziert!",
                                       "%s ist eingeschlafen!", "%s ist eingefroren!", "%s wurde schwer infiziert!" };
    s->m->status = st;
    if (st == ST_SLEEP) s->m->status |= (u8)((2 + rnd() % 3) << 4);
    if (st == ST_TOXIC) s->toxic_n = 0;
    fmt(buf, msg[st], mname(s, nb));
    bmsg(buf);
    check_cure_berry(s);
}

static void confuse(Side *s)
{
    char nb[40], buf[100];
    if (s->m->hp == 0) return;
    if (s->confuse) { fmt(buf, "%s ist bereits verwirrt.", mname(s, nb)); bmsg(buf); return; }
    if (abil_kind(s->m) == AB_NOCONFUSE) { fmt(buf, "%s lässt sich dank Eigensinn nicht verwirren!", mname(s, nb)); bmsg(buf); return; }
    s->confuse = 2 + rnd() % 4;
    fmt(buf, "%s ist verwirrt!", mname(s, nb));
    bmsg(buf);
    check_cure_berry(s);
}

static void set_weather(int w, Side *by, int permanent)
{
    static const char *const msg[5] = { "", "Die Plasmasonne brennt vom Himmel!", "Es beginnt, Daten zu regnen!",
                                        "Ein Glitchsturm zieht auf!", "Kristallhagel fällt vom Himmel!" };
    if (weather == w) { if (!permanent) bmsg("Das Wetter ändert sich nicht."); return; }
    weather = w;
    weather_turns = permanent ? 0 : (by && by->m->held == HI_KLIMAKERN ? 8 : 5);
    bmsg(msg[w]);
}

static int is_foe_move(const MoveData *M)
{
    if (M->power) return 1;
    switch (M->effect) {
    case E_BURN: case E_PARA: case E_POISON: case E_SLEEP: case E_CONFUSE: case E_TOXIC: case E_FATKDN: case E_FDEFDN: case E_FSPDDN:
    case E_FATKDN2: case E_FDEFDN2: case E_FTECDN: case E_SEED: return 1;
    }
    return 0;
}

// weather ball: type and power follow the weather
static int move_type(const MoveData *M)
{
    if (M->effect == E_WBALL) { static const u8 wt[5] = { T_AERO, T_PLASMA, T_HYDRO, T_GRAVO, T_KRISTALL }; return wt[weather]; }
    return M->type;
}

// effects before a Vexel can act; returns 0 if it cannot move this turn
static int can_act(Side *a)
{
    char nb[40], buf[140];
    if (a->recharge) { a->recharge = 0; fmt(buf, "%s muss sich erholen!", mname(a, nb)); bmsg(buf); return 0; }
    if (a->flinch) { a->flinch = 0; fmt(buf, "%s schreckt zurück!", mname(a, nb)); bmsg(buf); return 0; }
    int st = STATUS(a->m);
    if (st == ST_SLEEP) {
        int turns = a->m->status >> 4;
        turns -= abil_kind(a->m) == AB_EARLYBIRD ? 2 : 1;
        if (turns > 0) {
            a->m->status = (u8)(ST_SLEEP | (turns << 4));
            fmt(buf, "%s schläft tief und fest.", mname(a, nb)); bmsg(buf);
            return 0;
        }
        a->m->status = 0;
        fmt(buf, "%s ist aufgewacht!", mname(a, nb)); bmsg(buf);
    }
    if (st == ST_FREEZE) {
        if (rnd() % 5 == 0 || weather == W_SUN) { a->m->status = 0; fmt(buf, "%s ist wieder aufgetaut!", mname(a, nb)); bmsg(buf); }
        else { fmt(buf, "%s ist eingefroren!", mname(a, nb)); bmsg(buf); return 0; }
    }
    if (st == ST_PARA && rnd() % 4 == 0) { fmt(buf, "%s glitcht und kann sich nicht bewegen!", mname(a, nb)); bmsg(buf); return 0; }
    if (a->confuse) {
        if (--a->confuse == 0) { fmt(buf, "%s ist nicht mehr verwirrt!", mname(a, nb)); bmsg(buf); }
        else {
            fmt(buf, "%s ist verwirrt!", mname(a, nb)); bmsg(buf);
            if (rnd() % 3 == 0) {
                int dmg = ((2 * a->m->level / 5 + 2) * 40 * a->m->atk / (a->m->def ? a->m->def : 1)) / 50 + 2;
                blink(a == &P);
                hurt_hp(a->m, dmg);
                while (a->disp_hp != a->m->hp) bframe();
                bmsg("Es hat sich vor Verwirrung selbst verletzt!");
                return 0;
            }
        }
    }
    return 1;
}

// returns 1 if target fainted
static int do_move(Side *a, Side *d, int slot)
{
    char nb[40], buf[140];
    int mv = a->m->moves[slot];
    if (mv == NO_MOVE) return 0;
    const MoveData *M = &moves[mv];
    a->moved = 1;
    if (!can_act(a)) { a->charging = 0; return 0; }
    int continuing = a->charging != 0;
    if (!continuing && a->m->pp[slot] > 0) a->m->pp[slot]--;
    if (M->effect == E_CHARGE && !continuing && !(weather == W_SUN && M->type == T_BIO)) {
        a->charging = (u8)(slot + 1);
        fmt(buf, "%s setzt %s ein und lädt Energie auf!", mname(a, nb), M->name);
        bmsg(buf);
        return 0;
    }
    a->charging = 0;
    fmt(buf, "%s setzt %s ein!", mname(a, nb), M->name);
    bmsg(buf);
    if (M->effect != E_PROTECT) a->protect_chain = 0;
    if (d->protect && is_foe_move(M)) {
        fmt(buf, "%s schützt sich!", mname(d, nb));
        bmsg(buf);
        return 0;
    }
    int mt = move_type(M);
    int acc = M->acc;
    if (abil_kind(a->m) == AB_ACCURACY) acc += 20;
    if (abil_kind(d->m) == AB_SANDVEIL && weather == W_SAND) acc = acc * 4 / 5;
    int sure = M->effect == E_SURE || (weather == W_RAIN && mt == T_VOLT) || (weather == W_HAIL && mt == T_KRISTALL);
    if (!sure && (int)(rnd() % 100) >= acc) {
        bmsg("Daneben!");
        return 0;
    }
    int from_player = (a == &P);
    particles(from_player, mt, M->phys && M->power);
    if (M->power == 0) {
        int has_st = STATUS(d->m) != 0;
        switch (M->effect) {
        case E_BURN: if (has_st) bmsg("Es hat keine Wirkung..."); else inflict(d, ST_BURN); break;
        case E_PARA: if (has_st) bmsg("Es hat keine Wirkung..."); else inflict(d, ST_PARA); break;
        case E_POISON: if (has_st) bmsg("Es hat keine Wirkung..."); else inflict(d, ST_POISON); break;
        case E_TOXIC: if (has_st) bmsg("Es hat keine Wirkung..."); else inflict(d, ST_TOXIC); break;
        case E_SLEEP: if (has_st) bmsg("Es hat keine Wirkung..."); else inflict(d, ST_SLEEP); break;
        case E_CONFUSE: confuse(d); break;
        case E_ATKUP: apply_stage(a, 0, 1); break;
        case E_DEFUP: apply_stage(a, 1, 1); break;
        case E_TECUP: apply_stage(a, 2, 1); break;
        case E_SPDUP: apply_stage(a, 3, 2); break;
        case E_ATKUP2: apply_stage(a, 0, 2); break;
        case E_DEFUP2: apply_stage(a, 1, 2); break;
        case E_TECUP2: apply_stage(a, 2, 2); break;
        case E_DANCE: apply_stage(a, 0, 1); apply_stage(a, 3, 1); break;
        case E_CALM: apply_stage(a, 2, 1); apply_stage(a, 1, 1); break;
        case E_BULK: apply_stage(a, 0, 1); apply_stage(a, 1, 1); break;
        case E_FATKDN: apply_stage(d, 0, -1); break;
        case E_FDEFDN: apply_stage(d, 1, -1); break;
        case E_FSPDDN: apply_stage(d, 3, -1); break;
        case E_FTECDN: apply_stage(d, 2, -1); break;
        case E_FATKDN2: apply_stage(d, 0, -2); break;
        case E_FDEFDN2: apply_stage(d, 1, -2); break;
        case E_CRITUP: a->critup = 1; fmt(buf, "%s konzentriert sich!", mname(a, nb)); bmsg(buf); break;
        case E_HAZE: reset_side(a); reset_side(d); a->moved = 1; bmsg("Alle Statuswerte wurden zurückgesetzt!"); break;
        case E_SUN: set_weather(W_SUN, a, 0); break;
        case E_RAIN: set_weather(W_RAIN, a, 0); break;
        case E_SAND: set_weather(W_SAND, a, 0); break;
        case E_HAIL: set_weather(W_HAIL, a, 0); break;
        case E_PROTECT:
            if (rnd() % (1u << (a->protect_chain > 3 ? 3 : a->protect_chain)) == 0) {
                a->protect = 1; a->protect_chain++;
                fmt(buf, "%s schützt sich mit einem Schild!", mname(a, nb));
            } else { a->protect_chain = 0; fmt(buf, "%s: Der Schild hat versagt!", mname(a, nb)); }
            bmsg(buf);
            break;
        case E_REST:
            if (a->m->hp >= a->m->maxhp) { bmsg("Die KP sind bereits voll!"); break; }
            if (abil_kind(a->m) == AB_NOSLEEP) { bmsg("Schlaflos verhindert das!"); break; }
            a->m->hp = a->m->maxhp;
            a->m->status = ST_SLEEP | (3 << 4);
            sfx(SFX_HEAL);
            fmt(buf, "%s legt sich schlafen und heilt sich vollständig!", mname(a, nb));
            bmsg(buf);
            break;
        case E_CURE:
            if (!STATUS(a->m) && !a->confuse) { bmsg("Es hat keine Wirkung..."); break; }
            a->m->status = 0; a->confuse = 0;
            sfx(SFX_HEAL);
            fmt(buf, "%s hat seine Systeme bereinigt!", mname(a, nb));
            bmsg(buf);
            break;
        case E_SEED:
            if (d->seeded || has_type(d->m, T_BIO)) { bmsg("Es hat keine Wirkung..."); break; }
            d->seeded = 1;
            fmt(buf, "%s wurde mit Egeln infiziert!", mname(d, nb));
            bmsg(buf);
            break;
        case E_HEAL: {
            if (a->m->hp >= a->m->maxhp) { bmsg("Die KP sind bereits voll!"); break; }
            heal_hp(a->m, a->m->maxhp / 2);
            sfx(SFX_HEAL);
            fmt(buf, "%s regeneriert sich!", mname(a, nb));
            bmsg(buf);
            break;
        }
        }
        return 0;
    }
    int ak = abil_kind(a->m), dk = abil_kind(d->m);
    const AbilityData *aab = &abilities[abil_of(a->m)], *dab = &abilities[abil_of(d->m)];
    if (dk == AB_EVASIVE && rnd() % 10 == 0) {
        fmt(buf, "%s weicht per Phasenschritt aus!", mname(d, nb));
        bmsg(buf);
        return 0;
    }
    if (M->effect == E_DREAM && STATUS(d->m) != ST_SLEEP) { bmsg("Es hat keine Wirkung... das Ziel schläft nicht."); return 0; }
    int hits = 1;
    if (M->effect == E_MULTI) hits = 2 + rnd() % 3;
    int m4 = type_mult4(mt, d->m->species);   // 4 = 1x
    int any_crit = 0, total = 0, before = d->m->hp;
    int power = M->power;
    if (M->effect == E_HEX && STATUS(d->m)) power *= 2;
    if (M->effect == E_WBALL && weather) power *= 2;
    for (int h = 0; h < hits && d->m->hp > 0; h++) {
        int L = a->m->level;
        int A = M->phys ? stat_mod(a->m->atk, a->stage[0]) : stat_mod(a->m->tec, a->stage[2]);
        int D = M->phys ? stat_mod(d->m->def, d->stage[1]) : stat_mod(d->m->tec, d->stage[2]);
        if (M->phys && STATUS(a->m) == ST_BURN && ak != AB_GUTS) A /= 2;
        if (M->phys && ak == AB_GUTS && STATUS(a->m)) A = A * 3 / 2;
        if (!M->phys && weather == W_SAND && (has_type(d->m, T_GRAVO) || has_type(d->m, T_METALL))) D = D * 3 / 2;
        if (D < 1) D = 1;
        int dmg = ((2 * L / 5 + 2) * power * A / D) / 50 + 2;
        int stab = (mt == species[a->m->species].type1 || mt == species[a->m->species].type2);
        if (stab) dmg = ak == AB_STAB ? dmg * 7 / 4 : dmg * 3 / 2;
        dmg = dmg * m4 / 4;
        dmg = dmg * attr_mult(species[a->m->species].attr, species[d->m->species].attr) / 100;
        // weather
        if ((weather == W_SUN && mt == T_PLASMA) || (weather == W_RAIN && mt == T_HYDRO)) dmg = dmg * 3 / 2;
        if ((weather == W_SUN && mt == T_HYDRO) || (weather == W_RAIN && mt == T_PLASMA)) dmg /= 2;
        if (M->effect == E_CHARGE && M->type == T_BIO && (weather == W_RAIN || weather == W_SAND || weather == W_HAIL)) dmg /= 2;
        // abilities (attacker)
        if (ak == AB_WEAPON && M->phys) dmg = dmg * 6 / 5;
        if (ak == AB_TECH && !M->phys) dmg = dmg * 6 / 5;
        if (ak == AB_OVERLOAD && a->m->hp * 3 < a->m->maxhp) dmg = dmg * 3 / 2;
        if (ak == AB_BERSERK) dmg = dmg * 5 / 4;
        if (ak == AB_TYPEBOOST && aab->param == mt) dmg = dmg * 13 / 10;
        // held items (attacker)
        if (a->m->held == HI_KRAFTMODUL && M->phys) dmg = dmg * 6 / 5;
        if (a->m->held == HI_TECHNIKMODUL && !M->phys) dmg = dmg * 6 / 5;
        if (a->m->held == HI_UEBERLAST) dmg = dmg * 13 / 10;
        // abilities and items (defender)
        if (dk == AB_PHYS_ARMOR && M->phys) dmg = dmg * 4 / 5;
        if (dk == AB_SPEC_SHIELD && !M->phys) dmg = dmg * 4 / 5;
        if (dk == AB_BERSERK) dmg = dmg * 11 / 10;
        if (dk == AB_TYPERESIST && dab->param == mt) dmg /= 2;
        if (d->m->held == HI_PANZERWESTE) dmg = dmg * 17 / 20;
        int critc = 16;
        if (ak == AB_CRIT) critc /= 2;
        if (a->critup) critc /= 2;
        if (a->m->held == HI_ZIELCHIP) critc /= 2;
        if (M->effect == E_CRIT) critc /= 2;
        if (critc < 2) critc = 2;
        int crit = (rnd() % critc) == 0;
        if (crit) { dmg = dmg * 3 / 2; any_crit = 1; }
        dmg = dmg * (85 + rnd() % 16) / 100;
        if (M->effect == E_FIXED) dmg = m4 ? a->m->level : 0;
        if (M->effect == E_HALF) dmg = m4 ? (d->m->hp + 1) / 2 : 0;
        if (dmg < 1 && m4) dmg = 1;
        // sturdy and Notfallchip: survive a one-hit KO from full HP
        int side = d == &P ? 0 : 1;
        if (dk == AB_STURDY && d->m->hp == d->m->maxhp && dmg >= d->m->hp && !sturdy_used[side]) {
            dmg = d->m->hp - 1;
            sturdy_used[side] = 1;
            if (dmg <= 0) dmg = 0;
        }
        int sash = 0;
        if (d->m->held == HI_NOTFALLCHIP && d->m->hp == d->m->maxhp && dmg >= d->m->hp) { dmg = d->m->hp - 1; sash = 1; }
        sfx(m4 > 4 ? SFX_SUPER : m4 < 4 ? SFX_WEAK : SFX_HIT);
        if (d == &E) shake_e = 1; else shake_p = 1;
        blink(d == &P);
        shake_e = shake_p = 0;
        hurt_hp(d->m, dmg);
        total += dmg;
        while (d->disp_hp != d->m->hp) bframe();
        if (sash) { d->m->held = 0; fmt(buf, "%s übersteht den Treffer dank Notfallchip!", mname(d, nb)); bmsg(buf); }
        if (hits > 1 && h + 1 < hits && d->m->hp > 0) particles(from_player, mt, M->phys);
    }
    if (hits > 1) { fmt(buf, "%d Treffer!", hits); bmsg(buf); }
    if (any_crit) bmsg("Ein Volltreffer!");
    if (m4 == 0) bmsg("Es hat keine Wirkung...");
    else if (m4 > 4) bmsg("Das ist sehr effektiv!");
    else if (m4 < 4) bmsg("Das ist nicht sehr effektiv...");
    if (d->m->hp == 1 && dk == AB_STURDY && sturdy_used[d == &P ? 0 : 1] == 1) {
        sturdy_used[d == &P ? 0 : 1] = 2;
        fmt(buf, "%s hält dank Bollwerk durch!", mname(d, nb));
        bmsg(buf);
    }
    int dealt = before - d->m->hp;
    (void)total;
    if ((M->effect == E_DRAIN || M->effect == E_DREAM) && dealt > 0 && a->m->hp > 0) {
        heal_hp(a->m, dealt / 2 + 1);
        fmt(buf, "%s saugt Energie ab!", mname(a, nb));
        bmsg(buf);
    }
    if (ak == AB_LEECH && dealt > 0 && a->m->hp > 0 && a->m->hp < a->m->maxhp) {
        heal_hp(a->m, dealt / 8 + 1);
        fmt(buf, "%s stiehlt Lebensenergie!", mname(a, nb));
        bmsg(buf);
    }
    if (a->m->held == HI_SAUGMODUL && dealt > 0 && a->m->hp > 0 && a->m->hp < a->m->maxhp) {
        heal_hp(a->m, dealt / 8 + 1);
        fmt(buf, "%s heilt sich mit dem Saugmodul.", mname(a, nb));
        bmsg(buf);
    }
    if (M->effect == E_RECOIL && dealt > 0) {
        hurt_hp(a->m, dealt / 4 + 1);
        fmt(buf, "%s wird vom Rückstoß getroffen!", mname(a, nb));
        bmsg(buf);
    }
    if (a->m->held == HI_UEBERLAST && dealt > 0 && a->m->hp > 0) {
        hurt_hp(a->m, a->m->maxhp / 10 + 1);
        fmt(buf, "%s verliert KP durch den Überlastkern.", mname(a, nb));
        bmsg(buf);
    }
    if (dk == AB_THORNS && M->phys && dealt > 0 && a->m->hp > 0) {
        hurt_hp(a->m, a->m->maxhp / 8 + 1);
        fmt(buf, "%s verletzt sich am Stachelpanzer!", mname(a, nb));
        bmsg(buf);
    }
    if (dk == AB_RAGE && d->m->hp > 0 && dealt > 0 && rnd() % 10 < 3 && d->stage[0] < 6) {
        fmt(buf, "%s gerät in Kampfrausch!", mname(d, nb));
        bmsg(buf);
        apply_stage(d, 0, 1);
    }
    if (M->effect == E_HEX && M->type == T_BESTIE && STATUS(d->m) == ST_SLEEP && d->m->hp > 0) {   // Alarmhieb wakes the target
        d->m->status = 0;
        fmt(buf, "%s ist aufgewacht!", mname(d, nb));
        bmsg(buf);
    }
    if (M->effect == E_RECHARGE && dealt > 0) a->recharge = 1;
    if (M->effect == E_SELFTECDN && a->m->hp > 0) apply_stage(a, 2, -2);
    if (M->effect == E_SELFDEFDN && a->m->hp > 0) apply_stage(a, 1, -1);
    if (M->effect == E_TRAP && d->m->hp > 0 && !d->trap) {
        d->trap = 4 + rnd() % 2;
        fmt(buf, "%s ist gefangen!", mname(d, nb));
        bmsg(buf);
    }
    if (mt == T_PLASMA && STATUS(d->m) == ST_FREEZE && d->m->hp > 0) {
        d->m->status = 0;
        fmt(buf, "%s ist aufgetaut!", mname(d, nb));
        bmsg(buf);
    }
    int chance = M->chance;
    if (ak == AB_SERENE) chance *= 2;
    if (dk == AB_SHIELDDUST) chance = 0;
    if (d->m->hp > 0 && chance && (int)(rnd() % 100) < chance) {
        switch (M->effect) {
        case E_BURN: inflict(d, ST_BURN); break;
        case E_PARA: inflict(d, ST_PARA); break;
        case E_POISON: inflict(d, ST_POISON); break;
        case E_TOXIC: inflict(d, ST_TOXIC); break;
        case E_FREEZE: inflict(d, ST_FREEZE); break;
        case E_CONFUSE: confuse(d); break;
        case E_FLINCH: if (!d->moved && abil_kind(d->m) != AB_NOFLINCH) d->flinch = 1; break;
        case E_FATKDN: apply_stage(d, 0, -1); break;
        case E_FDEFDN: apply_stage(d, 1, -1); break;
        case E_FSPDDN: apply_stage(d, 3, -1); break;
        case E_FTECDN: apply_stage(d, 2, -1); break;
        }
    }
    check_hp_berry(d);
    check_hp_berry(a);
    return d->m->hp == 0;
}

static int ai_pick(void)
{
    int best = -1, bestv = -1;
    for (int i = 0; i < 4; i++) {
        int mv = E.m->moves[i];
        if (mv == NO_MOVE || E.m->pp[i] == 0) continue;
        const MoveData *M = &moves[mv];
        int v;
        if (M->power == 0) {
            v = 30 + rnd() % 40;
            if (M->effect == E_HEAL && E.m->hp * 2 > E.m->maxhp) v = 0;
            int e = M->effect;
            if ((e == E_BURN || e == E_PARA || e == E_POISON || e == E_TOXIC || e == E_SLEEP) && STATUS(P.m)) v = 0;
            if (e == E_CONFUSE && P.confuse) v = 0;
            if (e == E_SEED && P.seeded) v = 0;
            if ((e == E_REST || e == E_CURE) && E.m->hp * 2 > E.m->maxhp && !STATUS(E.m)) v = 0;
            if (e == E_PROTECT && E.protect_chain) v = 0;
            if (((e == E_SUN && weather == W_SUN) || (e == E_RAIN && weather == W_RAIN) || (e == E_SAND && weather == W_SAND) || (e == E_HAIL && weather == W_HAIL))) v = 0;
            if (e == E_CRITUP && E.critup) v = 0;
        } else {
            v = M->power * type_mult4(move_type(M), P.m->species) / 4;
            if (M->effect == E_FIXED || M->effect == E_HALF) v = 60;
            if (M->effect == E_DREAM && STATUS(P.m) != ST_SLEEP) v = 0;
            if (M->type == species[E.m->species].type1 || M->type == species[E.m->species].type2) v = v * 3 / 2;
            v += rnd() % 30;
        }
        if (!is_trainer) v = rnd() % 100 + (M->power ? 20 : 0);
        if (v > bestv) { bestv = v; best = i; }
    }
    return best;
}

static void end_of_turn(Side *s, Side *foe)
{
    if (!s->m || s->m->hp == 0) return;
    char nb[40], buf[100];
    s->flinch = 0; s->protect = 0; s->moved = 0;
    int st = STATUS(s->m), k = abil_kind(s->m);
    if (st == ST_BURN || st == ST_POISON || st == ST_TOXIC) {
        int d = s->m->maxhp / 8 + 1;
        if (st == ST_TOXIC) { if (s->toxic_n < 15) s->toxic_n++; d = s->m->maxhp * s->toxic_n / 16 + 1; }
        hurt_hp(s->m, d);
        fmt(buf, st == ST_BURN ? "%s leidet unter seiner Verbrennung!" : "%s leidet unter dem Virus!", mname(s, nb));
        blink(s == &P);
        bmsg(buf);
    }
    if (s->m->hp && weather == W_SAND && !has_type(s->m, T_GRAVO) && !has_type(s->m, T_METALL) && !has_type(s->m, T_VIRUS) && k != AB_SANDVEIL) {
        hurt_hp(s->m, s->m->maxhp / 16 + 1);
        fmt(buf, "%s wird vom Glitchsturm getroffen!", mname(s, nb));
        bmsg(buf);
    }
    if (s->m->hp && weather == W_HAIL && !has_type(s->m, T_KRISTALL) && k != AB_ICEBODY) {
        hurt_hp(s->m, s->m->maxhp / 16 + 1);
        fmt(buf, "%s wird vom Kristallhagel getroffen!", mname(s, nb));
        bmsg(buf);
    }
    if (s->m->hp && weather == W_HAIL && k == AB_ICEBODY && s->m->hp < s->m->maxhp) {
        heal_hp(s->m, s->m->maxhp / 16 + 1);
        fmt(buf, "%s heilt sich im Hagel.", mname(s, nb));
        bmsg(buf);
    }
    if (s->m->hp && s->seeded && foe->m && foe->m->hp) {
        int d = s->m->maxhp / 8 + 1;
        if (d > s->m->hp) d = s->m->hp;
        hurt_hp(s->m, d);
        heal_hp(foe->m, d);
        fmt(buf, "Die Egel saugen %s Energie ab!", mname(s, nb));
        bmsg(buf);
    }
    if (s->m->hp && s->trap) {
        s->trap--;
        hurt_hp(s->m, s->m->maxhp / 8 + 1);
        fmt(buf, s->trap ? "%s ist gefangen und nimmt Schaden!" : "%s hat sich befreit!", mname(s, nb));
        bmsg(buf);
    }
    if (k == AB_REGEN && s->m->hp > 0 && s->m->hp < s->m->maxhp) {
        heal_hp(s->m, s->m->maxhp / 16 + 1);
        fmt(buf, "%s regeneriert sich.", mname(s, nb));
        bmsg(buf);
    }
    if (s->m->held == HI_NANOREP && s->m->hp > 0 && s->m->hp < s->m->maxhp) {
        heal_hp(s->m, s->m->maxhp / 16 + 1);
        fmt(buf, "%s repariert sich mit der Nanoreparatur.", mname(s, nb));
        bmsg(buf);
    }
    if (k == AB_MEDIC && STATUS(s->m) && rnd() % 3 == 0) {
        s->m->status = 0;
        fmt(buf, "%s hat sich selbst geheilt!", mname(s, nb));
        bmsg(buf);
    }
    check_hp_berry(s);
}

static void weather_tick(void)
{
    if (weather && weather_turns && --weather_turns == 0) {
        static const char *const msg[5] = { "", "Die Plasmasonne verblasst.", "Der Datenregen hört auf.", "Der Glitchsturm legt sich.", "Der Hagel hört auf." };
        bmsg(msg[weather]);
        weather = W_NONE;
    }
}

static void on_entry(Side *s, Side *foe)
{
    char nb[40], buf[100];
    int k = abil_kind(s->m);
    if (k == AB_INTIMIDATE && foe->m && foe->m->hp > 0) {
        fmt(buf, "%s schüchtert den Gegner ein!", mname(s, nb));
        bmsg(buf);
        apply_stage(foe, 0, -1);
    }
    if (k == AB_SUN || k == AB_RAIN || k == AB_SAND || k == AB_HAIL) {
        int w = k == AB_SUN ? W_SUN : k == AB_RAIN ? W_RAIN : k == AB_SAND ? W_SAND : W_HAIL;
        if (weather != w) {
            fmt(buf, "%s: %s!", mname(s, nb), abilities[abil_of(s->m)].name);
            bmsg(buf);
            set_weather(w, s, 1);
        }
    }
}

static void faint_anim(Side *s)
{
    sfx(SFX_WEAK);
    for (int i = 0; i < 20; i++) {
        if (s == &E) e_mode = (i & 2) ? 2 : 0; else p_mode = (i & 2) ? 2 : 0;
        if (s == &E) off_ex = 0;
        bframe();
    }
    if (s == &E) hide_e = 1; else hide_p = 1;
    e_mode = p_mode = 0;
}

static int first_alive(void)
{
    for (int i = 0; i < G.party_n; i++) if (G.party[i].hp > 0) return i;
    return -1;
}

int battle_party_alive(void) { return first_alive() >= 0; }

static int active_idx;

static void learn_move(Mon *m, int mv)
{
    char buf[160];
    for (int i = 0; i < 4; i++) if (m->moves[i] == mv) return;
    for (int i = 0; i < 4; i++) if (m->moves[i] == NO_MOVE) {
        m->moves[i] = mv; m->pp[i] = moves[mv].pp;
        fmt(buf, "%s hat %s gelernt!", species[m->species].name, moves[mv].name);
        sfx(SFX_LEVEL);
        bmsg(buf);
        return;
    }
    fmt(buf, "%s möchte %s lernen, kennt aber schon 4 Attacken. Welche soll es vergessen?", species[m->species].name, moves[mv].name);
    bmsg(buf);
    const char *opts[5];
    char ob[4][40];
    for (int i = 0; i < 4; i++) { fmt(ob[i], "%s (%d)", moves[m->moves[i]].name, moves[m->moves[i]].power); opts[i] = ob[i]; }
    char nb[48];
    fmt(nb, "%s nicht lernen", moves[mv].name);
    opts[4] = nb;
    bottom_panel();
    bot_dirty = 1;
    int r = menu_list("Attacke vergessen", opts, 5, 0);
    if (r >= 0 && r < 4) {
        fmt(buf, "1, 2, 3... %s hat %s vergessen und %s gelernt!", species[m->species].name, moves[m->moves[r]].name, moves[mv].name);
        m->moves[r] = mv; m->pp[r] = moves[mv].pp;
        sfx(SFX_LEVEL);
        bmsg(buf);
    } else {
        fmt(buf, "%s hat %s nicht gelernt.", species[m->species].name, moves[mv].name);
        bmsg(buf);
    }
}

static void gain_exp(Mon *m, u32 amount, int show)
{
    char buf[120];
    if (m->level >= 100) return;
    m->exp += amount;
    if (show) {
        fmt(buf, "%s erhält %d EP!", species[m->species].name, (int)amount);
        bmsg(buf);
    }
    while (m->level < 100 && m->exp >= exp_for_level(m->level + 1)) {
        int oldhp = m->maxhp, oa = m->atk, od = m->def, ot = m->tec, os = m->spd;
        m->level++;
        mon_calc(m);
        sfx(SFX_LEVEL);
        fmt(buf, "%s erreicht Level %d!", species[m->species].name, m->level);
        if (m == P.m) P.disp_hp = m->hp;
        bmsg(buf);
        if (show) {
            // stat panel
            bottom_panel();
            panel_solid(fb_bot, 40, 80, 176, 96, RGB(3, 3, 9), C_YELLOW);
            const char *nm[5] = { "KP", "Angriff", "Verteid.", "Technik", "Initiat." };
            int ov[5] = { oldhp, oa, od, ot, os }, nv[5] = { m->maxhp, m->atk, m->def, m->tec, m->spd };
            for (int i = 0; i < 5; i++) {
                draw_text_sh(fb_bot, 52, 88 + i * 16, nm[i], C_TEXT2, C_BLACK);
                fmt(buf, "%d  +%d", nv[i], nv[i] - ov[i]);
                draw_text_sh(fb_bot, 140, 88 + i * 16, buf, C_WHITE, C_BLACK);
            }
            bot_dirty = 1;
            for (;;) { bframe(); if ((key_down & (K_A | K_B)) || touch_down) break; }
        }
        int ls[4];
        int n = mon_learn_at(m, m->level, ls, 4);
        for (int i = 0; i < n; i++) learn_move(m, ls[i]);
    }
}

static void award_exp(Mon *defeated)
{
    const SpeciesData *s = &species[defeated->species];
    u32 base = (u32)s->exp * defeated->level / 5;
    if (is_trainer) base = base * 3 / 2;
    if (base < 1) base = 1;
    int shared = 0;
    for (int i = 0; i < G.party_n; i++) {
        Mon *m = &G.party[i];
        if (m->hp == 0) continue;
        u32 b = m->held == HI_LERNCHIP ? base * 3 / 2 : base;
        if (part_mask & (1 << i)) gain_exp(m, b, 1);
        else if (G.expshare && flag(F_KEY_EXPSHARE)) {
            if (!shared) { bmsg("Der EP-Teiler verteilt Erfahrung an dein Team."); shared = 1; }
            gain_exp(m, b / 2 + 1, 0);
        } else gain_exp(m, b / 3 + 1, 0);
    }
}

static int choose_move(void)
{
    int cur = 0;
    for (;;) {
        bottom_panel();
        Mon *m = P.m;
        for (int i = 0; i < 4; i++) {
            int x = 8 + (i % 2) * 122, y = 8 + (i / 2) * 62;
            int mv = m->moves[i];
            if (mv == NO_MOVE) { panel_solid(fb_bot, x, y, 118, 58, RGB(2, 2, 4), RGB(6, 6, 8)); draw_text(fb_bot, x + 50, y + 24, "--", C_GREY); continue; }
            u16 col = type_color(moves[mv].type);
            button(fb_bot, x, y, 118, 58, "", col, i == cur);
            draw_text_sh(fb_bot, x + 59 - text_width(moves[mv].name) / 2, y + 10, moves[mv].name, C_WHITE, C_BLACK);
            char buf[32];
            fmt(buf, "AP %d/%d", m->pp[i], moves[mv].pp);
            draw_text_sh(fb_bot, x + 8, y + 40, buf, m->pp[i] ? C_TEXT : C_RED, C_BLACK);
            type_badge(fb_bot, x + 68, y + 39, moves[mv].type);
            if (moves[mv].power) {
                fmt(buf, "Stärke %d", moves[mv].power);
                int m4 = type_mult4(move_type(&moves[mv]), E.m->species);   // 4 = x1, dual types multiply
                draw_text_sh(fb_bot, x + 8, y + 25, buf, C_TEXT2, C_BLACK);
                if (m4 >= 16) draw_text_sh(fb_bot, x + 68, y + 25, "×4 stark!", C_GREEN, C_BLACK);
                else if (m4 > 4) draw_text_sh(fb_bot, x + 68, y + 25, "×2 stark", C_GREEN, C_BLACK);
                else if (m4 == 2) draw_text_sh(fb_bot, x + 68, y + 25, "×½ wenig", RGB(28, 14, 14), C_BLACK);
                else if (m4 < 2) draw_text_sh(fb_bot, x + 68, y + 25, "×¼ wenig", RGB(28, 8, 8), C_BLACK);
            } else draw_text_sh(fb_bot, x + 8, y + 25, "Status", C_TEXT2, C_BLACK);
        }
        button(fb_bot, 8, 142, 240, 44, "ZURÜCK (B)", C_GREY, 0);
        bot_dirty = 1;
        int done = 0;
        while (!done) {
            bframe();
            if (key_rep & K_LEFT) { cur ^= 1; done = 1; sfx(SFX_SELECT); }
            if (key_rep & K_RIGHT) { cur ^= 1; done = 1; sfx(SFX_SELECT); }
            if (key_rep & K_UP) { cur ^= 2; done = 1; sfx(SFX_SELECT); }
            if (key_rep & K_DOWN) { cur ^= 2; done = 1; sfx(SFX_SELECT); }
            int pick = -1;
            if (key_down & K_A) pick = cur;
            for (int i = 0; i < 4; i++) if (touched(8 + (i % 2) * 122, 8 + (i / 2) * 62, 118, 58)) pick = i;
            if ((key_down & K_B) || touched(8, 142, 240, 44)) { sfx(SFX_CANCEL); return -1; }
            if (pick >= 0) {
                if (m->moves[pick] == NO_MOVE) continue;
                if (m->pp[pick] == 0) {
                    int any = 0;
                    for (int i = 0; i < 4; i++) if (m->moves[i] != NO_MOVE && m->pp[i]) any = 1;
                    if (any) { sfx(SFX_BUMP); continue; }
                }
                sfx(SFX_CONFIRM);
                return pick;
            }
        }
    }
}

static int action_menu(void)
{
    static int cur = 0;
    static const char *const lab[4] = { "KAMPF", "TASCHE", "VEXEL", "FLUCHT" };
    static const u16 col[4] = { RGB(31, 6, 20), RGB(31, 22, 4), RGB(4, 24, 31), RGB(12, 12, 16) };
    char buf[80];
    for (;;) {
        bottom_panel();
        panel(fb_bot, 6, 4, 244, 34, C_CYAN);
        fmt(buf, "Was soll %s tun?", species[P.m->species].name);
        draw_text_sh(fb_bot, 16, 16, buf, C_TEXT, C_BLACK);
        button(fb_bot, 8, 44, 240, 62, lab[0], col[0], cur == 0);
        for (int i = 1; i < 4; i++) button(fb_bot, 8 + (i - 1) * 81, 112, 78, 74, lab[i], col[i], cur == i);
        bot_dirty = 1;
        int done = 0;
        while (!done) {
            bframe();
            if (key_rep & K_UP) { cur = 0; done = 1; }
            if (key_rep & K_DOWN && cur == 0) { cur = 1; done = 1; }
            if (key_rep & K_LEFT && cur > 1) { cur--; done = 1; }
            if (key_rep & K_RIGHT && cur >= 1 && cur < 3) { cur++; done = 1; }
            if (done) sfx(SFX_SELECT);
            if (key_down & K_A) { sfx(SFX_CONFIRM); return cur; }
            if (touched(8, 44, 240, 62)) { cur = 0; sfx(SFX_CONFIRM); return 0; }
            for (int i = 1; i < 4; i++) if (touched(8 + (i - 1) * 81, 112, 78, 74)) { cur = i; sfx(SFX_CONFIRM); return i; }
            if (key_down & K_B) { cur = 3; done = 1; }
        }
    }
}

static void send_out_player(int idx, int announce)
{
    char buf[80];
    active_idx = idx;
    P.m = &G.party[idx];
    reset_side(&P);
    P.disp_hp = P.m->hp;
    part_mask |= 1 << idx;
    hide_p = 0;
    if (announce) {
        fmt(buf, "Los, %s!", species[P.m->species].name);
        // slide in
        for (int i = 10; i >= 0; i--) { off_px = -i * 12; p_mode = i > 2 ? 1 : 0; bframe(); }
        p_mode = 0;
        bmsg(buf);
        if (E.m) on_entry(&P, &E);
    }
}

static int catch_attempt(int item)
{
    char buf[120];
    static const int mult[4] = { 10, 15, 20, 2550 };
    // throw animation
    sfx(SFX_CATCH);
    for (int f = 0; f <= 20; f++) {
        draw_scene();
        int x = 60 + (190 - 60) * f / 20, y = 140 - (f * (20 - f)) / 2 * 1 + (60 - 140) * f / 20;
        draw_capsule_icon(fb_top, x - 4, y - 4, item);
        frame();
    }
    // absorb
    for (int f = 0; f < 12; f++) {
        e_mode = 1;
        e_scale_override = f < 4 ? 3 : (f < 8 ? 2 : 1);
        draw_scene();
        draw_capsule_icon(fb_top, 186, 56, item);
        frame();
    }
    hide_e = 1; e_mode = 0; e_scale_override = -1;
    int maxhp = E.m->maxhp, hp = E.m->hp;
    int a = (3 * maxhp - 2 * hp) * species[E.m->species].catchrate * mult[item] / (2 * maxhp) / 10;
    if (STATUS(E.m)) a = (STATUS(E.m) == ST_SLEEP || STATUS(E.m) == ST_FREEZE) ? a * 2 : a * 3 / 2;
    int caught = a >= 255 || (int)(rnd() % 255) < a;
    int shakes = caught ? 3 : (a > 180 ? 2 : a > 90 ? 1 : 0) + (rnd() & 1);
    if (shakes > 3) shakes = 3;
    if (!caught && shakes == 3) shakes = 2;
    for (int s = 0; s < shakes; s++) {
        sfx(SFX_SHAKE);
        for (int f = 0; f < 24; f++) {
            draw_scene();
            int dx = (f < 12) ? ((f / 3) & 1 ? 3 : -3) : 0;
            draw_capsule_icon(fb_top, 186 + dx, 80, item);
            frame();
        }
    }
    if (caught) {
        for (int f = 0; f < 30; f++) {
            draw_scene();
            draw_capsule_icon(fb_top, 186, 80, item);
            for (int k = 0; k < 3; k++) draw_text(fb_top, 176 + k * 10, 64 - f / 3 - (k & 1) * 4, "*", C_YELLOW);
            frame();
        }
        fmt(buf, "Geschafft! %s wurde gefangen!", species[E.m->species].name);
        sfx(SFX_LEVEL);
        bmsg(buf);
        int was = is_caught(E.m->species);
        Mon m = *E.m;
        m.status = 0;
        int r = give_mon(&m);
        if (!was) {
            fmt(buf, "Neuer Eintrag im Vexikon: %s!", species[m.species].name);
            bmsg(buf);
        }
        if (r == 1) { fmt(buf, "%s wurde in den Cloud-Speicher übertragen.", species[m.species].name); bmsg(buf); }
        if (r < 0) bmsg("Kein Platz mehr! Der Vexel wurde freigelassen.");
        return 1;
    }
    hide_e = 0;
    for (int f = 0; f < 8; f++) { e_mode = f & 1; bframe(); }
    e_mode = 0;
    static const char *const fail[] = { "Oh nein! Es hat sich befreit!", "Fast! Es war beinahe gefangen!", "Argh! Nur ein paar Bits fehlten!" };
    bmsg(fail[shakes > 2 ? 2 : shakes]);
    return 0;
}

static void transition_in(void)
{
    plat_light_off();
    for (int i = 0; i < 6; i++) {
        fill_blend(fb_top, 0, 0, SCR_W, SCR_H, i & 1 ? C_BLACK : C_WHITE, 10);
        top_dirty = 1;
        frame();
    }
    for (int k = 0; k < 16; k++) {
        for (int y = 0; y < SCR_H; y += 12) {
            int w = (k + 1) * 16;
            if ((y / 12) & 1) fill(fb_top, SCR_W - w, y, w, 12, C_BLACK);
            else fill(fb_top, 0, y, w, 12, C_BLACK);
        }
        top_dirty = 1;
        frame();
    }
}

// some places have their own weather during battles
#include "gen_maps.h"
static int map_weather(void)
{
    switch (cur_map) {
    case M_R_LAVA: case M_GLUT: return W_SUN;
    case M_R_TIEFSEE: return W_RAIN;
    case M_GLITCH: return W_SAND;
    case M_AETHERBERG: case M_R_HIMMEL: return W_HAIL;
    }
    return W_NONE;
}

// sparkles around a shiny Vexel when it appears
static void shiny_sparkle(int player)
{
    sfx(SFX_SUPER);
    for (int f = 0; f < 40; f++) {
        draw_scene();
        int cx = player ? 64 : 192, cy = player ? 120 : 60;
        for (int k = 0; k < 6; k++) {
            int x = cx + ((k * 41 + f * 7) % 64) - 32;
            int y = cy + ((k * 37 + f * 3) % 50) - 25;
            draw_text(fb_top, x, y, "*", (f + k) & 2 ? C_YELLOW : C_WHITE);
        }
        frame();
    }
    bmsg(player ? "Dein Vexel schillert!" : "Wow! Es schillert in seltenen Farben!");
}

static int run_battle(void)
{
    setup_types();
    char buf[160];
    part_mask = 0;
    sturdy_used[0] = sturdy_used[1] = 0;
    hide_p = 1; hide_e = 0;
    off_ex = 200;
    int pi = first_alive();
    if (pi < 0) return BR_LOSE;
    P.m = NULL;
    E.m = &enemy_team[0];
    reset_side(&E);
    reset_side(&P);
    weather = map_weather(); weather_turns = 0;
    E.disp_hp = E.m->hp;
    mark_seen(E.m->species);
    transition_in();
    bottom_panel();
    bot_dirty = 1;
    for (int i = 20; i >= 0; i--) { off_ex = i * 8; bframe(); }
    if (is_trainer) {
        fmt(buf, "%s fordert dich heraus!", tr_name);
        bmsg(buf);
        fmt(buf, "%s schickt %s!", tr_name, species[E.m->species].name);
        bmsg(buf);
    } else {
        fmt(buf, "Ein wildes %s erscheint!", species[E.m->species].name);
        bmsg(buf);
    }
    if (E.m->mflags & MF_SHINY) shiny_sparkle(0);
    send_out_player(pi, 1);
    on_entry(&E, &P);
    int run_tries = 0;
    for (;;) {
        // ---- choose action
        int pmove = -1, switched = 0;
        int forced = P.charging || P.recharge;
        int act = forced ? -1 : action_menu();
        if (forced) pmove = P.charging ? P.charging - 1 : 0;
        if (act == 0) {
            int any = 0;
            for (int i = 0; i < 4; i++) if (P.m->moves[i] != NO_MOVE && P.m->pp[i]) any = 1;
            if (!any) { bmsg("Keine AP mehr! Dein Vexel kämpft mit letzter Kraft!"); pmove = 0; }
            else { pmove = choose_move(); if (pmove < 0) continue; }
        } else if (act == 1) {
            bot_snapshot();
            int it = bag_menu(1);
            bottom_panel();
            if (it < 0) continue;
            if (it <= IT_OMEGAKAPSEL) {
                if (is_trainer) { bmsg("Du kannst keine Vexel anderer Trainer fangen!"); continue; }
                G.items[it]--;
                fmt(buf, "Du wirfst eine %s!", items[it].name);
                bmsg(buf);
                if (catch_attempt(it)) return BR_CAUGHT;
            } else if (it >= IT_SCHUTZ) {
                bmsg("Das kannst du im Kampf nicht benutzen.");
                continue;
            } else {
                int t = party_menu(2, active_idx);
                bottom_panel();
                if (t < 0) continue;
                Mon *m = &G.party[t];
                int used = 0;
                if (it == IT_KIT || it == IT_SUPERKIT || it == IT_MAXKIT) {
                    if (m->hp > 0 && m->hp < m->maxhp) {
                        int h = it == IT_KIT ? 30 : it == IT_SUPERKIT ? 80 : 9999;
                        m->hp = m->hp + h > m->maxhp ? m->maxhp : m->hp + h;
                        used = 1;
                    }
                } else if (it == IT_REBOOT) {
                    if (m->hp == 0) { m->hp = m->maxhp / 2; used = 1; }
                } else if (it == IT_ANTIVIRUS) {
                    if (m->status) { m->status = 0; used = 1; }
                } else if (it == IT_ZELLE) {
                    for (int i = 0; i < 4; i++) if (m->moves[i] != NO_MOVE) m->pp[i] = moves[m->moves[i]].pp;
                    used = 1;
                }
                if (!used) { bmsg("Das hätte keine Wirkung."); continue; }
                G.items[it]--;
                sfx(SFX_HEAL);
                fmt(buf, "%s wurde bei %s angewendet.", items[it].name, species[m->species].name);
                bmsg(buf);
            }
        } else if (act == 2) {
            if (P.trap) { bmsg("Dein Vexel ist gefangen und kann nicht ausgewechselt werden!"); continue; }
            int t = party_menu(1, active_idx);
            bottom_panel();
            if (t < 0 || t == active_idx) continue;
            if (G.party[t].hp == 0) { bmsg("Dieser Vexel ist kampfunfähig!"); continue; }
            fmt(buf, "Zurück, %s!", species[P.m->species].name);
            bmsg(buf);
            send_out_player(t, 1);
            switched = 1;
        } else if (act == 3) {
            if (is_trainer) { bmsg("Gegen Trainer kannst du nicht fliehen!"); continue; }
            if (P.trap && P.m->held != HI_NEBELKAPSEL) { bmsg("Du bist gefangen und kannst nicht fliehen!"); continue; }
            run_tries++;
            int ps = eff_speed(&P), es = eff_speed(&E);
            if (P.m->held == HI_NEBELKAPSEL || ps >= es || (int)(rnd() % 256) < (ps * 128 / (es ? es : 1) + 30 * run_tries)) {
                sfx(SFX_CANCEL);
                bmsg("Du bist erfolgreich geflohen!");
                return BR_RUN;
            }
            bmsg("Flucht fehlgeschlagen!");
        }
        // ---- enemy move
        int emove = E.charging ? E.charging - 1 : E.recharge ? 0 : ai_pick();
        P.moved = E.moved = 0;
        // order
        int p_first = 1;
        if (pmove >= 0) {
            int pf = moves[P.m->moves[pmove]].effect == E_FAST || (abil_kind(P.m) == AB_FIRSTSTRIKE && P.m->hp == P.m->maxhp);
            int ef = emove >= 0 && (moves[E.m->moves[emove]].effect == E_FAST || (abil_kind(E.m) == AB_FIRSTSTRIKE && E.m->hp == E.m->maxhp));
            if (pmove >= 0 && moves[P.m->moves[pmove]].effect == E_PROTECT) pf = 2;
            if (emove >= 0 && moves[E.m->moves[emove]].effect == E_PROTECT) ef = 2;
            if (!pf && P.m->held == HI_TURBOCHIP && rnd() % 5 == 0) { pf = 1; bmsg("Der Turbochip macht deinen Vexel schneller!"); }
            if (!ef && emove >= 0 && E.m->held == HI_TURBOCHIP && rnd() % 5 == 0) ef = 1;
            if (pf != ef) p_first = pf > ef;
            else {
                int ps = eff_speed(&P), es = eff_speed(&E);
                p_first = ps > es || (ps == es && (rnd() & 1));
            }
        } else p_first = 1;
        (void)switched;
        for (int turn = 0; turn < 2; turn++) {
            int player_turn = (turn == 0) == p_first;
            if (player_turn) {
                if (pmove < 0 || P.m->hp == 0) continue;
                if (do_move(&P, &E, pmove)) break;
            } else {
                if (emove < 0 || E.m->hp == 0) continue;
                if (do_move(&E, &P, emove)) break;
            }
            if (P.m->hp == 0 || E.m->hp == 0) break;
        }
        if (E.m->hp > 0 && P.m->hp > 0) { end_of_turn(&P, &E); if (E.m->hp > 0) end_of_turn(&E, &P); }
        if (E.m->hp > 0 && P.m->hp > 0) weather_tick();
        // ---- fainted enemy
        if (E.m->hp == 0) {
            faint_anim(&E);
            fmt(buf, "%s ist kampfunfähig!", mname(&E, buf + 100));
            bmsg(buf);
            award_exp(E.m);
            P.disp_hp = P.m->hp;
            int next = -1;
            if (is_trainer)
                for (int i = enemy_cur + 1; i < enemy_n; i++) if (enemy_team[i].hp > 0) { next = i; break; }
            if (next < 0) {
                if (is_trainer) {
                    music_play(MUS_VICTORY);
                    int money = tr_money;
                    for (int i = 0; i < G.party_n; i++) if ((part_mask & (1 << i)) && G.party[i].held == HI_CREDITCHIP) { money *= 2; break; }
                    G.money += money;
                    fmt(buf, "Du hast %s besiegt!", tr_name);
                    bmsg(buf);
                    if (money) { fmt(buf, "Du erhältst %d Credits.", money); bmsg(buf); }
                } else music_play(MUS_VICTORY);
                return BR_WIN;
            }
            enemy_cur = next;
            E.m = &enemy_team[next];
            reset_side(&E);
            E.disp_hp = E.m->hp;
            mark_seen(E.m->species);
            hide_e = 0;
            for (int i = 12; i >= 0; i--) { off_ex = i * 10; bframe(); }
            fmt(buf, "%s schickt %s!", tr_name, species[E.m->species].name);
            bmsg(buf);
            on_entry(&E, &P);
        }
        // ---- fainted player
        if (P.m->hp == 0) {
            faint_anim(&P);
            fmt(buf, "%s ist kampfunfähig!", species[P.m->species].name);
            bmsg(buf);
            if (first_alive() < 0) {
                bmsg("Du hast keine kampffähigen Vexel mehr!");
                return BR_LOSE;
            }
            int t;
            do {
                bmsg("Wähle deinen nächsten Vexel!");
                t = party_menu(1, first_alive() >= 0 ? first_alive() : active_idx);
                bottom_panel();
            } while (t < 0 || G.party[t].hp == 0);
            send_out_player(t, 1);
        }
    }
}

static void finish_battle(int r)
{
    music_play(MUS_NONE);
    for (int i = 0; i < G.party_n; i++) {
        Mon *m = &G.party[i];
        if (STATUS(m) == ST_TOXIC) m->status = ST_POISON;   // bad poison wears off to normal poison
        // Sammler: sometimes picks up an item after a won battle
        if (r == BR_WIN && abil_kind(m) == AB_PICKUP && !m->held && rnd() % 10 == 0) {
            static const u8 finds[] = { HI_DATENBEERE, HI_DATENBEERE, HI_KLARBEERE, HI_KLARBEERE, HI_NEBELKAPSEL, HI_ZIELCHIP, HI_NANOREP };
            m->held = finds[rnd() % sizeof(finds)];
            char buf[100];
            fmt(buf, "%s hat etwas gefunden: %s!", species[m->species].name, held_items[m->held].name);
            bmsg(buf);
        }
    }
    fade_out(3);
    if (r != BR_LOSE) check_evolutions();
    P.m = E.m = NULL;
}

int battle_wild(int sp, int lv, int bg)
{
    return battle_wild_ex(sp, lv, bg, MUS_BATTLE);
}

int battle_wild_ex(int sp, int lv, int bg, int music)
{
    is_trainer = 0;
    bg_style = bg;
    enemy_n = 1; enemy_cur = 0;
    mon_create(&enemy_team[0], sp, lv);
#ifndef __NDS__
    if (getenv("VX_EMOVE")) { int mv = atoi(getenv("VX_EMOVE")); for (int i = 0; i < 4; i++) { enemy_team[0].moves[i] = mv; enemy_team[0].pp[i] = 30; } }
#endif
    int hr = rnd() % 100;   // wild Vexel sometimes carry an item
    if (hr < 4) enemy_team[0].held = HI_DATENBEERE;
    else if (hr < 7) enemy_team[0].held = HI_KLARBEERE;
    else if (hr == 7) { static const u8 rare[] = { HI_NANOREP, HI_ZIELCHIP, HI_TURBOCHIP, HI_LERNCHIP, HI_STASISKERN, HI_NEBELKAPSEL }; enemy_team[0].held = rare[rnd() % sizeof(rare)]; }
    music_play(music);
    int r = run_battle();
    finish_battle(r);
    return r;
}

int battle_team(const char *name, int chr, Mon *team, int n, int money, int boss, int bg, int music)
{
    (void)chr;
#ifndef __NDS__
    if (getenv("VX_WIN")) { LOG("[autowin] %s (%d mons, ace lv %d)\n", name, n, team[n - 1].level); return BR_WIN; }
#endif
    is_trainer = 1;
    tr_name = name;
    tr_money = money;
    tr_boss = boss;
    bg_style = bg;
    enemy_n = n > 6 ? 6 : n;
    enemy_cur = 0;
    for (int i = 0; i < enemy_n; i++) enemy_team[i] = team[i];
    // trainers carry items: bosses equip their ace, everyone may have berries
    static const u8 boss_items[] = { HI_DATENBEERE, HI_UEBERLAST, HI_NANOREP, HI_NOTFALLCHIP, HI_KRAFTMODUL, HI_TECHNIKMODUL, HI_ZIELCHIP, HI_PANZERWESTE };
    for (int i = 0; i < enemy_n; i++) {
        Mon *m = &enemy_team[i];
        if (m->held) continue;
        if (boss && i == enemy_n - 1 && m->level >= 15) m->held = boss_items[rnd() % sizeof(boss_items)];
        else if (rnd() % 100 < (boss ? 35 : 15)) m->held = (rnd() & 1) ? HI_DATENBEERE : HI_KLARBEERE;
    }
    music_play(music);
    int r = run_battle();
    finish_battle(r);
    return r;
}

int battle_trainer(int tid)
{
    const TrainerData *T = &trainers[tid];
    Mon team[6];
    for (int i = 0; i < T->n; i++) {
        int line = T->team[i][0] >> 3;
        if (line >= NUM_LINES) line = rival_line();
        int lv = scaled_level(T->team[i][1], i, T->boss);
        mon_create(&team[i], species_for_level(line, lv), lv);
    }
    int mus = T->boss ? MUS_BOSS : MUS_BATTLE;
    if (tid == 36) mus = MUS_FINAL;
    return battle_team(T->name, T->chr, team, T->n, T->money, T->boss, T->bg, mus);
}

// ------------------------------------------------------------------ evolution
void evolve_scene(Mon *m, int ns)
{
    plat_light_off();
    char buf[140];
    int old = m->species;
    music_play(MUS_EVOLVE);
    draw_bottom_bg();
    panel(fb_bot, 6, 6, 244, 60, C_YELLOW);
    fmt(buf, "Was? %s entwickelt sich!", species[old].name);
    draw_text_wrap(fb_bot, 16, 18, 226, buf, C_TEXT, C_BLACK, -1, 12);
    bot_dirty = 1;
    for (int f = 0; f < 300; f++) {
        u16 *fb = fb_top;
        draw_battle_bg(fb, 0x86, f);
        for (int k = 0; k < 24; k++) {
            int a = (k * 37 + f * (2 + k % 3)) % 256;
            int r = 20 + ((k * 53 + f * 2) % 90);
            static const s8 sn[16] = { 0, 38, 71, 92, 100, 92, 71, 38, 0, -38, -71, -92, -100, -92, -71, -38 };
            int x = 128 + sn[(a / 16) % 16] * r / 100, y = 96 + sn[((a / 16) + 4) % 16] * r / 100;
            circle_fill(fb, x, y, 1 + (k & 1), k & 2 ? C_CYAN : C_MAGENTA);
        }
        int period = 40 - f / 8;
        if (period < 3) period = 3;
        int show_new = (f / period) & 1;
        int sp = show_new ? ns : old;
        draw_mon(fb, sp, 64, 32, 4, 0, (f > 30 ? 1 : 0) | SHINY(m));
        top_dirty = 1;
        frame();
    }
    for (int f = 0; f < 16; f++) { fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, f); top_dirty = 1; frame(); }
    m->species = ns;
    mon_calc(m);
    mark_caught(ns);
    for (int f = 0; f < 40; f++) {
        gradient_v(fb_top, 0, 0, SCR_W, SCR_H, RGB(2, 0, 8), RGB(10, 2, 16));
        draw_mon(fb_top, ns, 64, 32, 4, 0, SHINY(m));
        if (f < 16) fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, 16 - f);
        top_dirty = 1;
        frame();
    }
    sfx(SFX_LEVEL);
    fmt(buf, "Glückwunsch! %s hat sich zu %s entwickelt!", species[old].name, species[ns].name);
    LOG("[evolve] %s -> %s\n", species[old].name, species[ns].name);
    draw_bottom_bg();
    panel(fb_bot, 6, 6, 244, 60, C_YELLOW);
    draw_text_wrap(fb_bot, 16, 18, 226, buf, C_TEXT, C_BLACK, -1, 12);
    if (species[ns].type2 != species[old].type2) {
        fmt(buf, "Neuer Typ: %s / %s", type_names[species[ns].type1], type_names[species[ns].type2]);
        draw_text_sh(fb_bot, 16, 46, buf, C_YELLOW, C_BLACK);
    }
    bot_dirty = 1;
    for (;;) { frame(); if ((key_down & (K_A | K_B)) || touch_down) break; }
    // learn moves for the new level if any
    int ls[4];
    int n = mon_learn_at(m, m->level, ls, 4);
    P.m = NULL; E.m = NULL;
    for (int i = 0; i < n; i++) {
        int known = 0;
        for (int k = 0; k < 4; k++) if (m->moves[k] == ls[i]) known = 1;
        if (!known) {
            for (int k = 0; k < 4; k++) if (m->moves[k] == NO_MOVE) { m->moves[k] = ls[i]; m->pp[k] = moves[ls[i]].pp; known = 1; break; }
        }
    }
    music_play(MUS_NONE);
    fade_out(3);
}

void check_evolutions(void)
{
    for (int i = 0; i < G.party_n; i++) {
        Mon *m = &G.party[i];
        const SpeciesData *s = &species[m->species];
        if (s->evo && m->level >= s->evo && m->hp > 0 && m->held != HI_STASISKERN) {
            int ns = m->species + 1;
            if (s->alt_evo != 0xFFFF && alt_cond_met(m, s->alt_cond)) ns = s->alt_evo;   // alternative path
            if (ns < NUM_SPECIES && species[ns].line == s->line) evolve_scene(m, ns);
        }
    }
}
