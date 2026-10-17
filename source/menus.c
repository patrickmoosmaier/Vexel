// Bottom screen menus: party, bag, dex, shop, storage, save, options.
#include <string.h>
#include "game.h"

static int wait_input_ab(void)
{
    for (;;) {
        frame();
        if (key_down & K_A) return 1;
        if (key_down & K_B) return 0;
        if (touch_down) return 1;
    }
}

static void stat_row(u16 *fb, int x, int y, const char *n, int v, int maxv, u16 c)
{
    char buf[16];
    draw_text_sh(fb, x, y, n, C_TEXT2, C_BLACK);
    fmt(buf, "%d", v);
    draw_text_sh(fb, x + 52, y, buf, C_WHITE, C_BLACK);
    int w = v * 70 / (maxv ? maxv : 1);
    if (w > 70) w = 70;
    fill(fb, x + 74, y + 2, 70, 5, RGB(2, 2, 5));
    fill(fb, x + 74, y + 2, w, 5, c);
}

void mon_summary(Mon *m)
{
    int page = 0;
    for (;;) {
        u16 *fb = fb_bot;
        draw_bottom_bg();
        const SpeciesData *s = &species[m->species];
        panel_solid(fb, 4, 4, 92, 108, lerp_col(type_color(s->type1), C_BLACK, 12, 16), type_color(s->type1));
        draw_mon(fb, m->species, 18, 22, 2, 0, SHINY(m));
        char buf[80];
        draw_text_sh(fb, 100, 6, s->name, C_WHITE, C_BLACK);
        fmt(buf, "Lv %d", m->level);
        draw_text_sh(fb, 220, 6, buf, C_YELLOW, C_BLACK);
        type_badge(fb, 100, 18, s->type1);
        if (s->type2 != s->type1) type_badge(fb, 148, 18, s->type2);
        fmt(buf, "%s", tier_names[s->tier]);
        draw_text_sh(fb, 198, 19, buf, C_YELLOW, C_BLACK);
        if (page == 0) {
            fmt(buf, "KP %d/%d", m->hp, m->maxhp);
            draw_text_sh(fb, 100, 34, buf, C_WHITE, C_BLACK);
            hpbar(fb, 170, 36, 76, m->hp, m->maxhp);
            stat_row(fb, 100, 48, "Angriff", m->atk, 250, RGB(31, 10, 6));
            stat_row(fb, 100, 60, "Verteid.", m->def, 250, RGB(31, 26, 4));
            stat_row(fb, 100, 72, "Technik", m->tec, 250, RGB(10, 18, 31));
            stat_row(fb, 100, 84, "Initiat.", m->spd, 250, RGB(6, 28, 12));
            fmt(buf, "%s  ·  %s", attr_names[s->attr], abilities[s->ability].name);
            draw_text_sh(fb, 100, 96, buf, C_CYAN, C_BLACK);
            char nb[48];
            draw_text(fb, 100, 106, nature_desc(m->nature, nb), C_TEXT2);
            if (m->mflags & MF_SHINY) draw_text_sh(fb, 8, 8, "* Schillernd", C_YELLOW, C_BLACK);
            if (m->held) draw_text_sh(fb, 8, 98, held_items[m->held].name, C_YELLOW, C_BLACK);
            // moves
            for (int i = 0; i < 4; i++) {
                int x = 4 + (i % 2) * 126, y = 116 + (i / 2) * 26;
                int mv = m->moves[i];
                if (mv == NO_MOVE) { panel_solid(fb, x, y, 122, 24, RGB(2, 2, 4), RGB(6, 6, 8)); continue; }
                panel_solid(fb, x, y, 122, 24, lerp_col(type_color(moves[mv].type), C_BLACK, 11, 16), type_color(moves[mv].type));
                draw_text_sh(fb, x + 6, y + 3, moves[mv].name, C_WHITE, C_BLACK);
                fmt(buf, "%s  St.%d  AP %d/%d", type_names[moves[mv].type], moves[mv].power, m->pp[i], moves[mv].pp);
                draw_text(fb, x + 6, y + 13, buf, C_TEXT2);
            }
        } else {
            panel_solid(fb, 100, 34, 152, 76, RGB(2, 2, 6), C_CYAN);
            draw_text_wrap(fb, 106, 38, 142, s->dex, C_TEXT, C_BLACK, -1, 10);
            fmt(buf, "%s: %s", abilities[s->ability].name, abilities[s->ability].desc);
            draw_text_wrap(fb, 106, 80, 142, buf, C_YELLOW, C_BLACK, -1, 10);
            // evolution chain
            const LineData *L = &mlines[s->line];
            panel_solid(fb, 4, 116, 248, 54, RGB(2, 2, 6), C_MAGENTA);
            for (int k = 0; k < L->nstages; k++) {
                int sp = L->first + k;
                int x = 8 + k * 41;
                if (is_seen(sp)) draw_mon(fb, sp, x + 2, 118, 1, 0, 0);
                else draw_mon(fb, sp, x + 2, 118, 1, 0, 2);
                if (k < L->nstages - 1) {
                    fmt(buf, "%d", species[sp].evo);
                    draw_text(fb, x + 34, 128, "→", C_CYAN);
                    draw_text(fb, x + 33, 140, buf, C_YELLOW);
                }
            }
            draw_text(fb, 10, 158, "Evolutionslinie (Level)", C_TEXT2);
        }
        button(fb, 4, 172, 122, 18, page ? "< Werte" : "Info >", C_CYAN, 0);
        button(fb, 130, 172, 122, 18, "Zurück (B)", C_GREY, 0);
        bot_dirty = 1;
        for (;;) {
            frame();
            if ((key_down & (K_LEFT | K_RIGHT | K_A)) || touched(4, 172, 122, 18)) { page ^= 1; sfx(SFX_SELECT); break; }
            if ((key_down & K_B) || touched(130, 172, 122, 18)) { sfx(SFX_CANCEL); return; }
        }
    }
}

