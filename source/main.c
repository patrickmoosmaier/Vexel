#include <string.h>
#include "game.h"
#include "gen_maps.h"
#include <stdio.h>

static void new_game(void)
{
    memset(&G, 0, sizeof(G));
    G.map = M_HOME;
    G.x = P_HOME_START_X; G.y = P_HOME_START_Y; G.dir = 0;
    G.money = 500;
    G.textspeed = 1;
    G.music_on = 1;
    save_init_new_fields(&G);
}

static void draw_logo(u16 *fb, int t)
{
    static const char *title = "VEXEL";
    int sc = 5;
    int w = text_width(title) * sc + 4 * sc;
    int x = 128 - w / 2, y = 30;
    // glow layers
    for (int k = 3; k >= 1; k--) {
        int o = k;
        draw_text_big(fb, x - o, y, title, lerp_col(C_MAGENTA, C_BLACK, 8 + k * 2, 16), lerp_col(C_MAGENTA, C_BLACK, 8 + k * 2, 16), sc);
        draw_text_big(fb, x + o, y, title, lerp_col(C_CYAN, C_BLACK, 8 + k * 2, 16), lerp_col(C_CYAN, C_BLACK, 8 + k * 2, 16), sc);
    }
    draw_text_big(fb, x, y, title, C_WHITE, C_WHITE, sc);
    // scanline shimmer
    int sy = y + ((t / 2) % 40);
    fill_blend(fb, x - 6, sy, w + 12, 2, C_CYAN, 6);
    const char *sub = "PROTOKOLL  NULL";
    draw_text_big(fb, 128 - (text_width(sub) * 2) / 2, y + 56, sub, C_YELLOW, RGB(10, 2, 8), 2);
}

void play_title(void)
{
    music_play(MUS_TITLE);
    int has_save = save_exists();
    int cur = has_save ? 1 : 0;
    int showcase = rnd() % NUM_SPECIES;
    int t = 0;
    for (;;) {
        t++;
        draw_battle_bg(fb_top, 0x80, t);
        // showcase creature cycling
        if (t % 150 == 0) showcase = rnd() % NUM_SPECIES;
        int fade = t % 150;
        draw_mon(fb_top, showcase, 128 - 48, 92 - 8, 3, (t / 150) & 1, fade < 8 || fade > 142 ? 1 : 0);
        draw_logo(fb_top, t);
        if ((t / 30) & 1) draw_text_sh(fb_top, 128 - text_width("Berühre den Bildschirm oder drücke A") / 2, 178, "Berühre den Bildschirm oder drücke A", C_WHITE, C_BLACK);
        top_dirty = 1;
        if (t == 1 || (t % 30) == 0) {
            draw_bottom_bg();
            u16 *fb = fb_bot;
            draw_text_big(fb, 128 - text_width("NEO-KYRA 2187") , 14, "NEO-KYRA 2187", C_CYAN, RGB(2, 2, 8), 2);
            button(fb, 48, 56, 160, 34, "NEUES SPIEL", C_MAGENTA, cur == 0);
            button(fb, 48, 98, 160, 34, has_save ? "FORTSETZEN" : "(kein Spielstand)", has_save ? C_CYAN : C_GREY, cur == 1);
            draw_text(fb, 128 - text_width("1669 Vexel. 24 Arenen. 8 Urcodes.") / 2, 150, "1463 Vexel. 24 Arenen. 8 Urcodes.", C_TEXT2);
            draw_text(fb, 128 - text_width("Ein unabhängiges Homebrew-Spiel") / 2, 172, "Ein unabhängiges Homebrew-Spiel", RGB(10, 12, 18));
            bot_dirty = 1;
        }
        frame();
        if ((key_rep & K_UP) && cur == 1) { cur = 0; sfx(SFX_SELECT); t = (t / 30) * 30 + 29; }
        if ((key_rep & K_DOWN) && cur == 0 && has_save) { cur = 1; sfx(SFX_SELECT); t = (t / 30) * 30 + 29; }
        int pick = -1;
        if (key_down & (K_A | K_START)) pick = cur;
        if (touched(48, 56, 160, 34)) pick = 0;
        if (touched(48, 98, 160, 34) && has_save) pick = 1;
        if (pick == 0) {
            sfx(SFX_CONFIRM);
            if (has_save) {
                fade_out(3);
                draw_bottom_bg();
                say(NULL, "Ein Spielstand existiert bereits. Ein neues Spiel überschreibt ihn beim nächsten Speichern.");
            }
            new_game();
            fade_out(3);
            play_intro();
            return;
        }
        if (pick == 1) {
            sfx(SFX_CONFIRM);
            load_game();
            fade_out(3);
            return;
        }
    }
}

static void story_page(const char *text, int style, int sp)
{
    int total = text_glyph_count(text), shown = 0, t = 0;
    for (;;) {
        t++;
        draw_battle_bg(fb_top, style | 0x80, t);
        if (sp >= 0) draw_mon(fb_top, sp, 128 - 48, 40, 3, 0, 0);
        top_dirty = 1;
        if (shown < total || (t & 15) == 0) {
            draw_bottom_bg();
            panel(fb_bot, 10, 30, 236, 130, C_CYAN);
            draw_text_wrap(fb_bot, 22, 44, 212, text, C_TEXT, RGB(2, 2, 6), shown, 13);
            if (shown >= total && ((t / 16) & 1)) draw_text(fb_bot, 232, 146, "▼", C_CYAN);
            bot_dirty = 1;
        }
        frame();
        if (shown < total) { shown += 2; if ((key_down & (K_A | K_B)) || touch_down) shown = total; }
        else if ((key_down & (K_A | K_B)) || touch_down) { sfx(SFX_SELECT); break; }
        if (key_down & K_START) break;
    }
}

void play_intro(void)
{
    music_play(MUS_ENDING);
    static const char *const pages[] = {
        "Jahr 2187.",
        "Die Meere sind gestiegen. Was von der Menschheit blieb, lebt in NEO-KYRA – einer Megastadt auf Stelzen über dem endlosen Ozean.",
        "Vor zwanzig Jahren traf ein gewaltiger Sonnensturm das globale Netz AETHER. Der Code zerbrach – und aus den Trümmern entstand Leben.",
        "Die VEXEL. Halb Daten, halb Materie. Sie wachsen und entwickeln sich in bis zu sechs Stufen: Bit, Byte, Kern, Champion, Ultra und Mega.",
        "Der Konzern HELIX DYNAMICS kontrolliert Energie, Netz und Gesetz. Und HELIX sammelt Vexel. Niemand weiß, wofür.",
        "Du bist NOVA VOSS, Kurier im Rost-Sektor. Deine Mutter, Dr. Lyra Voss, ist die bekannteste Vexel-Forscherin der Stadt.",
        "Gestern Nacht ist sie nicht nach Hause gekommen. Stattdessen... blinkt eine Nachricht auf deinem Terminal.",
    };
    static const int styles[] = { 6, 0, 6, 1, 5, 0, 0 };
    for (int i = 0; i < 7; i++) {
        int sp = -1;
        if (i == 3) sp = species_for_level(rnd() % FIRST_LEGEND_LINE, 30);
        story_page(pages[i], styles[i], sp);
        if (key_held & K_START) break;
    }
    fade_out(3);
}

void play_ending(void)
{
    music_play(MUS_ENDING);
    fade_out(3);
    static const char *const credits[] = {
        "VEXEL – PROTOKOLL NULL", "", "Die Vexel sind frei.", "Helix Dynamics wurde aufgelöst.", "Kai Kade baut die Stadt neu auf.",
        "Lyra Voss forscht wieder – diesmal mit dir.", "", "~ ENDE ~", "", "Danke fürs Spielen!", "", "Aber das Abenteuer geht weiter:",
        "Im Datenabgrund warten die stärksten Vexel.", "Und als Champion beginnt die Jagd nach den Urcodes...", "Kannst du alle 1669 fangen?", "",
    };
    int n = sizeof(credits) / sizeof(credits[0]);
    for (int t = 0; t < 1100; t++) {
        draw_battle_bg(fb_top, 0x80, t);
        int sp = (t / 40) % NUM_SPECIES;
        sp = (sp * 37) % NUM_SPECIES;
        draw_mon(fb_top, sp, 128 - 48, 60, 3, 0, 0);
        top_dirty = 1;
        if (t % 2 == 0) {
            draw_bottom_bg();
            for (int i = 0; i < n; i++) {
                int y = 200 + i * 18 - t / 3;
                if (y < -10 || y > 192) continue;
                draw_text_sh(fb_bot, 128 - text_width(credits[i]) / 2, y, credits[i], i == 0 ? C_YELLOW : C_TEXT, C_BLACK);
            }
            bot_dirty = 1;
        }
        frame();
        if (t > 200 && (key_down & K_START)) break;
    }
    fade_out(3);
}