// choose a held item from the inventory (0 = cancel)
static int held_pick(void)
{
    const char *opts[NUM_HELD];
    static char rows[NUM_HELD][40];
    int ids[NUM_HELD], n = 0;
    for (int h = 1; h < NUM_HELD; h++)
        if (G.held_inv[h - 1]) { fmt(rows[n], "%s x%d", held_items[h].name, G.held_inv[h - 1]); opts[n] = rows[n]; ids[n++] = h; }
    if (!n) { say(NULL, "Du hast keine tragbaren Items. Händler in den Ladestationen verkaufen welche."); return 0; }
    int r = menu_list("Welches Item?", opts, n, 0);
    return r < 0 ? 0 : ids[r];
}

static void give_held(Mon *m, int h)
{
    char buf[120];
    if (m->held && G.held_inv[m->held - 1] < 255) G.held_inv[m->held - 1]++;
    G.held_inv[h - 1]--;
    m->held = h;
    fmt(buf, "%s trägt jetzt %s.", species[m->species].name, held_items[h].name);
    sfx(SFX_CONFIRM);
    say(NULL, buf);
}

// mode 0 view/reorder, 1 battle switch select, 2 item target select
int party_menu(int mode, int cur)
{
    int swap_from = -1;
    if (G.party_n == 0) return -1;
    if (cur >= G.party_n) cur = 0;
    for (;;) {
        u16 *fb = fb_bot;
        draw_bottom_bg();
        for (int i = 0; i < 6; i++) {
            int x = 4 + (i % 2) * 126, y = 4 + (i / 2) * 52;
            if (i >= G.party_n) { panel_solid(fb, x, y, 122, 50, RGB(2, 2, 4), RGB(5, 5, 8)); continue; }
            Mon *m = &G.party[i];
            u16 bc = i == swap_from ? C_YELLOW : (i == cur ? C_WHITE : (m->hp ? C_CYAN : C_RED));
            panel_solid(fb, x, y, 122, 50, i == cur ? RGB(6, 8, 18) : RGB(3, 3, 9), bc);
            draw_mon(fb, m->species, x + 2, y + 9, 1, 0, (m->hp ? 0 : 2) | SHINY(m));
            draw_text_sh(fb, x + 36, y + 5, species[m->species].name, C_WHITE, C_BLACK);
            char buf[32];
            fmt(buf, "Lv%d", m->level);
            draw_text_sh(fb, x + 36, y + 17, buf, C_YELLOW, C_BLACK);
            if (STATUS(m)) draw_text_sh(fb, x + 70, y + 17, status_short[STATUS(m)], C_RED, C_BLACK);
            if (m->held) draw_text_sh(fb, x + 110, y + 5, "+", C_YELLOW, C_BLACK);
            hpbar(fb, x + 36, y + 30, 80, m->hp, m->maxhp);
            fmt(buf, "%d/%d", m->hp, m->maxhp);
            draw_text(fb, x + 116 - text_width(buf), y + 38, buf, C_TEXT2);
        }
        const char *hint = mode == 0 ? (swap_from >= 0 ? "Wähle Ziel zum Tauschen" : "A: Optionen  B: Zurück") : mode == 1 ? "Welcher Vexel?" : "Auf wen anwenden?";
        panel_solid(fb, 4, 160, 180, 28, RGB(2, 2, 6), C_MAGENTA);
        draw_text_sh(fb, 12, 169, hint, C_TEXT, C_BLACK);
        button(fb, 188, 160, 64, 28, "Zurück", C_GREY, 0);
        bot_dirty = 1;
        int pick = -1;
        for (;;) {
            frame();
            int moved = 0;
            if (key_rep & K_LEFT) { cur = (cur & ~1) | ((cur & 1) ^ 1); moved = 1; }
            if (key_rep & K_RIGHT) { cur = (cur & ~1) | ((cur & 1) ^ 1); moved = 1; }
            if (key_rep & K_UP) { cur = (cur + 4) % 6; moved = 1; }
            if (key_rep & K_DOWN) { cur = (cur + 2) % 6; moved = 1; }
            if (moved) { if (cur >= G.party_n) cur = G.party_n - 1; sfx(SFX_SELECT); break; }
            if (key_down & K_A) { pick = cur; break; }
            for (int i = 0; i < G.party_n; i++) if (touched(4 + (i % 2) * 126, 4 + (i / 2) * 52, 122, 50)) { pick = i; cur = i; }
            if (pick >= 0) break;
            if ((key_down & K_B) || touched(188, 160, 64, 28)) {
                sfx(SFX_CANCEL);
                if (swap_from >= 0) { swap_from = -1; break; }
                return -1;
            }
        }
        if (pick < 0) continue;
        sfx(SFX_CONFIRM);
        if (mode != 0) return pick;
        if (swap_from >= 0) {
            Mon t = G.party[swap_from]; G.party[swap_from] = G.party[pick]; G.party[pick] = t;
            swap_from = -1;
            continue;
        }
        Mon *pm = &G.party[pick];
        const char *opts[4] = { "Status", pm->held ? "Item nehmen" : "Item geben", "Position tauschen", "Zurück" };
        int r = menu_list(species[pm->species].name, opts, 4, 0);
        if (r == 0) mon_summary(pm);
        else if (r == 1) {
            char buf[100];
            if (pm->held) {
                if (G.held_inv[pm->held - 1] < 255) G.held_inv[pm->held - 1]++;
                fmt(buf, "Du nimmst %s das Item %s ab.", species[pm->species].name, held_items[pm->held].name);
                pm->held = 0;
                say(NULL, buf);
            } else {
                int h = held_pick();
                if (h > 0) give_held(pm, h);
            }
        }
        else if (r == 2) swap_from = pick;
    }
}

int bag_menu(int in_battle)
{
    int ids[NUM_ITEMS + NUM_HELD], n = 0;
    for (int i = 0; i < NUM_ITEMS; i++) if (G.items[i] > 0) ids[n++] = i;
    if (!in_battle) for (int h = 1; h < NUM_HELD; h++) if (G.held_inv[h - 1]) ids[n++] = 100 + h;
    if (n == 0) {
        draw_bottom_bg();
        panel(fb_bot, 20, 70, 216, 40, C_CYAN);
        draw_text_sh(fb_bot, 128 - text_width("Die Tasche ist leer.") / 2, 85, "Die Tasche ist leer.", C_TEXT, C_BLACK);
        bot_dirty = 1;
        wait_input_ab();
        return -1;
    }
    int cur = 0, scroll = 0;
    for (;;) {
        u16 *fb = fb_bot;
        draw_bottom_bg();
        panel_solid(fb, 4, 4, 248, 18, RGB(3, 3, 9), C_MAGENTA);
        draw_text_sh(fb, 12, 8, "TASCHE", C_YELLOW, C_BLACK);
        char buf[64];
        fmt(buf, "%d Credits", (int)G.money);
        draw_text_sh(fb, 244 - text_width(buf), 8, buf, C_YELLOW, C_BLACK);
        if (cur < scroll) scroll = cur;
        if (cur >= scroll + 6) scroll = cur - 5;
        for (int i = 0; i < 6 && scroll + i < n; i++) {
            int it = ids[scroll + i];
            int y = 26 + i * 22;
            button(fb, 4, y, 248, 20, "", C_CYAN, scroll + i == cur);
            if (it >= 100) {
                fill(fb, 10, y + 6, 9, 8, C_YELLOW); fill(fb, 12, y + 8, 5, 4, C_MAGENTA);
                draw_text_sh(fb, 26, y + 5, held_items[it - 100].name, C_WHITE, C_BLACK);
                draw_text(fb, 150, y + 6, "(Tragbar)", C_TEXT2);
                fmt(buf, "x%d", G.held_inv[it - 101]);
                draw_text_sh(fb, 240 - text_width(buf), y + 5, buf, C_YELLOW, C_BLACK);
                continue;
            }
            if (it <= IT_OMEGAKAPSEL) draw_capsule_icon(fb, 10, y + 5, it);
            else { fill(fb, 10, y + 6, 9, 8, it == IT_REBOOT ? C_YELLOW : it == IT_ANTIVIRUS ? C_GREEN : it >= IT_SCHUTZ ? C_CYAN : C_RED); fill(fb, 13, y + 7, 3, 6, C_WHITE); }
            draw_text_sh(fb, 26, y + 5, items[it].name, C_WHITE, C_BLACK);
            fmt(buf, "x%d", G.items[it]);
            draw_text_sh(fb, 240 - text_width(buf), y + 5, buf, C_YELLOW, C_BLACK);
        }
        panel_solid(fb, 4, 160, 248, 28, RGB(2, 2, 6), C_CYAN);
        draw_text_wrap(fb, 10, 163, 236, ids[cur] >= 100 ? held_items[ids[cur] - 100].desc : items[ids[cur]].desc, C_TEXT, C_BLACK, -1, 11);
        bot_dirty = 1;
        for (;;) {
            frame();
            if (key_rep & K_UP) { cur = (cur + n - 1) % n; sfx(SFX_SELECT); break; }
            if (key_rep & K_DOWN) { cur = (cur + 1) % n; sfx(SFX_SELECT); break; }
            int pick = -1;
            if (key_down & K_A) pick = cur;
            for (int i = 0; i < 6 && scroll + i < n; i++) if (touched(4, 26 + i * 22, 248, 20)) pick = scroll + i;
            if (key_down & K_B) { sfx(SFX_CANCEL); return -1; }
            if (pick >= 0) {
                cur = pick;
                int it = ids[pick];
                sfx(SFX_CONFIRM);
                if (in_battle) return it;
                if (it >= 100) {   // give a held item
                    int t = party_menu(2, 0);
                    if (t < 0) break;
                    give_held(&G.party[t], it - 100);
                    int left = 0;
                    for (int h = 1; h < NUM_HELD; h++) left += G.held_inv[h - 1];
                    for (int i = 0; i < NUM_ITEMS; i++) left += G.items[i];
                    if (!left) return -1;
                    return bag_menu(0);
                }
                // field use
                if (it <= IT_OMEGAKAPSEL) { say(NULL, "Kapseln kannst du nur im Kampf gegen wilde Vexel einsetzen."); break; }
                if (it == IT_SCHUTZ || it == IT_SUPERSCHUTZ) {
                    if (G.repel) { say(NULL, "Es ist noch ein Schutzfeld aktiv."); break; }
                    G.items[it]--;
                    G.repel = it == IT_SCHUTZ ? 100 : 250;
                    sfx(SFX_HEAL);
                    say(NULL, "Ein Schutzfeld umgibt dich. Schwache wilde Vexel halten Abstand.");
                    if (G.items[it] == 0) return -1;
                    break;
                }
                int t = party_menu(2, 0);
                if (t < 0) break;
                Mon *m = &G.party[t];
                int used = 0;
                if (it == IT_KIT || it == IT_SUPERKIT || it == IT_MAXKIT) {
                    if (m->hp > 0 && m->hp < m->maxhp) { int h = it == IT_KIT ? 30 : it == IT_SUPERKIT ? 80 : 9999; m->hp = m->hp + h > m->maxhp ? m->maxhp : m->hp + h; used = 1; }
                } else if (it == IT_REBOOT) { if (m->hp == 0) { m->hp = m->maxhp / 2; used = 1; } }
                else if (it == IT_ANTIVIRUS) { if (m->status) { m->status = 0; used = 1; } }
                else if (it == IT_ZELLE) { for (int i = 0; i < 4; i++) if (m->moves[i] != NO_MOVE) m->pp[i] = moves[m->moves[i]].pp; used = 1; }
                if (used) {
                    G.items[it]--;
                    sfx(SFX_HEAL);
                    fmt(buf, "%s wurde bei %s angewendet.", items[it].name, species[m->species].name);
                    say(NULL, buf);
                    if (G.items[it] == 0) return -1;
                } else say(NULL, "Das hätte keine Wirkung.");
                break;
            }
        }
    }
}