#ifndef __NDS__
#include <stdlib.h>
void world_debug_dump(int map, const char *path);
static int debug_start(void)
{
    if (getenv("VX_CHECK")) { void world_debug_check(void); world_debug_check(); exit(0); }
    const char *md = getenv("VX_MAPDUMP");
    if (md) {
        for (int i = 0; i < NUM_MAPS; i++) { char p[256]; sprintf(p, "%s/map%02d.ppm", md, i); world_debug_dump(i, p); }
        exit(0);
    }
    const char *t = getenv("VX_TEST");
    if (!t) return 0;
    new_game();
    int map = M_ROST, x = 12, y = 9, lv = 20, sp = 0, flags = 0, ret = M_NEON;
    sscanf(t, "%d,%d,%d,%d,%d,%d,%d", &map, &x, &y, &lv, &sp, &flags, &ret);
    G.ret_map = ret; G.ret_x = 17; G.ret_y = 5;
    G.map = map; G.x = x; G.y = y;
    for (int f = 1; f <= flags; f++) set_flag(f);
    const char *sf = getenv("VX_SETFLAGS");   // e.g. "300-306,310-316"
    while (sf && *sf) {
        int a = 0, b2 = -1, n = 0;
        if (sscanf(sf, "%d-%d%n", &a, &b2, &n) < 2) { sscanf(sf, "%d%n", &a, &n); b2 = a; }
        for (int f = a; f <= b2; f++) set_flag(f);
        sf += n; if (*sf == ',') sf++; else break;
    }
    const char *eg = getenv("VX_EGGS");      // incubator with eggs about to hatch
    if (eg) for (int i = 0; i < atoi(eg) && i < 3; i++) { set_flag(F_KEY_INCUBATOR); G.eggs[i].species = (i * 211) % NUM_SPECIES; G.eggs[i].steps = 3 + i; G.eggs[i].mflags = i == 1; G.eggs[i].iv_bonus = 25; }
    const char *bd = getenv("VX_BADGES");
    if (bd) for (int i = 0; i < atoi(bd); i++) set_flag(F_BADGE_BASE + (i * 5) % NUM_GYMS);
    Mon m;
    for (int i = 0; i < 4; i++) { mon_create(&m, (sp + i * 37) % NUM_SPECIES, lv); give_mon(&m); }
    G.items[IT_KAPSEL] = 10; G.items[IT_KIT] = 5; G.items[IT_SUPERKAPSEL] = 3;
    const char *pm = getenv("VX_PMOVE");     // first party Vexel only knows this move
    if (pm) for (int i = 0; i < 4; i++) { G.party[0].moves[i] = atoi(pm); G.party[0].pp[i] = 30; }
    const char *hl = getenv("VX_HELD");      // held item for every party Vexel
    if (hl) for (int i = 0; i < G.party_n; i++) G.party[i].held = atoi(hl);
    const char *b = getenv("VX_BATTLE");
    if (b) {
        int bsp = 0, blv = 10, tr = -1;
        sscanf(b, "%d,%d,%d", &bsp, &blv, &tr);
        world_warp(G.map, G.x, G.y, 0);
        if (tr >= 0) battle_trainer(tr); else battle_wild(bsp, blv, 1);
    }
    const char *e = getenv("VX_EVO");
    if (e) { G.party[0].level = 60; check_evolutions(); }
    return 1;
}
#endif

#ifdef DS_TEST_BATTLE
static void ds_test(void)
{
    new_game();
    Mon m;
    for (int i = 0; i < 3; i++) { mon_create(&m, 4 + i * 100, 30); give_mon(&m); }
    G.items[IT_KAPSEL] = 10;
    G.map = DS_TEST_MAP; G.x = 14; G.y = 20;
    if (DS_TEST_BATTLE) battle_wild(DS_TEST_BATTLE, 20, 3);
}
#endif

int main(void)
{
    plat_init();
    gfx_init();
    new_game();
    for (int i = 0; i < 3; i++) frame();
#ifdef DS_TEST_BATTLE
    ds_test();
#else
#ifndef __NDS__
    if (!debug_start())
#endif
    play_title();
#endif
    world_run();
    return 0;
}