void dex_menu(void)
{
    int cur = 0;
    for (;;) {
        u16 *fb = fb_bot;
        draw_bottom_bg();
        panel_solid(fb, 4, 4, 248, 18, RGB(3, 3, 9), C_MAGENTA);
        char buf[80];
        fmt(buf, "VEXIKON   Gesehen %d   Gefangen %d / %d", count_seen(), count_caught(), NUM_SPECIES);
        draw_text_sh(fb, 12, 8, buf, C_YELLOW, C_BLACK);
        // grid 6x4 icons
        int page0 = (cur / 24) * 24;
        for (int i = 0; i < 24; i++) {
            int sp = page0 + i;
            if (sp >= NUM_SPECIES) break;
            int x = 4 + (i % 6) * 42, y = 26 + (i / 6) * 36;
            u16 bc = sp == cur ? C_WHITE : (is_caught(sp) ? C_CYAN : RGB(6, 7, 12));
            panel_solid(fb, x, y, 40, 34, sp == cur ? RGB(6, 8, 18) : RGB(2, 2, 6), bc);
            if (is_seen(sp)) draw_mon(fb, sp, x + 4, y + 1, 1, 0, is_caught(sp) ? 0 : 2);
            else draw_text(fb, x + 16, y + 12, "?", RGB(8, 8, 12));
            fmt(buf, "%d", sp + 1);
            draw_text(fb, x + 3, y + 24, buf, RGB(14, 16, 22));
            if (is_caught(sp)) draw_capsule_icon(fb, x + 30, y + 24, 0);
        }
        panel_solid(fb, 4, 172, 248, 18, RGB(2, 2, 6), C_CYAN);
        if (is_seen(cur)) fmt(buf, "#%d %s  (L/R: Seite, A: Details)", cur + 1, species[cur].name);
        else fmt(buf, "#%d ???  (L/R: Seite)", cur + 1);
        draw_text_sh(fb, 10, 176, buf, C_TEXT, C_BLACK);
        bot_dirty = 1;
        for (;;) {
            frame();
            int moved = 0;
            if (key_rep & K_LEFT) { cur--; moved = 1; }
            if (key_rep & K_RIGHT) { cur++; moved = 1; }
            if (key_rep & K_UP) { cur -= 6; moved = 1; }
            if (key_rep & K_DOWN) { cur += 6; moved = 1; }
            if (key_rep & K_L) { cur -= 24; moved = 1; }
            if (key_rep & K_R) { cur += 24; moved = 1; }
            if (moved) { cur = (cur + NUM_SPECIES) % NUM_SPECIES; sfx(SFX_SELECT); break; }
            if (key_down & K_B) { sfx(SFX_CANCEL); return; }
            int pick = -1;
            if (key_down & K_A) pick = cur;
            for (int i = 0; i < 24; i++) if (touched(4 + (i % 6) * 42, 26 + (i / 6) * 36, 40, 34)) pick = page0 + i;
            if (touched(4, 172, 124, 18)) { cur = (cur + NUM_SPECIES - 24) % NUM_SPECIES; break; }
            if (touched(128, 172, 124, 18)) { cur = (cur + 24) % NUM_SPECIES; break; }
            if (pick >= 0 && pick < NUM_SPECIES) {
                if (pick != cur) { cur = pick; break; }
                if (!is_seen(cur)) { sfx(SFX_BUMP); break; }
                // details on top screen
                sfx(SFX_CONFIRM);
                top_snapshot();
                u16 *ft = fb_top;
                const SpeciesData *s = &species[cur];
                gradient_v(ft, 0, 0, SCR_W, SCR_H, lerp_col(type_color(s->type1), C_BLACK, 12, 16), RGB(1, 1, 4));
                for (int y = 0; y < SCR_H; y += 8) fill_blend(ft, 0, y, SCR_W, 1, C_CYAN, 2);
                draw_mon(ft, cur, 4, 20, 4, 0, is_caught(cur) ? 0 : 2);
                panel_solid(ft, 136, 8, 116, 40, RGB(3, 3, 9), type_color(s->type1));
                fmt(buf, "#%d", cur + 1);
                draw_text_sh(ft, 142, 12, buf, C_YELLOW, C_BLACK);
                draw_text_sh(ft, 142, 24, s->name, C_WHITE, C_BLACK);
                fmt(buf, "%s · %s", tier_names[s->tier], attr_names[s->attr]);
                draw_text(ft, 142, 36, buf, C_TEXT2);
                type_badge(ft, 140, 54, s->type1);
                if (s->type2 != s->type1) type_badge(ft, 188, 54, s->type2);
                fmt(buf, "Größe %d.%d m", s->height / 10, s->height % 10);
                draw_text_sh(ft, 140, 72, buf, C_TEXT, C_BLACK);
                fmt(buf, "Gewicht %d.%d kg", s->weight / 10, s->weight % 10);
                draw_text_sh(ft, 140, 84, buf, C_TEXT, C_BLACK);
                if (is_caught(cur)) {
                    const char *nm[5] = { "KP", "ANG", "VER", "TEC", "INI" };
                    for (int k = 0; k < 5; k++) {
                        draw_text(ft, 140, 100 + k * 9, nm[k], C_TEXT2);
                        fill(ft, 164, 102 + k * 9, s->base[k] * 86 / 255, 4, type_color(s->type1));
                    }
                }
                panel_solid(ft, 4, 150, 248, 38, RGB(2, 2, 6), C_CYAN);
                draw_text_wrap(ft, 10, 155, 236, is_caught(cur) ? s->dex : "Fange diesen Vexel, um mehr Daten zu erhalten.", C_TEXT, C_BLACK, -1, 11);
                top_dirty = 1;
                wait_input_ab();
                top_restore();
                break;
            }
        }
    }
}

void shop_menu(void)
{
    static const int stock_base[] = { IT_KAPSEL, IT_KIT, IT_ANTIVIRUS };
    static const int stock_mid[] = { IT_KAPSEL, IT_SUPERKAPSEL, IT_KIT, IT_SUPERKIT, IT_ANTIVIRUS, IT_REBOOT, IT_SCHUTZ };
    static const int stock_hi[] = { IT_KAPSEL, IT_SUPERKAPSEL, IT_HYPERKAPSEL, IT_KIT, IT_SUPERKIT, IT_MAXKIT, IT_ANTIVIRUS, IT_REBOOT, IT_ZELLE, IT_SCHUTZ, IT_SUPERSCHUTZ };
    const int *stock; int n;
    if (badge_count() >= 4) { stock = stock_hi; n = 11; }
    else if (badge_count() >= 1) { stock = stock_mid; n = 7; }
    else { stock = stock_base; n = 3; }
    int cur = 0;
    for (;;) {
        u16 *fb = fb_bot;
        draw_bottom_bg();
        panel_solid(fb, 4, 4, 248, 18, RGB(3, 3, 9), C_MAGENTA);
        char buf[64];
        draw_text_sh(fb, 12, 8, "HÄNDLER-BOT", C_YELLOW, C_BLACK);
        fmt(buf, "%d Credits", (int)G.money);
        draw_text_sh(fb, 244 - text_width(buf), 8, buf, C_YELLOW, C_BLACK);
        for (int i = 0; i < n; i++) {
            int y = 25 + i * (n > 9 ? 12 : 15);
            int it = stock[i];
            if (i == cur) fill(fb, 4, y, 248, n > 9 ? 12 : 14, RGB(6, 8, 18));
            if (i == cur) draw_text(fb, 8, y + 2, "▶", C_CYAN);
            draw_text_sh(fb, 18, y + 2, items[it].name, C_WHITE, C_BLACK);
            fmt(buf, "%d Cr", items[it].price);
            draw_text_sh(fb, 180 - text_width(buf), y + 2, buf, C_YELLOW, C_BLACK);
            fmt(buf, "Besitz %d", G.items[it]);
            draw_text(fb, 246 - text_width(buf), y + 2, buf, C_TEXT2);
        }
        panel_solid(fb, 4, 164, 248, 24, RGB(2, 2, 6), C_CYAN);
        draw_text_sh(fb, 10, 171, items[stock[cur]].desc, C_TEXT, C_BLACK);
        bot_dirty = 1;
        for (;;) {
            frame();
            if (key_rep & K_UP) { cur = (cur + n - 1) % n; sfx(SFX_SELECT); break; }
            if (key_rep & K_DOWN) { cur = (cur + 1) % n; sfx(SFX_SELECT); break; }
            if (key_down & K_B) { sfx(SFX_CANCEL); return; }
            int pick = -1;
            if (key_down & K_A) pick = cur;
            for (int i = 0; i < n; i++) if (touched(4, 25 + i * (n > 9 ? 12 : 15), 248, 12)) pick = i;
            if (pick >= 0) {
                if (pick != cur) { cur = pick; break; }
                int it = stock[cur];
                // quantity selection
                int q = 1;
                for (;;) {
                    draw_bottom_bg();
                    panel_solid(fb, 30, 50, 196, 90, RGB(3, 3, 9), C_YELLOW);
                    draw_text_sh(fb, 40, 58, items[it].name, C_WHITE, C_BLACK);
                    fmt(buf, "Menge: < %d >", q);
                    draw_text_sh(fb, 40, 76, buf, C_CYAN, C_BLACK);
                    fmt(buf, "Preis: %d Cr", q * items[it].price);
                    draw_text_sh(fb, 40, 90, buf, C_YELLOW, C_BLACK);
                    draw_text(fb, 40, 112, "A: Kaufen   B: Abbrechen", C_TEXT2);
                    draw_text(fb, 40, 124, "Links/Rechts: Menge", C_TEXT2);
                    bot_dirty = 1;
                    frame();
                    if (key_rep & (K_RIGHT | K_UP)) { q = q < 99 ? q + 1 : 99; sfx(SFX_SELECT); }
                    if (key_rep & (K_LEFT | K_DOWN)) { q = q > 1 ? q - 1 : 1; sfx(SFX_SELECT); }
                    if (touch_down && tx > 128 && ty < 120) q = q < 99 ? q + 1 : 99;
                    if (touch_down && tx <= 128 && ty < 120) q = q > 1 ? q - 1 : 1;
                    if (key_down & K_B) break;
                    if ((key_down & K_A) || (touch_down && ty >= 120)) {
                        u32 cost = (u32)q * items[it].price;
                        if (cost > G.money) { sfx(SFX_BUMP); say("HÄNDLER-BOT", "FEHLER: NICHT GENUG CREDITS."); }
                        else {
                            G.money -= cost; G.items[it] += q;
                            sfx(SFX_SAVE);
                            if (it == IT_KAPSEL && q >= 10) { G.items[IT_SUPERKAPSEL]++; say("HÄNDLER-BOT", "DANKE. BONUS: 1 SUPERKAPSEL GRATIS."); }
                            else say("HÄNDLER-BOT", "TRANSAKTION ERFOLGREICH. BEEP.");
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

// Ausrüster: sells held items (stock grows with badges)
void held_shop_menu(void)
{
    static const u8 stock[] = { HI_DATENBEERE, HI_KLARBEERE, HI_NEBELKAPSEL, HI_STASISKERN, HI_ZIELCHIP, HI_TURBOCHIP, HI_KRAFTMODUL,
                                HI_TECHNIKMODUL, HI_KLIMAKERN, HI_NOTFALLCHIP, HI_SAUGMODUL, HI_PANZERWESTE, HI_NANOREP,
                                HI_CREDITCHIP, HI_LERNCHIP, HI_UEBERLAST };
    int b = badge_count();
    int n = 4 + b;   // 4 at the start, all 16 from 12 badges on
    if (flag(F_CHAMPION) || n > 16) n = 16;
    const char *opts[16];
    static char rows[16][48];
    for (;;) {
        for (int i = 0; i < n; i++) { fmt(rows[i], "%s  %d Cr  (x%d)", held_items[stock[i]].name, held_items[stock[i]].price, G.held_inv[stock[i] - 1]); opts[i] = rows[i]; }
        draw_bottom_bg();
        int r = menu_list("AUSRÜSTER", opts, n, 0);
        if (r < 0) return;
        int h = stock[r];
        char buf[160];
        fmt(buf, "%s: %s Kaufen für %d Credits?", held_items[h].name, held_items[h].desc, held_items[h].price);
        if (!ask_yesno("Ausrüster", buf)) continue;
        if (G.money < held_items[h].price) { sfx(SFX_BUMP); say("Ausrüster", "Dafür reichen deine Credits nicht."); continue; }
        if (G.held_inv[h - 1] >= 99) { say("Ausrüster", "Davon hast du schon genug."); continue; }
        G.money -= held_items[h].price;
        G.held_inv[h - 1]++;
        sfx(SFX_SAVE);
        say("Ausrüster", "Danke! Gib es einem Vexel im Team-Menü oder in der Tasche.");
    }
}

void box_menu(void)
{
    static const char *const opts[] = { "Vexel ablegen", "Vexel holen", "Abbrechen" };
    for (;;) {
        int r = menu_list("CLOUD-SPEICHER", opts, 3, 0);
        if (r < 0 || r == 2) return;
        if (r == 0) {
            if (G.party_n <= 1) { say(NULL, "Du brauchst mindestens einen Vexel im Team!"); continue; }
            if (G.box_n >= BOX_MAX) { say(NULL, "Der Cloud-Speicher ist voll."); continue; }
            int t = party_menu(1, 0);
            if (t < 0) continue;
            G.box[G.box_n++] = G.party[t];
            mon_heal(&G.box[G.box_n - 1]);
            for (int i = t; i < G.party_n - 1; i++) G.party[i] = G.party[i + 1];
            G.party_n--;
            say(NULL, "Der Vexel wurde in die Cloud hochgeladen.");
        } else {
            if (G.box_n == 0) { say(NULL, "Der Cloud-Speicher ist leer."); continue; }
            if (G.party_n >= PARTY_MAX) { say(NULL, "Dein Team ist voll!"); continue; }
            int cur = 0;
            for (;;) {
                u16 *fb = fb_bot;
                draw_bottom_bg();
                panel_solid(fb, 4, 4, 248, 18, RGB(3, 3, 9), C_MAGENTA);
                char buf[64];
                fmt(buf, "CLOUD  %d/%d", G.box_n, BOX_MAX);
                draw_text_sh(fb, 12, 8, buf, C_YELLOW, C_BLACK);
                int p0 = (cur / 24) * 24;
                for (int i = 0; i < 24 && p0 + i < G.box_n; i++) {
                    Mon *m = &G.box[p0 + i];
                    int x = 4 + (i % 6) * 42, y = 26 + (i / 6) * 36;
                    panel_solid(fb, x, y, 40, 34, p0 + i == cur ? RGB(6, 8, 18) : RGB(2, 2, 6), p0 + i == cur ? C_WHITE : C_CYAN);
                    draw_mon(fb, m->species, x + 4, y + 1, 1, 0, SHINY(m));
                    fmt(buf, "%d", m->level);
                    draw_text(fb, x + 3, y + 24, buf, C_YELLOW);
                }
                Mon *m = &G.box[cur];
                panel_solid(fb, 4, 172, 248, 18, RGB(2, 2, 6), C_CYAN);
                fmt(buf, "%s  Lv%d   A: Holen  B: Zurück", species[m->species].name, m->level);
                draw_text_sh(fb, 10, 176, buf, C_TEXT, C_BLACK);
                bot_dirty = 1;
                frame();
                int moved = 0;
                if (key_rep & K_LEFT) { cur--; moved = 1; }
                if (key_rep & K_RIGHT) { cur++; moved = 1; }
                if (key_rep & K_UP) { cur -= 6; moved = 1; }
                if (key_rep & K_DOWN) { cur += 6; moved = 1; }
                if (moved) { cur = (cur + G.box_n * 6) % G.box_n; sfx(SFX_SELECT); continue; }
                if (key_down & K_B) break;
                int pick = -1;
                if (key_down & K_A) pick = cur;
                for (int i = 0; i < 24 && p0 + i < G.box_n; i++) if (touched(4 + (i % 6) * 42, 26 + (i / 6) * 36, 40, 34)) pick = p0 + i;
                if (pick >= 0) {
                    if (pick != cur) { cur = pick; continue; }
                    G.party[G.party_n++] = G.box[cur];
                    for (int i = cur; i < G.box_n - 1; i++) G.box[i] = G.box[i + 1];
                    G.box_n--;
                    sfx(SFX_CONFIRM);
                    say(NULL, "Der Vexel wurde in dein Team heruntergeladen.");
                    break;
                }
            }
        }
    }
}

static void options_menu(void)
{
    int cur = 0;
    for (;;) {
        char a[40], b[40], c[40], d[48];
        static const char *const sp[] = { "Langsam", "Normal", "Schnell" };
        fmt(a, "Textgeschwindigkeit: %s", sp[G.textspeed % 3]);
        fmt(b, "Musik: %s", G.music_on ? "An" : "Aus");
        fmt(c, "Partner folgt dir: %s", G.follow ? "An" : "Aus");
        fmt(d, "Tageszeit: %s", G.clock_off ? "Immer Tag" : "DS-Uhr");
        const char *opts[5] = { a, b, c, d, "Zurück" };
        int r = menu_list("OPTIONEN", opts, 5, cur);
        if (r >= 0) cur = r;
        if (r == 0) G.textspeed = (G.textspeed + 1) % 3;
        else if (r == 1) { G.music_on ^= 1; if (!G.music_on) { plat_tone_off(0); plat_tone_off(1); plat_tone_off(2); } }
        else if (r == 2) G.follow ^= 1;
        else if (r == 3) G.clock_off ^= 1;
        else return;
    }
}

// Trainerpass: overview of the player's progress
static void trainer_card(void)
{
    u16 *fb = fb_bot;
    char buf[64];
    draw_bottom_bg();
    panel_solid(fb, 6, 6, 244, 180, RGB(3, 4, 12), flag(F_CHAMPION) ? C_YELLOW : C_CYAN);
    gradient_v(fb, 8, 8, 240, 22, RGB(10, 4, 18), RGB(3, 3, 10));
    draw_text_sh(fb, 14, 13, "TRAINERPASS", C_YELLOW, C_BLACK);
    draw_text_sh(fb, 150, 13, flag(F_CHAMPION) ? "CHAMPION" : "KURIER", flag(F_CHAMPION) ? C_YELLOW : C_CYAN, C_BLACK);
    fill(fb, 14, 38, 50, 66, RGB(2, 2, 6));
    draw_chr(fb, CH_PLAYER, 0, 31, 56, 0);
    draw_text_sh(fb, 74, 38, "Name: Nova Voss", C_WHITE, C_BLACK);
    fmt(buf, "Credits: %d", (int)G.money);
    draw_text_sh(fb, 74, 52, buf, C_TEXT, C_BLACK);
    int secs = G.playtime / 60;
    fmt(buf, "Spielzeit: %d:%s%d", secs / 3600, ((secs / 60) % 60) < 10 ? "0" : "", (secs / 60) % 60);
    draw_text_sh(fb, 74, 66, buf, C_TEXT, C_BLACK);
    fmt(buf, "Vexikon: %d gesehen, %d gefangen", count_seen(), count_caught());
    draw_text_sh(fb, 74, 80, buf, C_TEXT, C_BLACK);
    int shiny = 0;
    for (int i = 0; i < G.party_n; i++) shiny += (G.party[i].mflags & MF_SHINY) != 0;
    for (int i = 0; i < G.box_n; i++) shiny += (G.box[i].mflags & MF_SHINY) != 0;
    fmt(buf, "Schillernde Vexel: %d", shiny);
    draw_text_sh(fb, 74, 94, buf, C_TEXT, C_BLACK);
    fmt(buf, "Orden: %d/24", badge_count());
    draw_text_sh(fb, 14, 112, buf, C_TEXT2, C_BLACK);
    for (int i = 0; i < 24; i++) {
        int x = 14 + (i % 12) * 19, y = 126 + (i / 12) * 16;
        u16 c = flag(F_BADGE_BASE + i) ? type_color(gyms[i].t1) : RGB(5, 5, 9);
        fill(fb, x, y, 15, 12, RGB(1, 1, 3));
        fill(fb, x + 2, y + 2, 11, 8, c);
    }
    int leg = 0;
    for (int i = 0; i < NUM_LEGENDS; i++) leg += flag(F_LEG_CAUGHT_BASE + i);
    if (flag(F_CHAMPION)) fmt(buf, "Titelverteidigungen: %d   Urcodes: %d/8", G.defenses, leg);
    else fmt(buf, "Hoher Rat besiegt: %d/4", flag(F_ELITE_BASE) + flag(F_ELITE_BASE + 1) + flag(F_ELITE_BASE + 2) + flag(F_ELITE_BASE + 3));
    draw_text_sh(fb, 14, 164, buf, C_YELLOW, C_BLACK);
    bot_dirty = 1;
    wait_input_ab();
}

void menu_main(void)
{
    static int cur = 0;
    for (;;) {
        static const char *const opts[] = { "VEXEL-TEAM", "TASCHE", "AUSRÜSTUNG", "VEXIKON", "TRAINERPASS", "SPEICHERN", "OPTIONEN", "ZURÜCK" };
        draw_bottom_bg();
        bot_dirty = 1;
        int r = menu_list("MENÜ", opts, 8, cur);
        if (r < 0 || r == 7) return;
        cur = r;
        if (r == 0) party_menu(0, 0);
        else if (r == 1) bag_menu(0);
        else if (r == 2) { int equipment_menu(void); if (equipment_menu()) return; }
        else if (r == 3) dex_menu();
        else if (r == 4) trainer_card();
        else if (r == 5) {
            if (ask_yesno(NULL, "Spielstand speichern?")) {
                sfx(SFX_SAVE);
                if (save_game()) say(NULL, "Gespeichert! Dein Fortschritt ist sicher.");
                else say(NULL, "Speichern fehlgeschlagen: kein Speicherchip gefunden. Tipp: Nutze die Speicherstände (Save States) deines Emulators.");
            }
        } else if (r == 6) options_menu();
    }
}
