// Overworld: maps, movement, NPCs, events and story scripts.
#include <string.h>
#include "game.h"
#include "gen_maps.h"
#ifndef __NDS__
#include <stdio.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

enum { NK_TALK, NK_TRAINER, NK_NURSE, NK_SHOP, NK_BOX, NK_SCRIPT, NK_SIGN, NK_ITEM, NK_GYM, NK_GYMTR, NK_ELITE, NK_LEGEND, NK_CHALLENGER };
enum { D_DOWN, D_UP, D_LEFT, D_RIGHT };
#define NOF -1

typedef struct { u8 x, y, w, h, map, tx, ty, dir; s16 req; u8 gym; } Warp;   // gym: 0 none, else gym+1
typedef struct { u8 x, y, chr, dir, kind, arg; s16 req, hide; const char *name; const char *text; } NpcDef;
typedef struct { u8 x, y, w, h; s16 req, hide; u8 script; } Trigger;
typedef struct {
    const char *name; char outside; s8 zone; u8 music; u8 bg; u8 enc_any;
    const Warp *warps; u8 nwarps; const NpcDef *npcs; u8 nnpcs; const Trigger *trig; u8 ntrig;
    const Warp *xwarps; u8 nxwarps;
} MapDef;

#define N(a) (sizeof(a) / sizeof((a)[0]))
#define RET 255
#define KEEP 255
extern const int num_trainers;

// scripts
enum { S_NONE, S_HOME_PC, S_LAB_BRANDT, S_ROST_BLOCK, S_HOME_BLOCK, S_NEON_GUARD, S_CANAL_GUARD, S_AQ_GUARD, S_KH_GUARD, S_S0_GUARD,
       S_LOBBY_BOT, S_KAI_HELP, S_CORE, S_ABYSS_SIGN, S_HOUSE, S_ARENA_LOCK, S_FIRST_HOME,
       S_THRONE, S_RANKING, S_HALL_NURSE, S_AETHER_GUARD, S_HERMIT, S_PENSION, S_GIFT_BIKE, S_GIFT_ROD, S_RELEARN, S_HELDSHOP };

typedef struct { const char *name; u8 chr, t1, t2; u16 lines[6]; const char *intro, *lose; } ChallengerData;   // line 0xFFFF = rival starter
#include "gen_world.inc"
#include "gen_champ.inc"

// ------------------------------------------------------------------ map data
static const Warp w_home[] = { { P_HOME_EXIT_X, P_HOME_EXIT_Y, 2, 1, M_ROST, P_ROST_HOME_X, P_ROST_HOME_Y + 1, D_DOWN, F_INTRO_DONE } };
static const NpcDef n_home[] = {
    { P_HOME_PC_X, P_HOME_PC_Y, 0, 0, NK_SCRIPT, S_HOME_PC, NOF, NOF, NULL, NULL },
};
static const Trigger t_home[] = { { P_HOME_EXIT_X, P_HOME_EXIT_Y - 1, 2, 1, NOF, F_INTRO_DONE, S_HOME_BLOCK } };

static const Warp w_rost[] = {
    { P_ROST_LAB_X, P_ROST_LAB_Y, 1, 1, M_LAB, P_LAB_IN_X, P_LAB_IN_Y, D_UP, NOF },
    { P_ROST_HOME_X, P_ROST_HOME_Y, 1, 1, M_HOME, P_HOME_IN_X, P_HOME_IN_Y, D_UP, NOF },
    { P_ROST_STATION_X, P_ROST_STATION_Y, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF },
    { P_ROST_N_X, P_ROST_N_Y, 4, 1, M_KABEL, P_KA_S_X, P_KA_S_Y - 1, D_UP, NOF },
};
static const NpcDef n_rost[] = {
    { P_ROST_SIGN_X, P_ROST_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "ROST-SEKTOR|\"Wo Neo-Kyra rostet, aber niemals schläft.\"" },
    { 8, 9, CH_KID, D_DOWN, NK_TALK, 0, NOF, NOF, "Junge", "Hast du gehört? Letzte Nacht sind schwarze Helix-Gleiter über dem Labor gekreist!" },
    { 15, 14, CH_ELDER, D_LEFT, NK_TALK, 0, NOF, NOF, "Alter Fischer", "Früher war hier unten Land. Jetzt fischen wir Datenkabel aus dem Meer." },
    { 20, 7, CH_WOMAN, D_LEFT, NK_TALK, 0, F_GOT_STARTER, NOF, "Nachbarin", "Du hast jetzt einen Vexel? Pass gut auf ihn auf. Helix nimmt sie einfach mit." },
};
static const Trigger t_rost[] = { { 10, 1, 4, 1, NOF, F_GOT_STARTER, S_ROST_BLOCK } };

static const Warp w_lab[] = { { P_LAB_EXIT_X, P_LAB_EXIT_Y, 2, 1, M_ROST, P_ROST_LAB_X, P_ROST_LAB_Y + 1, D_DOWN, NOF } };
static const NpcDef n_lab[] = {
    { 6, 1, CH_SCIENTIST, D_DOWN, NK_SCRIPT, S_LAB_BRANDT, NOF, NOF, "Dr. Brandt", NULL },
    { 11, 4, CH_SCIENTIST, D_LEFT, NK_TALK, 0, NOF, NOF, "Assistent", "Die Helix-Agenten haben alle Festplatten mitgenommen. Nur die drei Vexel-Kerne haben sie nicht gefunden." },
    { 0, 6, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "Das Terminal zeigt: \"PROTOKOLL NULL – Zugriff verweigert.\"" },
    { 13, 6, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "Eine Simulation von Vexel-Evolutionen: Bit, Byte, Kern, Champion, Ultra, Mega. Ab Ultra tragen viele echte Waffen!" },
};

static const Warp w_kabel[] = {
    { P_KA_S_X, P_KA_S_Y, 4, 1, M_ROST, P_ROST_N_X, P_ROST_N_Y + 1, D_DOWN, NOF },
    { P_KA_N_X, P_KA_N_Y, 4, 1, M_NEON, P_NE_S_X, P_NE_S_Y - 1, D_UP, NOF },
};
static const NpcDef n_kabel[] = {
    { P_KA_SIGN_X, P_KA_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "KABELPFAD|Norden: Neon-Distrikt. Achtung: wilde Vexel im leuchtenden Datengras!" },
    { 12, 24, CH_RUNNER, D_LEFT, NK_TRAINER, 1, NOF, NOF, NULL, NULL },
    { 13, 12, CH_PUNK, D_DOWN, NK_TRAINER, 2, NOF, NOF, NULL, NULL },
    { 6, 6, CH_KID, D_RIGHT, NK_TRAINER, 3, NOF, NOF, NULL, NULL },
    { 17, 22, 0, 0, NK_ITEM, IT_KIT, NOF, F_ITEM_BASE + 0, NULL, NULL },
    { 2, 3, 0, 0, NK_ITEM, IT_KAPSEL, NOF, F_ITEM_BASE + 1, NULL, NULL },
    { 8, 17, CH_WOMAN, D_RIGHT, NK_TALK, 0, NOF, NOF, "Kurierin", "Tipp unter Kurieren: Mit B gedrückt rennst du. Und im Menü (X) siehst du deine Vexel." },
};

static const Warp w_neon[] = {
    { P_NE_S_X, P_NE_S_Y, 4, 1, M_KABEL, P_KA_N_X, P_KA_N_Y + 1, D_DOWN, NOF },
    { P_NE_ARENA_X, P_NE_ARENA_Y, 2, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 1 },
    { P_NE_STATION_X, P_NE_STATION_Y, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF },
    { P_NE_H1_X, P_NE_H1_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_NE_GYM2_X, P_NE_GYM2_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 2 },
    { P_NE_H3_X, P_NE_H3_Y, 1, 1, M_CHAMPHALL, P_CHH_IN_X, P_CHH_IN_Y, D_UP, NOF },
    { P_NE_H4_X, P_NE_H4_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_NE_H5_X, P_NE_H5_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_NE_E_X, P_NE_E_Y, 1, 2, M_HYDRO, P_HY_W_X + 1, P_HY_W_Y, D_RIGHT, NOF },
};
static const NpcDef n_neon[] = {
    { P_NE_SIGN_X, P_NE_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "NEON-DISTRIKT|Arena des 1. Netzknotens: Norden. Ladestation: Osten. Neonpark: Südwesten." },
    { P_NE_PARKSIGN_X, P_NE_PARKSIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "NEONPARK|Bitte keine Vexel füttern. Sie fressen sonst die Straßenlaternen." },
    { 17, 5, CH_SCIENTIST, D_DOWN, NK_TALK, 0, NOF, F_GRUNT_PARK, "Arena-Techniker", "Die Arena ist geschlossen! Wächterin Volta ist im Neonpark – Helix-Agenten machen dort Ärger!" },
    { 3, 23, CH_GRUNT, D_RIGHT, NK_TRAINER, 4, NOF, NOF, NULL, NULL },
    { 10, 22, CH_GRUNT, D_LEFT, NK_TRAINER, 5, NOF, NOF, NULL, NULL },
    { 7, 22, CH_WARDEN_VOLT, D_DOWN, NK_TALK, 0, NOF, F_GRUNT_PARK, "Volta", "Diese Helix-Typen! Hilf mir, sie zu vertreiben – dann reden wir in der Arena." },
    { 12, 25, CH_KID, D_UP, NK_TALK, 0, NOF, NOF, "Mädchen", "Die Agenten wollten mein Pixiko mitnehmen! Danke, dass du hier bist..." },
    { 28, 15, CH_PUNK, D_LEFT, NK_TRAINER, 6, NOF, NOF, NULL, NULL },
    { 22, 6, CH_ELDER, D_DOWN, NK_TALK, 0, NOF, NOF, "Straßenhändler", "Die Billboards zeigen nur noch Helix-Werbung. \"OMEGA – Die Zukunft ist vernetzt.\" Gruselig." },
    { 9, 14, CH_RUNNER, D_UP, NK_TALK, 0, NOF, NOF, "Läuferin", "Vexel entwickeln sich, wenn sie stark genug werden. Manche verändern dabei sogar ihren Typ!" },
    { 31, 22, CH_WOMAN, D_UP, NK_TALK, 0, NOF, NOF, "Frau", "In der Ladestation heilt NURSE-7 deine Vexel. Und der Händler-Bot verkauft Kapseln." },
    { 29, 11, CH_ELDER, D_DOWN, NK_SCRIPT, S_PENSION, NOF, NOF, "Pensionsleiterin Mo", NULL },
    { 22, 21, CH_PUNK, D_DOWN, NK_SCRIPT, S_GIFT_BIKE, NOF, NOF, "Mechanikerin Zip", NULL },
};

static const Warp w_hydro[] = {
    { 0, P_HY_W_Y, 1, 2, M_NEON, P_NE_E_X - 1, P_NE_E_Y, D_LEFT, NOF },
    { 39, P_HY_E_Y, 1, 2, M_AQUARIS, P_AQ_W_X + 1, P_AQ_W_Y, D_RIGHT, NOF },
};
static const NpcDef n_hydro[] = {
    { P_HY_SIGN_X, P_HY_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "HYDROKANAL|Osten: Hafenstadt Aquaris. Brücken zu den Inseln aus Datengras." },
    { 9, 4, CH_RUNNER, D_DOWN, NK_TRAINER, 10, NOF, NOF, NULL, NULL },
    { 22, 13, CH_WOMAN, D_UP, NK_TRAINER, 11, NOF, NOF, NULL, NULL },
    { 20, 9, CH_KAI, D_LEFT, NK_TRAINER, 12, NOF, NOF, NULL, NULL },
    { 30, 4, CH_GRUNT, D_DOWN, NK_TRAINER, 13, NOF, NOF, NULL, NULL },
    { 12, 13, 0, 0, NK_ITEM, IT_SUPERKAPSEL, NOF, F_ITEM_BASE + 2, NULL, NULL },
    { 32, 2, 0, 0, NK_ITEM, IT_ANTIVIRUS, NOF, F_ITEM_BASE + 3, NULL, NULL },
};

static const Warp w_aq[] = {
    { 0, P_AQ_W_Y, 1, 2, M_HYDRO, 38, P_HY_E_Y, D_LEFT, NOF },
    { P_AQ_ARENA_X, P_AQ_ARENA_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 3 },
    { P_AQ_STATION_X, P_AQ_STATION_Y, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF },
    { P_AQ_H1_X, P_AQ_H1_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_AQ_GYM2_X, P_AQ_GYM2_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 4 },
    { P_AQ_N_X, P_AQ_N_Y, 2, 1, M_CAVE, P_CA_S_X, P_CA_S_Y - 1, D_UP, NOF },
};
static const NpcDef n_aq[] = {
    { P_AQ_SIGN_X, P_AQ_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "AQUARIS|Hafenstadt am Rand von Neo-Kyra. Norden: Kristallhöhle." },
    { 9, 10, CH_ELDER, D_RIGHT, NK_SCRIPT, S_GIFT_ROD, NOF, NOF, "Hafenmeister", NULL },
    { 16, 15, CH_KID, D_DOWN, NK_TALK, 0, NOF, NOF, "Junge", "Vom Pier aus sieht man die alten Städte unter Wasser. Da leuchtet es manchmal..." },
    { 25, 9, CH_WOMAN, D_LEFT, NK_TALK, 0, NOF, NOF, "Matrosin", "Helix-Frachter laden jede Nacht Kisten voller Vexel-Kapseln. Wohin bringen die die nur?" },
    { 27, 14, 0, 0, NK_ITEM, IT_SUPERKIT, NOF, F_ITEM_BASE + 4, NULL, NULL },
};

static const Warp w_cave[] = {
    { P_CA_S_X, P_CA_S_Y, 2, 1, M_AQUARIS, P_AQ_N_X, P_AQ_N_Y + 1, D_DOWN, NOF },
    { P_CA_N_X, P_CA_N_Y, 2, 1, M_KRISHAIN, P_KH_S_X, P_KH_S_Y - 1, D_UP, NOF },
};
static const NpcDef n_cave[] = {
    { 10, 13, CH_SCIENTIST, D_RIGHT, NK_TRAINER, 17, NOF, NOF, NULL, NULL },
    { 23, 10, CH_GRUNT, D_DOWN, NK_TRAINER, 18, NOF, NOF, NULL, NULL },
    { 7, 18, CH_WOMAN, D_RIGHT, NK_TRAINER, 19, NOF, NOF, NULL, NULL },
    { 19, 20, 0, 0, NK_ITEM, IT_HYPERKAPSEL, NOF, F_ITEM_BASE + 5, NULL, NULL },
    { 12, 14, 0, 0, NK_ITEM, IT_REBOOT, NOF, F_ITEM_BASE + 6, NULL, NULL },
    { 16, 5, CH_ELDER, D_DOWN, NK_TALK, 0, NOF, NOF, "Höhlenforscher", "Die Kristalle hier summen. Sie speichern Erinnerungen an das alte Netz." },
};

static const Warp w_kh[] = {
    { P_KH_S_X, P_KH_S_Y, 2, 1, M_CAVE, P_CA_N_X, P_CA_N_Y + 1, D_DOWN, NOF },
    { P_KH_ARENA_X, P_KH_ARENA_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 5 },
    { P_KH_STATION_X, P_KH_STATION_Y, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF },
    { P_KH_GYM2_X, P_KH_GYM2_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 6 },
    { P_KH_H2_X, P_KH_H2_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_KH_E_X, P_KH_E_Y, 1, 2, M_GLITCH, P_GL_W_X + 1, P_GL_W_Y, D_RIGHT, NOF },
};
static const NpcDef n_kh[] = {
    { P_KH_SIGN_X, P_KH_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "KRISTALLHAIN|Ein Dorf aus gefrorenem Licht. Osten: Glitchwüste." },
    { 8, 11, CH_KID, D_DOWN, NK_TALK, 0, NOF, NOF, "Mädchen", "Im Eisfeld leben Kristall-Vexel. Manche sind so selten, dass niemand sie je gefangen hat!" },
    { 20, 8, CH_ELDER, D_DOWN, NK_TALK, 0, NOF, NOF, "Älteste", "Wächterin Silja war Lyras Schülerin. Sie hat seit ihrem Verschwinden kaum geschlafen." },
};

static const Warp w_glitch[] = {
    { 0, P_GL_W_Y, 1, 2, M_KRISHAIN, P_KH_E_X - 1, P_KH_E_Y, D_LEFT, NOF },
    { 39, P_GL_E_Y, 1, 2, M_SEKTOR0, P_S0_W_X + 1, P_S0_W_Y, D_RIGHT, NOF },
};
static const NpcDef n_glitch[] = {
    { P_GL_SIGN_X, P_GL_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "GLITCHWÜSTE|Warnung: Realitätsfehler. Plasmafelder nicht betreten!" },
    { 20, 11, CH_KAI, D_LEFT, NK_TRAINER, 23, NOF, NOF, NULL, NULL },
    { 8, 6, CH_WOMAN, D_RIGHT, NK_TRAINER, 24, NOF, NOF, NULL, NULL },
    { 28, 13, CH_GRUNT, D_LEFT, NK_TRAINER, 25, NOF, NOF, NULL, NULL },
    { 17, 18, CH_PUNK, D_UP, NK_TRAINER, 26, NOF, NOF, NULL, NULL },
    { 35, 20, 0, 0, NK_ITEM, IT_MAXKIT, NOF, F_ITEM_BASE + 7, NULL, NULL },
    { 6, 19, 0, 0, NK_ITEM, IT_ZELLE, NOF, F_ITEM_BASE + 8, NULL, NULL },
};

static const Warp w_s0[] = {
    { 0, P_S0_W_Y, 1, 2, M_GLITCH, 38, P_GL_E_Y, D_LEFT, NOF },
    { P_S0_ARENA_X, P_S0_ARENA_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 7 },
    { P_S0_STATION_X, P_S0_STATION_Y, 1, 1, M_STATION, P_ST_IN_X, P_ST_IN_Y, D_UP, NOF },
    { P_S0_H1_X, P_S0_H1_Y, 1, 1, M_HOUSE, P_HO_IN_X, P_HO_IN_Y, D_UP, NOF },
    { P_S0_GYM2_X, P_S0_GYM2_Y, 1, 1, M_ARENA, P_AR_IN_X, P_AR_IN_Y, D_UP, NOF, 8 },
    { P_S0_N_X, P_S0_N_Y, 2, 1, M_LOBBY, P_LO_S_X, P_LO_S_Y - 1, D_UP, F_TOWER_OPEN },
};
static const NpcDef n_s0[] = {
    { P_S0_SIGN_X, P_S0_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "SEKTOR NULL|Hier begann der Sonnensturm. Norden: Helix-Turm (Zutritt verboten)." },
    { 12, 1, CH_GRUNT, D_DOWN, NK_SCRIPT, S_S0_GUARD, NOF, F_TOWER_OPEN, "Helix-Wache", NULL },
    { 13, 1, CH_GRUNT, D_DOWN, NK_SCRIPT, S_S0_GUARD, NOF, F_TOWER_OPEN, "Helix-Wache", NULL },
    { 6, 11, CH_ELDER, D_RIGHT, NK_TALK, 0, NOF, NOF, "Überlebender", "Ich war hier, als der Sturm kam. Die Bildschirme schrien. Und dann... krochen die ersten Vexel heraus." },
    { 20, 11, CH_RUNNER, D_LEFT, NK_TALK, 0, NOF, NOF, "Rebellin", "Wir nennen uns Freie Kuriere. Wenn du gegen Helix kämpfst, bist du eine von uns." },
};

static const Warp w_lobby[] = {
    { P_LO_S_X, P_LO_S_Y, 2, 1, M_SEKTOR0, P_S0_N_X, P_S0_N_Y + 1, D_DOWN, NOF },
    { P_LO_WARP_X, P_LO_WARP_Y, 2, 1, M_LIGA, P_LI_S_X, P_LI_S_Y - 1, D_UP, NOF },
};
static const NpcDef n_lobby[] = {
    { 9, 3, CH_SHOPBOT, D_DOWN, NK_SCRIPT, S_LOBBY_BOT, NOF, NOF, "EMPFANG-3", NULL },
    { 6, 9, CH_GRUNT, D_RIGHT, NK_TRAINER, 30, NOF, NOF, NULL, NULL },
    { 13, 6, CH_GRUNT, D_LEFT, NK_TRAINER, 31, NOF, NOF, NULL, NULL },
    { 16, 14, 0, 0, NK_ITEM, IT_MAXKIT, NOF, F_ITEM_BASE + 9, NULL, NULL },
};

static const Warp w_orbit[] = {
    { P_OR_S_X, P_OR_S_Y, 2, 1, M_LIGA, P_LI_N_X, P_LI_N_Y + 1, D_DOWN, NOF },
    { P_OR_N_X, P_OR_N_Y, 2, 1, M_CORE, P_CO_S_X, P_CO_S_Y - 1, D_UP, F_RIVAL4 },
};
static const NpcDef n_orbit[] = {
    { 4, 11, CH_GRUNT, D_RIGHT, NK_TRAINER, 32, NOF, NOF, NULL, NULL },
    { 15, 3, CH_KAI, D_DOWN, NK_TRAINER, 34, NOF, NOF, NULL, NULL },
    { 14, 3, CH_KAI, D_DOWN, NK_SCRIPT, S_KAI_HELP, F_RIVAL4, NOF, "Kai", NULL },
    { 25, 21, 0, 0, NK_ITEM, IT_HYPERKAPSEL, NOF, F_ITEM_BASE + 10, NULL, NULL },
    { 3, 21, 0, 0, NK_ITEM, IT_REBOOT, NOF, F_ITEM_BASE + 11, NULL, NULL },
};

static const Warp w_core[] = {
    { P_CO_S_X, P_CO_S_Y, 2, 1, M_ORBIT, P_OR_N_X, P_OR_N_Y + 1, D_DOWN, NOF },
    { P_CO_ABYSS_X, P_CO_ABYSS_Y, 1, 1, M_ABYSS, P_AB_S_X, P_AB_S_Y - 1, D_UP, F_ENDING },
};
static const NpcDef n_core[] = {
    { P_CO_CEO_X, P_CO_CEO_Y, CH_CEO, D_DOWN, NK_SCRIPT, S_CORE, NOF, F_CEO, "Viktor Kade", NULL },
    { P_CO_BOSS_X, P_CO_BOSS_Y, CH_OMEGA, D_DOWN, NK_SCRIPT, S_CORE, NOF, F_OMEGA, "OMEGA", NULL },
    { P_CO_BOSS_X, P_CO_BOSS_Y, CH_MOM, D_DOWN, NK_TALK, 0, F_ENDING, NOF, "Lyra Voss",
      "Ich bin so stolz auf dich, Nova. Die Vexel sind frei. Im Westen hat sich ein Riss im Code geöffnet – der Datenabgrund. Dort leben die seltensten Vexel." },
    { P_CO_BOSS_X + 2, P_CO_BOSS_Y + 1, CH_KAI, D_LEFT, NK_TALK, 0, F_ENDING, NOF, "Kai",
      "Ich baue Helix neu auf. Ohne OMEGA. Ohne Käfige. Und irgendwann... fordere ich dich wieder heraus." },
};

static const Warp w_abyss[] = { { P_AB_S_X, P_AB_S_Y, 2, 1, M_CORE, P_CO_ABYSS_X + 1, P_CO_ABYSS_Y, D_RIGHT, NOF } };
static const NpcDef n_abyss[] = {
    { P_AB_SIGN_X, P_AB_SIGN_Y, 0, 0, NK_SIGN, 0, NOF, NOF, NULL, "DATENABGRUND|Hier ruht der Code, aus dem alle Vexel entstanden." },
    { 14, 12, CH_OMEGA, D_DOWN, NK_TRAINER, 37, NOF, NOF, NULL, NULL },
    { 5, 15, CH_SCIENTIST, D_RIGHT, NK_TRAINER, 38, NOF, NOF, NULL, NULL },
    { 24, 15, CH_GRUNT, D_LEFT, NK_TRAINER, 39, NOF, NOF, NULL, NULL },
    { 3, 3, 0, 0, NK_ITEM, IT_OMEGAKAPSEL, NOF, F_ITEM_BASE + 12, NULL, NULL },
};

static const Warp w_station[] = { { P_ST_EXIT_X, P_ST_EXIT_Y, 2, 1, RET, 0, 0, D_DOWN, NOF } };
static const NpcDef n_station[] = {
    { P_ST_NURSE_X, P_ST_NURSE_Y, CH_NURSEBOT, D_DOWN, NK_NURSE, 0, NOF, NOF, "NURSE-7", NULL },
    { P_ST_SHOP_X, P_ST_SHOP_Y, CH_SHOPBOT, D_DOWN, NK_SHOP, 0, NOF, NOF, "HÄNDLER-BOT", NULL },
    { 9, 1, 0, 0, NK_BOX, 0, NOF, NOF, NULL, NULL },
    { 10, 1, 0, 0, NK_BOX, 0, NOF, NOF, NULL, NULL },
    { 2, 4, CH_RUNNER, D_RIGHT, NK_SCRIPT, S_RELEARN, NOF, NOF, "Attacken-Coach", NULL },
    { 10, 4, CH_SCIENTIST, D_LEFT, NK_SCRIPT, S_HELDSHOP, NOF, NOF, "Ausrüster", NULL },
};
static const Warp w_arena[] = { { P_AR_EXIT_X, P_AR_EXIT_Y, 2, 1, RET, 0, 0, D_DOWN, NOF } };
static const Warp w_house[] = { { P_HO_EXIT_X, P_HO_EXIT_Y, 2, 1, RET, 0, 0, D_DOWN, NOF } };

static const Warp w_liga[] = {
    { P_LI_S_X, P_LI_S_Y, 2, 1, M_LOBBY, P_LO_WARP_X, P_LO_WARP_Y + 1, D_DOWN, NOF, 0 },
    { P_LI_N_X, P_LI_N_Y, 2, 1, M_ORBIT, P_OR_S_X, P_OR_S_Y - 1, D_UP, NOF, 0 },
};
static const NpcDef n_liga[] = {
    { P_LI_E0_X, P_LI_E0_Y, CH_ELITE0, D_DOWN, NK_ELITE, 0, NOF, F_ELITE_BASE + 0, NULL, NULL },
    { P_LI_E1_X, P_LI_E1_Y, CH_ELITE1, D_DOWN, NK_ELITE, 1, NOF, F_ELITE_BASE + 1, NULL, NULL },
    { P_LI_E2_X, P_LI_E2_Y, CH_ELITE2, D_DOWN, NK_ELITE, 2, NOF, F_ELITE_BASE + 2, NULL, NULL },
    { P_LI_E3_X, P_LI_E3_Y, CH_ELITE3, D_DOWN, NK_ELITE, 3, NOF, F_ELITE_BASE + 3, NULL, NULL },
    { 3, 41, CH_NURSEBOT, D_DOWN, NK_NURSE, 0, NOF, NOF, "NURSE-9", NULL },
    { 10, 41, CH_SHOPBOT, D_DOWN, NK_SHOP, 0, NOF, NOF, "HÄNDLER-BOT", NULL },
};

static const MapDef maps[NUM_MAPS] = {
    [M_HOME] = { "Zuhause", ' ', -1, MUS_TOWN, 0, 0, w_home, N(w_home), n_home, N(n_home), t_home, N(t_home) },
    [M_ROST] = { "Rost-Sektor", '~', -1, MUS_TOWN, 0, 0, w_rost, N(w_rost), n_rost, N(n_rost), t_rost, N(t_rost) },
    [M_LAB] = { "Voss-Labor", ' ', -1, MUS_TOWN, 0, 0, w_lab, N(w_lab), n_lab, N(n_lab), NULL, 0 },
    [M_STATION] = { "Ladestation", ' ', -1, MUS_HEAL, 0, 0, w_station, N(w_station), n_station, N(n_station), NULL, 0 },
    [M_ARENA] = { "Knoten-Arena", ' ', -1, MUS_TOWER, 0, 0, w_arena, N(w_arena), NULL, 0, NULL, 0 },
    [M_HOUSE] = { "Wohnung", ' ', -1, MUS_TOWN, 0, 0, w_house, N(w_house), NULL, 0, NULL, 0 },
    [M_KABEL] = { "Kabelpfad", 'T', 0, MUS_ROUTE, 1, 0, w_kabel, N(w_kabel), n_kabel, N(n_kabel), NULL, 0 },
    [M_NEON] = { "Neon-Distrikt", 'w', 1, MUS_TOWN, 0, 0, w_neon, N(w_neon), n_neon, N(n_neon), NULL, 0 , xw_NEON, N(xw_NEON) },
    [M_HYDRO] = { "Hydrokanal", '~', 2, MUS_ROUTE, 2, 0, w_hydro, N(w_hydro), n_hydro, N(n_hydro), NULL, 0 },
    [M_AQUARIS] = { "Aquaris", '~', -1, MUS_TOWN, 2, 0, w_aq, N(w_aq), n_aq, N(n_aq), NULL, 0 , xw_AQUARIS, N(xw_AQUARIS) },
    [M_CAVE] = { "Kristallhöhle", 'V', 3, MUS_CAVE, 3, 1, w_cave, N(w_cave), n_cave, N(n_cave), NULL, 0 },
    [M_KRISHAIN] = { "Kristallhain", 'c', 3, MUS_TOWN, 3, 0, w_kh, N(w_kh), n_kh, N(n_kh), NULL, 0 , xw_KRISHAIN, N(xw_KRISHAIN) },
    [M_GLITCH] = { "Glitchwüste", 'V', 4, MUS_ROUTE, 4, 0, w_glitch, N(w_glitch), n_glitch, N(n_glitch), NULL, 0 , xw_GLITCH, N(xw_GLITCH) },
    [M_SEKTOR0] = { "Sektor Null", 'U', -1, MUS_TOWN, 4, 0, w_s0, N(w_s0), n_s0, N(n_s0), NULL, 0 , xw_SEKTOR0, N(xw_SEKTOR0) },
    [M_LOBBY] = { "Helix-Turm", 'U', -1, MUS_TOWER, 5, 0, w_lobby, N(w_lobby), n_lobby, N(n_lobby), NULL, 0 },
    [M_ORBIT] = { "Orbitalgarten", 'U', 5, MUS_TOWER, 5, 0, w_orbit, N(w_orbit), n_orbit, N(n_orbit), NULL, 0 },
    [M_CORE] = { "OMEGA-Kern", 'G', -1, MUS_TOWER, 6, 0, w_core, N(w_core), n_core, N(n_core), NULL, 0 },
    [M_ABYSS] = { "Datenabgrund", 'g', 6, MUS_CAVE, 6, 1, w_abyss, N(w_abyss), n_abyss, N(n_abyss), NULL, 0 },
    [M_LIGA] = { "Halle des Hohen Rats", 'U', -1, MUS_TOWER, 5, 0, w_liga, N(w_liga), n_liga, N(n_liga), NULL, 0 },
    GEN_MAPDEFS
};

static const char *const house_texts[] = {
    "Mein Sohn arbeitet für Helix. Er kommt nie nach Hause. Er sagt, OMEGA braucht ihn rund um die Uhr.",
    "Wusstest du, dass Vexel träumen? Mein kleines Byte zuckt im Schlaf, als würde es Daten verarbeiten.",
    "Früher gab es Bäume aus Holz. Heute wachsen nur noch Bio-Vexel auf den Dächern. Auch schön, irgendwie.",
    "Hast du schon alle 1463 Vexel gesehen? Mein Opa sagt, die letzten leben im Datenabgrund.",
    "Ich repariere alte Kampfroboter. Die Vexel des Metall-Typs lieben es, in meiner Werkstatt zu schlafen.",
    "Lyra Voss? Die hat mir mal mein Haustier-Vexel gerettet. Eine gute Frau.",
    "Schhh! Mein Psi-Vexel meditiert. Wenn es aufwacht, schweben hier wieder alle Möbel.",
    "Die Netzknoten-Wächter sind die einzigen, die sich gegen Helix stellen. Mögen ihre Server ewig laufen.",
    "Die Typen sind wichtig! Plasma schlägt Bio, Bio schlägt Hydro, Hydro schlägt Plasma. Und dann gibt es noch neun weitere...",
    "Ein Tipp: Status-Attacken wie Overclock oder Firewall können Kämpfe entscheiden!",
};
static const u8 house_chr[] = { CH_WOMAN, CH_ELDER, CH_KID, CH_RUNNER, CH_SCIENTIST, CH_PUNK };

// ------------------------------------------------------------------ runtime state
typedef struct { int x, y, dir, visible, chr, ox, oy, moving; const NpcDef *def; } Npc;
#define MAX_NPC 16
static Npc npcs[MAX_NPC];
static int nnpc;
static NpcDef dyn_npc[4];
static NpcDef leg_npc, chal_npc;
static int fol_x, fol_y, fol_vis, fol_ox, fol_oy;   // partner Vexel walking behind the player
static int is_outdoor(int m);
static void follower_reset(void);
static int follower_species(void);
static int light_phase = -1;
static int face_toward(int fx, int fy, int tx_, int ty_);
static void give_key(int f, const char *who, const char *text);
static u8 tiles[64 * 64];
static int mw, mh;
int cur_map = 0;
static int px, py, pdir, pmoving, poff, pframe, prun;
static int cam_x, cam_y;
static int banner_t;
static int bottom_need;
static u16 tile_avg[80];
static int steps;

static const char legend_chars[] = " .,_-|;*TY~=HWwNnRrDEFkCcsxOSLPfiIJKQBtpMXmaAZvUuGVezbhqg";
static const u8 legend_tiles[] = {
    TL_VOID, TL_PLATE, TL_PLATE2, TL_ROAD, TL_ROAD_H, TL_ROAD_V, TL_GRASS, TL_DATAGRASS, TL_TREE, TL_TREE2, TL_WATER, TL_BRIDGE, TL_BRIDGE_V,
    TL_WALL, TL_WALL2, TL_WALL_NEON, TL_WALL_CYAN, TL_ROOF, TL_ROOF2, TL_DOOR, TL_DOOR_RED, TL_FENCE, TL_ROCKFLOOR, TL_CRYSTAL, TL_CRYSTAL2,
    TL_SAND, TL_SAND_ENC, TL_ROCK, TL_SIGN, TL_LAMP, TL_PIPE, TL_FLOWER, TL_IN_FLOOR, TL_IN_WALL, TL_IN_WINDOW, TL_COUNTER, TL_PC, TL_BED,
    TL_TABLE, TL_PLANT, TL_MACHINE, TL_WARP, TL_CARPET, TL_ARENA, TL_ARENA_LINE, TL_PLASMA, TL_VOIDFLOOR, TL_TOWERWALL, TL_TOWERFLOOR,
    TL_RACK, TL_CLIFF, TL_ICE, TL_ICE_ENC, TL_BILLBOARD, TL_HEAL, TL_MAT, TL_GLITCH,
};

static int char_tile(char c)
{
    const char *p = strchr(legend_chars, c);
    if (!p || !c) return TL_VOID;
    return legend_tiles[p - legend_chars];
}

static int tile_at(int x, int y)
{
    if (x < 0 || y < 0 || x >= mw || y >= mh) return char_tile(maps[cur_map].outside);
    return tiles[y * mw + x];
}

static int npc_visible(const NpcDef *d)
{
    if (d->req >= 0 && !flag(d->req)) return 0;
    if (d->hide >= 0 && flag(d->hide)) return 0;
    if (d->kind == NK_TRAINER && d->arg != 0 && flag(F_TRAINER_BASE + d->arg) && trainers[d->arg].boss == 0 && 0) return 0;
    return 1;
}

static void load_npcs(void)
{
    nnpc = 0;
    const MapDef *m = &maps[cur_map];
    const NpcDef *list = m->npcs;
    int n = m->nnpcs;
    if (cur_map == M_ARENA) {
        int g = G.cur_gym % NUM_GYMS;
        static const u8 trc[4] = { CH_RUNNER, CH_PUNK, CH_SCIENTIST, CH_WOMAN };
        dyn_npc[0] = (NpcDef){ P_AR_BOSS_X, P_AR_BOSS_Y, gyms[g].chr, D_DOWN, NK_GYM, g, NOF, NOF, NULL, NULL };
        dyn_npc[1] = (NpcDef){ P_AR_T1_X, P_AR_T1_Y, trc[g & 3], D_RIGHT, NK_GYMTR, g * 2, NOF, NOF, NULL, NULL };
        dyn_npc[2] = (NpcDef){ P_AR_T2_X, P_AR_T2_Y, trc[(g + 1) & 3], D_LEFT, NK_GYMTR, g * 2 + 1, NOF, NOF, NULL, NULL };
        list = dyn_npc; n = 3;
    }
    if (cur_map == M_HOUSE) {
        int h = (G.ret_map * 7 + G.ret_x * 3 + G.ret_y) % 10;
        dyn_npc[0] = (NpcDef){ P_HO_NPC_X, P_HO_NPC_Y, house_chr[h % 6], D_DOWN, NK_TALK, 0, NOF, NOF, "Bewohner", house_texts[h] };
        list = dyn_npc; n = 1;
    }
    for (int i = 0; i < n + 1 && nnpc < MAX_NPC; i++) {
        const NpcDef *d = &list[i];
        if (i == n) {
            // legendary Urcode living on this map (post-champion story)
            int k;
            for (k = 0; k < NUM_LEGENDS; k++) if (legend_spot[k][0] == cur_map) break;
            if (k == NUM_LEGENDS) break;
            leg_npc = (NpcDef){ legend_spot[k][1], legend_spot[k][2], 0, D_DOWN, NK_LEGEND, k,
                                k == 0 ? F_LEGEND_START : F_LEG_MET_BASE + k - 1, F_LEG_CAUGHT_BASE + k, NULL, NULL };
            d = &leg_npc;
        }
        Npc *o = &npcs[nnpc++];
        o->def = d;
        o->x = d->x; o->y = d->y; o->dir = d->dir; o->chr = d->chr;
        o->ox = o->oy = 0; o->moving = 0;
        o->visible = npc_visible(d);
    }
}

static void refresh_npcs(void)
{
    for (int i = 0; i < nnpc; i++) npcs[i].visible = npc_visible(npcs[i].def);
}

void world_warp(int map, int x, int y, int dir)
{
    if (map == RET) { map = G.ret_map; x = G.ret_x; y = G.ret_y; }
    cur_map = map;
    LOG("[warp] map %d (%d,%d)\n", map, x, y);
    const MapGeom *g = &map_geom[map];
    mw = g->w; mh = g->h;
    for (int j = 0; j < mh; j++)
        for (int i = 0; i < mw; i++) tiles[j * mw + i] = char_tile(g->rows[j][i]);
    px = x; py = y;
    if (dir != KEEP) pdir = dir;
    pmoving = 0; poff = 0;
    G.map = map; G.x = x; G.y = y; G.dir = pdir;
    load_npcs();
    follower_reset();
    if (!is_outdoor(map)) G.bike = 0;
    if (city_index(map) >= 0) set_flag(F_VISIT_BASE + city_index(map));
    music_play(maps[map].music);
    banner_t = 100;
    bottom_need = 1;
}

int world_style_bg(void) { return maps[cur_map].bg; }

// ------------------------------------------------------------------ drawing
static void compute_camera(void)
{
    int ox = 0, oy = 0;
    if (pmoving) {
        if (pdir == D_UP) oy = -poff;
        if (pdir == D_DOWN) oy = poff;
        if (pdir == D_LEFT) ox = -poff;
        if (pdir == D_RIGHT) ox = poff;
    }
    int wx = px * 16 + ox, wy = py * 16 + oy;
    cam_x = wx - 120;
    cam_y = wy - 88;
    if (mw * 16 <= SCR_W) cam_x = (mw * 16 - SCR_W) / 2;
    else { if (cam_x < 0) cam_x = 0; if (cam_x > mw * 16 - SCR_W) cam_x = mw * 16 - SCR_W; }
    if (mh * 16 <= SCR_H) cam_y = (mh * 16 - SCR_H) / 2;
    else { if (cam_y < 0) cam_y = 0; if (cam_y > mh * 16 - SCR_H) cam_y = mh * 16 - SCR_H; }
}

typedef struct { int y, kind, idx; } DrawItem;

void world_draw(void)
{
    compute_camera();
    u16 *fb = fb_top;
    if (light_phase < 0 || (frame_count & 63) == 0) light_phase = day_phase();
    static const u8 phase_tint[4] = { 1, 0, 2, 3 };
    gfx_set_tint(is_outdoor(cur_map) ? phase_tint[light_phase] : 0);
    int tx0 = cam_x >> 4, ty0 = cam_y >> 4;
    if (cam_x < 0) tx0 = (cam_x - 15) / 16;
    if (cam_y < 0) ty0 = (cam_y - 15) / 16;
    for (int ty_ = ty0; ty_ * 16 - cam_y < SCR_H; ty_++)
        for (int tx_ = tx0; tx_ * 16 - cam_x < SCR_W; tx_++) {
            int t = tile_at(tx_, ty_);
            draw_tile(fb, t, frame_count, tx_ * 16 - cam_x, ty_ * 16 - cam_y);
        }
    // sprites sorted by y
    DrawItem items_[MAX_NPC + 2];
    int n = 0;
    for (int i = 0; i < nnpc; i++) {
        if (!npcs[i].visible) continue;
        if (npcs[i].def->kind == NK_SIGN || npcs[i].def->kind == NK_BOX || (npcs[i].def->kind == NK_SCRIPT && npcs[i].chr == 0)) continue;
        items_[n++] = (DrawItem){ npcs[i].y * 16 + npcs[i].oy, 1, i };
    }
    int ox = 0, oy = 0;
    if (pmoving) {
        if (pdir == D_UP) oy = -poff;
        if (pdir == D_DOWN) oy = poff;
        if (pdir == D_LEFT) ox = -poff;
        if (pdir == D_RIGHT) ox = poff;
    }
    items_[n++] = (DrawItem){ py * 16 + oy, 0, 0 };
    int fsp = follower_species();
    if (fsp >= 0 && fol_vis && !(fol_x == px && fol_y == py)) items_[n++] = (DrawItem){ fol_y * 16 + fol_oy - 1, 2, fsp };
    for (int i = 1; i < n; i++) {
        DrawItem t = items_[i]; int j = i - 1;
        while (j >= 0 && items_[j].y > t.y) { items_[j + 1] = items_[j]; j--; }
        items_[j + 1] = t;
    }
    for (int k = 0; k < n; k++) {
        if (items_[k].kind == 2) {
            Mon *m = &G.party[items_[k].idx];
            int sx = fol_x * 16 + fol_ox - cam_x, sy = fol_y * 16 + fol_oy - cam_y;
            int b = ((frame_count >> 3) & 1) && !pmoving ? 1 : (pmoving && (poff & 8) ? 1 : 0);
            fill_blend(fb, sx + 1, sy + 12, 14, 4, C_BLACK, 7);
            int fdir = face_toward(fol_x, fol_y, px, py);
            draw_mon(fb, m->species, sx - 8, sy - 16 - b, 1, fdir == D_LEFT, SHINY(m));
        } else if (items_[k].kind == 0) {
            int sx = px * 16 + ox - cam_x, sy = py * 16 + oy - cam_y - 8;
            int fr = 0;
            if (pmoving) fr = 1 + ((poff / 8 + pframe) & 1);
            int base = pdir == D_DOWN ? 0 : pdir == D_UP ? 3 : 6;
            if (G.bike) {
                // hoverboard: glowing board, rider floats a little
                fr = 0;
                fill_blend(fb, sx + 2, sy + 23, 12, 3, C_BLACK, 8);
                fill(fb, sx + 1, sy + 19, 14, 3, RGB(4, 6, 12));
                fill(fb, sx + 2, sy + 20, 12, 1, (frame_count & 4) ? C_CYAN : C_MAGENTA);
                draw_chr(fb, CH_PLAYER, base, sx, sy - 3, pdir == D_RIGHT);
            } else {
                fill_blend(fb, sx + 3, sy + 21, 10, 3, C_BLACK, 7);
                draw_chr(fb, CH_PLAYER, base + fr, sx, sy, pdir == D_RIGHT);
            }
        } else {
            Npc *o = &npcs[items_[k].idx];
            int sx = o->x * 16 + o->ox - cam_x, sy = o->y * 16 + o->oy - cam_y - 8;
            if (o->def->kind == NK_LEGEND) {
                int b = ((frame_count >> 4) & 3) == 3 ? 1 : ((frame_count >> 4) & 1);
                fill_blend(fb, sx - 6, sy + 19, 28, 4, C_BLACK, 9);
                draw_mon(fb, FIRST_LEGEND_SPECIES + o->def->arg, sx - 24, sy - 42 - b, 2, o->x * 16 - cam_x > px * 16 - cam_x, 0);
                continue;
            }
            if (o->def->kind == NK_ITEM) {
                int b = ((frame_count >> 3) & 3) == 0 ? 1 : 0;
                fill_blend(fb, sx + 4, sy + 20, 8, 2, C_BLACK, 8);
                draw_capsule_icon(fb, sx + 4, sy + 11 - b, o->def->arg <= IT_OMEGAKAPSEL ? o->def->arg : 1);
                continue;
            }
            int base = o->dir == D_DOWN ? 0 : o->dir == D_UP ? 3 : 6;
            int fr = o->moving ? 1 + ((frame_count >> 3) & 1) : 0;
            fill_blend(fb, sx + 3, sy + 21, 10, 3, C_BLACK, 7);
            draw_chr(fb, o->chr, base + fr, sx, sy, o->dir == D_RIGHT);
        }
    }
    // atmosphere
    if (cur_map == M_CAVE || cur_map == M_ABYSS || cur_map == M_CORE) {
        int cx = px * 16 + ox - cam_x + 8, cy = py * 16 + oy - cam_y;
        light_cone(fb, cx, cy);
        if (banner_t > 0) plat_light_hole(4, 4, text_width(maps[cur_map].name) + 28, 21);
        else plat_light_hole(0, 0, 0, 0);
        plat_light(cx, cy, 64, 12);
    } else plat_light_off();
    // overworld weather (matches the weather in battles there)
    if (cur_map == M_R_TIEFSEE) {
        for (int k = 0; k < 40; k++) {
            int x = (k * 53 + frame_count * 3) % SCR_W, y = (k * 29 + frame_count * 7) % SCR_H;
            fill(fb, x, y, 1, 5, RGB(14, 20, 31));
        }
    } else if (cur_map == M_AETHERBERG || cur_map == M_R_HIMMEL) {
        for (int k = 0; k < 40; k++) {
            int x = (k * 53 + frame_count + ((frame_count / 8 + k) & 3)) % SCR_W, y = (k * 29 + frame_count * 2) % SCR_H;
            fill(fb, x, y, 2, 2, C_WHITE);
        }
    } else if (cur_map == M_R_LAVA || cur_map == M_GLUT) {
        for (int k = 0; k < 16; k++) {
            int x = (k * 67 + frame_count / 2) % SCR_W, y = SCR_H - 1 - (k * 37 + frame_count) % SCR_H;
            fill(fb, x, y, 1, 2, (k & 1) ? RGB(31, 20, 4) : RGB(31, 10, 2));
        }
    }
    if (cur_map == M_GLITCH && (frame_count % 97) < 3) {
        int y = (frame_count * 37) % 180;
        for (int j = 0; j < 6; j++)
            memmove(fb + (y + j) * SCR_W + 4, fb + (y + j) * SCR_W, (SCR_W - 4) * 2);
    }
    if (banner_t > 0) {
        banner_t--;
        int y = banner_t > 90 ? -(banner_t - 90) * 2 : (banner_t < 15 ? -(15 - banner_t) * 2 : 0);
        const char *nm = maps[cur_map].name;
        int w = text_width(nm) + 24;
        panel_solid(fb, 6, 6 + y, w, 17, RGB(3, 3, 9), C_MAGENTA);
        draw_text_sh(fb, 18, 10 + y, nm, C_WHITE, C_BLACK);
        fill(fb, 10, 11 + y, 4, 5, C_CYAN);
    }
    top_dirty = 1;
}

static void compute_tile_avg(void)
{
    for (int t = 0; t < tile_count && t < 80; t++) {
        const u16 *g = tile_gfx[tile_info[t][0]];
        int r = 0, gg = 0, b = 0;
        for (int i = 0; i < 256; i++) { r += g[i] & 31; gg += (g[i] >> 5) & 31; b += (g[i] >> 10) & 31; }
        tile_avg[t] = RGB(r / 256, gg / 256, b / 256);
    }
}

void world_draw_bottom(void)
{
    u16 *fb = fb_bot;
    draw_bottom_bg();
    // header
    panel_solid(fb, 4, 4, 248, 22, RGB(3, 4, 10), C_CYAN);
    draw_text_sh(fb, 12, 10, maps[cur_map].name, C_WHITE, C_BLACK);
    char buf[48];
    {
        int mins = clock_minutes();
        static const u16 pc[4] = { RGB(31, 24, 18), C_CYAN, RGB(31, 16, 6), RGB(14, 16, 31) };
        fmt(buf, "%s%d:%s%d", mins / 60 < 10 ? "0" : "", mins / 60, (mins % 60) < 10 ? "0" : "", mins % 60);
        draw_text_sh(fb, 214, 10, buf, pc[day_phase()], C_BLACK);
    }
    fmt(buf, "%d Cr", (int)G.money);
    draw_text_sh(fb, 150 - text_width(buf) / 2 + 20, 10, buf, C_YELLOW, C_BLACK);
    // fragments
    panel_solid(fb, 4, 30, 120, 20, RGB(3, 4, 10), C_MAGENTA);
    {
        int b = badge_count();
        fmt(buf, "Orden %d/24", b);
        draw_text_sh(fb, 10, 35, buf, C_TEXT2, C_BLACK);
        for (int i = 0; i < 24; i++) {
            int x = 66 + (i % 12) * 4 + (i >= 12 ? 0 : 0), y = 33 + (i / 12) * 7;
            u16 c = flag(F_BADGE_BASE + i) ? type_color(gyms[i].t1) : RGB(5, 5, 9);
            fill(fb, x, y, 3, 5, c);
        }
        if (b >= 8) draw_text(fb, 116, 35, "*", C_YELLOW);
    }
    // dex
    panel_solid(fb, 128, 30, 124, 20, RGB(3, 4, 10), C_MAGENTA);
    fmt(buf, "Vexikon %d/%d", count_caught(), NUM_SPECIES);
    draw_text_sh(fb, 136, 35, buf, C_TEXT2, C_BLACK);
    // party
    for (int i = 0; i < 6; i++) {
        int x = 4 + (i % 3) * 82, y = 54 + (i / 3) * 44;
        panel_solid(fb, x, y, 80, 42, RGB(3, 3, 8), i < G.party_n ? C_CYAN : RGB(6, 6, 10));
        if (i < G.party_n) {
            Mon *m = &G.party[i];
            draw_mon(fb, m->species, x + 1, y + 6, 1, 0, (m->hp ? 0 : 2) | SHINY(m));
            draw_text_sh(fb, x + 34, y + 5, species[m->species].name, C_WHITE, C_BLACK);
            fmt(buf, "Lv%d", m->level);
            draw_text_sh(fb, x + 34, y + 17, buf, C_YELLOW, C_BLACK);
            hpbar(fb, x + 34, y + 31, 40, m->hp, m->maxhp);
        }
    }
    // minimap
    int ms = (mw <= 30 && mh <= 26) ? 3 : 2;
    int mmw = mw * ms, mmh = mh * ms;
    if (mmh > 46) ms = 1, mmw = mw, mmh = mh;
    int mx0 = 4, my0 = 144;
    panel_solid(fb, mx0, my0, 160, 46, RGB(2, 2, 6), RGB(10, 14, 22));
    int ox = mx0 + 80 - mmw / 2, oy = my0 + 23 - mmh / 2;
    for (int y = 0; y < mh; y++)
        for (int x = 0; x < mw; x++) {
            u16 c = tile_avg[tiles[y * mw + x]];
            if (oy + y * ms > my0 + 1 && oy + y * ms < my0 + 44 && ox + x * ms > mx0 && ox + x * ms < mx0 + 158)
                fill(fb, ox + x * ms, oy + y * ms, ms, ms, c);
        }
    fill(fb, ox + px * ms - 1, oy + py * ms - 1, ms + 2, ms + 2, C_WHITE);
    fill(fb, ox + px * ms, oy + py * ms, ms, ms, C_MAGENTA);
    button(fb, 170, 144, 82, 46, "MENÜ (X)", C_MAGENTA, 0);
    bot_dirty = 1;
}

// ------------------------------------------------------------------ helpers
static Npc *npc_at(int x, int y)
{
    for (int i = 0; i < nnpc; i++)
        if (npcs[i].visible && npcs[i].x == x && npcs[i].y == y) return &npcs[i];
    return NULL;
}

static int solid_at(int x, int y)
{
    if (x < 0 || y < 0 || x >= mw || y >= mh) return 1;
    int t = tiles[y * mw + x];
    if (tile_info[t][2]) return 1;
    Npc *n = npc_at(x, y);
    if (n && n->def->kind != NK_SIGN) return 1;
    if (x == px && y == py) return 1;
    return 0;
}

static void dir_delta(int d, int *dx, int *dy)
{
    *dx = d == D_LEFT ? -1 : d == D_RIGHT ? 1 : 0;
    *dy = d == D_UP ? -1 : d == D_DOWN ? 1 : 0;
}

static void redraw(void)
{
    world_draw();
    frame();
}

static void npc_walk(Npc *o, int dir, int tiles_n)
{
    int dx, dy;
    dir_delta(dir, &dx, &dy);
    o->dir = dir;
    for (int t = 0; t < tiles_n; t++) {
        o->moving = 1;
        for (int k = 1; k <= 8; k++) {
            o->ox = dx * k * 2; o->oy = dy * k * 2;
            redraw();
        }
        o->x += dx; o->y += dy; o->ox = o->oy = 0;
    }
    o->moving = 0;
    redraw();
}

static void player_walk(int dir, int n)
{
    pdir = dir;
    int dx, dy;
    dir_delta(dir, &dx, &dy);
    for (int t = 0; t < n; t++) {
        pmoving = 1;
        for (poff = 2; poff <= 16; poff += 2) redraw();
        fol_x = px; fol_y = py;
        px += dx; py += dy; pmoving = 0; poff = 0; pframe ^= 1;
    }
    redraw();
}

static int face_toward(int fx, int fy, int tx_, int ty_)
{
    int dx = tx_ - fx, dy = ty_ - fy;
    if (dx * dx > dy * dy) return dx > 0 ? D_RIGHT : D_LEFT;
    return dy > 0 ? D_DOWN : D_UP;
}

void heal_party(void)
{
    for (int i = 0; i < G.party_n; i++) mon_heal(&G.party[i]);
}

static void exclaim(Npc *o)
{
    for (int i = 0; i < 30; i++) {
        world_draw();
        int sx = o->x * 16 - cam_x, sy = o->y * 16 - cam_y - 22;
        panel_solid(fb_top, sx + 3, sy - 2, 10, 12, C_WHITE, C_RED);
        draw_text(fb_top, sx + 7, sy, "!", C_RED);
        frame();
    }
}

static void blackout(void)
{
    say(NULL, "Du hast keine kampffähigen Vexel mehr...|Du eilst zur letzten Ladestation zurück.");
    G.money /= 2;
    heal_party();
    if (G.heal_map) { G.ret_map = G.heal_map; G.ret_x = G.heal_x; G.ret_y = G.heal_y; }
    else { G.ret_map = M_ROST; G.ret_x = P_ROST_STATION_X; G.ret_y = P_ROST_STATION_Y + 1; }
    world_warp(M_STATION, P_ST_NURSE_X, P_ST_NURSE_Y + 1, D_UP);
    redraw();
    say("NURSE-7", "Deine Vexel sind wieder vollständig geladen. Pass auf dich auf, Kurier.");
}

static int do_trainer(int tid, Npc *o);

// story after trainer battles
static void post_trainer(int tid)
{
    char buf[200];
    if (tid == 4 || tid == 5) {
        if (flag(F_TRAINER_BASE + 4) && flag(F_TRAINER_BASE + 5) && !flag(F_GRUNT_PARK)) {
            say("Volta", "Ha! Seht ihr sie rennen? Danke, Kurier. Du bist Lyras Kind, oder? Du hast ihre Augen.");
            say("Volta", "Komm in die Arena im Norden. Ich habe etwas, das deiner Mutter gehört – aber du musst es dir verdienen!");
            set_flag(F_GRUNT_PARK);
            refresh_npcs();
        }
    }
    if (tid == 12) {
        say("Kai", "Mein Vater sagt, Lyra hätte Helix bestohlen. Aber... warum versteckt er dann alles vor mir?");
    }
    if (tid == 23) {
        say("Kai", "Ich hab heimlich die Akten meines Vaters gelesen. OMEGA... da sind Gehirnscans drin. Von deiner Mutter.|Ich... ich wusste das nicht. Ich muss selbst herausfinden, was da oben passiert.");
        set_flag(F_RIVAL3);
    }
    if (tid == 34) {
        set_flag(F_RIVAL4);
        say("Kai", "Du hattest die ganze Zeit recht. Vexel sind keine Werkzeuge. Sie sind... Freunde.|Ich halte die Sicherheitsdrohnen auf. Hier, deine Vexel sollten voll geladen sein, bevor du nach oben gehst.");
        heal_party();
        sfx(SFX_HEAL);
        say(NULL, "Kai hat dein Team vollständig geheilt!");
        refresh_npcs();
        for (int i = 0; i < nnpc; i++)
            if (npcs[i].def->kind == NK_TRAINER && npcs[i].def->arg == 34) npcs[i].visible = 0;
    }
    if (tid == 37) {
        say("NULL", "Du bist der erste, der mich besiegt hat. Nimm diese Erinnerung mit: Jeder Code kann sich verändern. Genau wie jeder Vexel.");
    }
    (void)buf;
}

static int do_trainer(int tid, Npc *o)
{
    const TrainerData *T = &trainers[tid];
    if (o) {
        o->dir = face_toward(o->x, o->y, px, py);
        pdir = face_toward(px, py, o->x, o->y);
    }
    redraw();
    say(T->name, T->intro);
    int r = battle_trainer(tid);
    world_draw();
    frame();
    if (r == BR_WIN) {
        set_flag(F_TRAINER_BASE + tid);
        say(T->name, T->lose);
        post_trainer(tid);
        bottom_need = 1;
        return 1;
    }
    if (tid == 0) {      // first rival battle: no blackout
        heal_party();
        set_flag(F_TRAINER_BASE + tid);
        return 1;
    }
    blackout();
    return 0;
}

// ------------------------------------------------------------------ gyms & league
static int lines_of_type(int t1, int t2, int *out, int maxn)
{
    int n = 0;
    for (int l = 0; l < NUM_LINES && n < maxn; l++)
        if (!IS_LEGEND_LINE(l) && (mlines[l].type == t1 || (t2 != t1 && mlines[l].type == t2))) out[n++] = l;
    return n;
}

static int build_team(Mon *team, int t1, int t2, int n, int ace, int seed)
{
    int ls[40];
    int cnt = lines_of_type(t1, t2, ls, 40);
    if (n > 6) n = 6;
    for (int i = 0; i < n; i++) {
        int line = ls[(seed * 7 + i * 3) % cnt];
        int lv = ace - (n - 1 - i) * 2;
        if (lv < 3) lv = 3;
        if (lv > 100) lv = 100;
        mon_create(&team[i], species_for_level(line, lv), lv);
    }
    return n;
}

static void badge_story(void)
{
    int b = badge_count();
    char buf[160];
    fmt(buf, "Du besitzt jetzt %d von 24 Orden.", b);
    say(NULL, buf);
    if (b >= 1 && !flag(F_MSG1)) {
        set_flag(F_MSG1);
        say("Lyra (Hologramm)", "Nova... wenn du das siehst, hast du den ersten Netzknoten geöffnet.|Protokoll Null ist kein Virus. Es ist ein Schlüssel – ein Schlüssel, der die Vexel befreien kann.|Ich habe es auf alle 24 Knoten verteilt. Acht Fragmente genügen, um es wieder zusammenzusetzen.");
    }
    if (b >= 1 && !flag(F_KEY_DRONE)) {
        say("Lyra (Hologramm)", "Und noch etwas: Im Knoten habe ich dir meine Gleitdrohne versteckt. Damit erreichst du jede Stadt, die du schon kennst.");
        give_key(F_KEY_DRONE, NULL, "Du erhältst die GLEITDROHNE! Menü → Ausrüstung: Flieg zu jeder besuchten Stadt.");
    }
    if (b >= 3 && !flag(F_MSG2)) {
        set_flag(F_MSG2);
        say("Lyra (Hologramm)", "Drei Fragmente. Gut gemacht.|Viktor will die Vexel nicht nur benutzen. Er will sie alle kontrollieren – über eine KI namens OMEGA.|Wenn OMEGA Protokoll Null umkehrt, gehorcht jeder Vexel der Welt nur noch einem Willen.");
    }
    if (b >= 6 && !flag(F_MSG3)) {
        set_flag(F_MSG3);
        say("Lyra (Hologramm)", "Nova... ich muss dir etwas sagen.|Ich bin nicht geflohen. Viktor hat meinen Geist in OMEGA hochgeladen.|Ich halte OMEGA von innen auf. Aber ich werde schwächer. Beeil dich.");
    }
    if (b >= 8 && !flag(F_MSG4)) {
        set_flag(F_MSG4);
        set_flag(F_TOWER_OPEN);
        say("Lyra (Hologramm)", "Acht Fragmente. Protokoll Null ist vollständig.|Der Helix-Turm in Sektor Null lässt dich jetzt ein. Besiege den Hohen Rat und erreiche den OMEGA-Kern.|Und Nova – Viktor ist nicht mehr der, der er war.");
        say(NULL, "Die Wachen am Helix-Turm in Sektor Null treten jetzt zur Seite!");
    }
}

static int do_gym(int g, Npc *o)
{
    const GymData *Gd = &gyms[g];
    char buf[200];
    int b = badge_count();
    int n = 2 + b / 3;
    int ace = 12 + (b * 7) / 2;
    if (n > 6) n = 6;
    o->dir = face_toward(o->x, o->y, px, py);
    redraw();
    say(Gd->name, Gd->intro);
    fmt(buf, "Du hast %d Orden. Also kämpfe ich mit %d Vexel bis Level %d!", b, n, ace > 100 ? 100 : ace);
    say(Gd->name, buf);
    Mon team[6];
    build_team(team, Gd->t1, Gd->t2, n, ace, g);
    int r = battle_team(Gd->name, Gd->chr, team, n, 600 + b * 200, 1, world_style_bg(), MUS_BOSS);
    world_draw();
    frame();
    if (r != BR_WIN) { blackout(); return 0; }
    set_flag(F_BADGE_BASE + g);
    say(Gd->name, Gd->lose);
    sfx(SFX_LEVEL);
    fmt(buf, "Du erhältst den %s-Orden und ein Fragment von Protokoll Null!", type_names[Gd->t1]);
    say(NULL, buf);
    {
        // every gym also hands out a held item
        static const u8 reward[NUM_GYMS] = { HI_DATENBEERE, HI_TURBOCHIP, HI_NANOREP, HI_KLARBEERE, HI_TECHNIKMODUL, HI_ZIELCHIP,
                                             HI_SAUGMODUL, HI_STASISKERN, HI_PANZERWESTE, HI_KRAFTMODUL, HI_UEBERLAST, HI_NOTFALLCHIP,
                                             HI_KLIMAKERN, HI_LERNCHIP, HI_NANOREP, HI_CREDITCHIP, HI_UEBERLAST, HI_NOTFALLCHIP,
                                             HI_KRAFTMODUL, HI_TECHNIKMODUL, HI_NEBELKAPSEL, HI_LERNCHIP, HI_PANZERWESTE, HI_ZIELCHIP };
        int h = reward[g];
        if (G.held_inv[h - 1] < 99) G.held_inv[h - 1]++;
        fmt(buf, "%s gibt dir außerdem: %s! (%s)", Gd->name, held_items[h].name, held_items[h].desc);
        say(NULL, buf);
    }
    badge_story();
    bottom_need = 1;
    return 1;
}

static int do_gym_trainer(int k, Npc *o)
{
    const GymData *Gd = &gyms[k / 2];
    char name[40];
    int b = badge_count();
    int n = 1 + b / 6;
    int ace = 9 + (b * 7) / 2;
    if (n > 4) n = 4;
    fmt(name, "%s-Adept%s", type_names[(k & 1) ? Gd->t2 : Gd->t1], (k & 1) ? "in" : "");
    o->dir = face_toward(o->x, o->y, px, py);
    pdir = face_toward(px, py, o->x, o->y);
    redraw();
    say(name, (k & 1) ? "Bevor du zur Leitung darfst, musst du an mir vorbei!" : "Willkommen in der Arena! Zeig, ob du bereit bist.");
    Mon team[6];
    build_team(team, (k & 1) ? Gd->t2 : Gd->t1, Gd->t1, n, ace, k + 11);
    int r = battle_team(name, o->chr, team, n, 200 + b * 60, 0, world_style_bg(), MUS_BATTLE);
    world_draw();
    frame();
    if (r != BR_WIN) { blackout(); return 0; }
    set_flag(F_GYMTR_BASE + k);
    say(name, "Gut gekämpft. Die Leitung erwartet dich.");
    return 1;
}

static const struct { const char *name; u8 t1, t2; const char *intro, *lose; } elites[4] = {
    { "Direktor Krell", T_ARSENAL, T_METALL, "Ich bin Krell, Rüstungsdirektor von Helix. Jede Waffe in diesem Turm trägt meine Signatur.", "Meine Waffen... versagt." },
    { "Agentin Vex", T_VIRUS, T_VOID, "Ich bin Vex. Ich lösche Kuriere wie dich, bevor sie Fragen stellen.", "Datei nicht gefunden: Sieg." },
    { "Admin Sera", T_PSI, T_KOSMOS, "Sera, Sicherheitschefin. Ich habe jede Variable berechnet. Auch dich.", "Unmöglich... du warst nicht in meiner Rechnung." },
    { "General Brakk", T_DRACO, T_DAEMON, "Ich bin Brakk, der letzte Wall vor dem Orbitalgarten. Drachen und Dämonen gehorchen mir!", "Der Wall... ist gefallen. Geh. Der Orbitalgarten wartet." },
};

static int do_elite(int i, Npc *o)
{
    int b = badge_count();
    int ace = 18 + (b * 7) / 2 + i * 2;
    o->dir = face_toward(o->x, o->y, px, py);
    redraw();
    say(elites[i].name, elites[i].intro);
    Mon team[6];
    build_team(team, elites[i].t1, elites[i].t2, i < 2 ? 5 : 6, ace, 40 + i);
    int r = battle_team(elites[i].name, o->chr, team, i < 2 ? 5 : 6, 3000 + i * 1000, 1, 5, MUS_BOSS);
    world_draw();
    frame();
    if (r != BR_WIN) { blackout(); return 0; }
    say(elites[i].name, elites[i].lose);
    set_flag(F_ELITE_BASE + i);
    if (i == 2) set_flag(F_ADMIN);
    refresh_npcs();
    if (i == 3) say(NULL, "Du hast den Hohen Rat besiegt! Der Weg zum Orbitalgarten ist frei.");
    else say(NULL, "Der Durchgang öffnet sich.");
    return 1;
}

// ------------------------------------------------------------------ champion post-game
// recurring challengers: they level up with every fight against the player
static int challenger_level(int c)
{
    int lv = 50 + G.rematch[c] * 4 + (c % 5);
    return lv > 100 ? 100 : lv;
}

static int challenger_team(int c, Mon *team)
{
    static const int starters[3] = { 0, 10, 20 };
    const ChallengerData *C = &challengers[c];
    int n = 3 + G.rematch[c] / 2;
    if (n > 6) n = 6;
    int ace = challenger_level(c);
    for (int i = 0; i < n; i++) {
        int line = C->lines[6 - n + i];
        if (line == 0xFFFF) line = starters[(G.starter + 1) % 3];
        int lv = ace - (n - 1 - i) * 2;
        mon_create(&team[i], species_for_level(line, lv), lv);
    }
    return n;
}

// fight challenger c; returns battle result. heal=0 keeps damage (title defense chain)
static int challenger_fight(int c)
{
    const ChallengerData *C = &challengers[c];
    Mon team[6];
    int n = challenger_team(c, team);
    int r = battle_team(C->name, C->chr, team, n, 1500 + G.rematch[c] * 300, 1, world_style_bg(), MUS_BOSS);
    if (G.rematch[c] < 255) G.rematch[c]++;
    world_draw();
    frame();
    return r;
}

static void challenger_intro(int c)
{
    const ChallengerData *C = &challengers[c];
    char buf[200];
    if (G.rematch[c] == 0) say(C->name, C->intro);
    else {
        static const char *const again[] = {
            "Da bist du ja, Champion! Ich habe trainiert. Mein Team ist jetzt Level %d!",
            "Revanche! Meine Vexel sind auf Level %d gewachsen.",
            "Ich habe dich gesucht! Level %d – diesmal reicht es!",
            "Champion! Kampf Nummer %d zwischen uns. Los geht's!",
        };
        int k = (c + G.rematch[c]) & 3;
        fmt(buf, again[k], k == 3 ? G.rematch[c] + 1 : challenger_level(c));
        say(C->name, buf);
    }
}

// spawn an NPC up to 'maxd' tiles away in direction d from the player; it walks next to the player
static Npc *spawn_walker(int chr, int maxd, int d)
{
    if (nnpc >= MAX_NPC) return NULL;
    int dx, dy;
    dir_delta(d, &dx, &dy);
    int dist = 0;
    for (int k = 1; k <= maxd; k++) {
        if (solid_at(px + dx * k, py + dy * k)) break;
        dist = k;
    }
    if (dist < 2) return NULL;
    Npc *o = &npcs[nnpc++];
    chal_npc = (NpcDef){ px + dx * dist, py + dy * dist, chr, D_DOWN, NK_CHALLENGER, 0, NOF, NOF, NULL, NULL };
    o->def = &chal_npc;
    o->x = chal_npc.x; o->y = chal_npc.y; o->chr = chr; o->ox = o->oy = 0; o->moving = 0; o->visible = 1;
    o->dir = d == D_UP ? D_DOWN : d == D_DOWN ? D_UP : d == D_LEFT ? D_RIGHT : D_LEFT;
    exclaim(o);
    sfx(SFX_ENCOUNTER);
    while (!(o->x - dx == px && o->y - dy == py)) npc_walk(o, o->dir, 1);
    pdir = d;
    redraw();
    return o;
}

static void despawn_walker(Npc *o)
{
    int back = o->dir == D_UP ? D_DOWN : o->dir == D_DOWN ? D_UP : o->dir == D_LEFT ? D_RIGHT : D_LEFT;
    npc_walk(o, back, 2);
    o->visible = 0;
    redraw();
    nnpc--;
}

// how fast the "challenge clock" runs here (per step, in quarters). Cities count fully, so at least
// 750 steps pass between two challenges; elsewhere it takes longer, inside buildings never.
static int challenge_weight(int m)
{
    switch (m) {
    case M_ROST: case M_NEON: case M_AQUARIS: case M_KRISHAIN: case M_SEKTOR0: case M_CHROM: case M_DRACHEN:
    case M_GLUT: case M_HIMMEL: case M_SCHATTEN: case M_ORBITAL: case M_URWALD: case M_NANORIFF:
        return 4;
    case M_KABEL: case M_HYDRO: case M_R_DATA: case M_R_TIEFSEE: case M_R_RAUM: case M_ORBIT:
        return 3;
    case M_R_SCHROTT: case M_R_LAVA: case M_R_HIMMEL: case M_R_SCHATTEN: case M_R_DSCHUNGEL:
        return 2;
    case M_CAVE: case M_GLITCH: case M_ABYSS: case M_AETHERBERG:
        return 1;
    }
    return 0;
}

static int last_chal = -1;
static int pick_challenger(void)
{
    int c;
    do c = rnd() % NUM_CHALLENGERS; while (c == last_chal);
    last_chal = c;
    return c;
}

static void try_challenge(void)
{
    if (!flag(F_CHAMPION) || G.party_n == 0 || !battle_party_alive()) return;
    int w = challenge_weight(cur_map);
    if (!w) return;
    if (G.chal_target < 750 || G.chal_target > 2500) G.chal_target = rnd_range(750, 2500);
    if (G.chal_prog < 0xFFF0) G.chal_prog += w;
    if (G.chal_prog < G.chal_target * 4) return;
    // the clock has run out: the next challenger shows up around here (random extra steps)
    if (rnd() % 8) return;
    int c = pick_challenger();
    LOG("[challenge] map %d challenger %d\n", cur_map, c);
    Npc *o = NULL;
    int d0 = rnd() & 3;
    for (int k = 0; k < 4 && !o; k++) o = spawn_walker(challengers[c].chr, 5, (d0 + k) & 3);
    if (!o) return;
    G.chal_prog = 0;
    G.chal_target = rnd_range(750, 2500);
    challenger_intro(c);
    if (!ask_yesno(challengers[c].name, "Nimmst du die Herausforderung an, Champion?")) {
        say(challengers[c].name, "Kneifen gilt nicht! Ich finde dich wieder...");
        despawn_walker(o);
        return;
    }
    int r = challenger_fight(c);
    if (r == BR_WIN) {
        G.chal_wins++;
        say(challengers[c].name, challengers[c].lose);
        say(challengers[c].name, "Ich trainiere weiter. Beim nächsten Mal bin ich stärker!");
        if (rnd() % 3 == 0) {
            static const u8 gifts[] = { HI_DATENBEERE, HI_KLARBEERE, HI_DATENBEERE, HI_NANOREP, HI_UEBERLAST, HI_NOTFALLCHIP };
            int h = gifts[rnd() % sizeof(gifts)];
            if (G.held_inv[h - 1] < 99) G.held_inv[h - 1]++;
            char gb[100];
            fmt(gb, "Als Respekt schenkt %s dir: %s!", challengers[c].name, held_items[h].name);
            say(NULL, gb);
        }
        despawn_walker(o);
    } else {
        o->visible = 0; nnpc--;
        blackout();
    }
    bottom_need = 1;
}

static void script_throne(void)
{
    char buf[200];
    fmt(buf, "Der Thron des Champions. Erfolgreiche Titelverteidigungen: %d.", G.defenses);
    say(NULL, buf);
    if (!ask_yesno(NULL, "Titel verteidigen? Drei Herausforderer nacheinander, ohne Heilung dazwischen.")) return;
    // the player stands in front of the throne, challengers come up the carpet
    int picks[3];
    for (int i = 0; i < 3; i++) {
        int ok;
        do { picks[i] = rnd() % NUM_CHALLENGERS; ok = 1; for (int j = 0; j < i; j++) if (picks[j] == picks[i]) ok = 0; } while (!ok);
    }
    music_play(MUS_TOWER);
    for (int i = 0; i < 3; i++) {
        int c = picks[i];
        fmt(buf, "Herausforderer %d von 3: %s!", i + 1, challengers[c].name);
        say("Hallensprecherin", buf);
        Npc *o = spawn_walker(challengers[c].chr, 7, D_DOWN);
        challenger_intro(c);
        int r = challenger_fight(c);
        if (r != BR_WIN) {
            if (o) { o->visible = 0; nnpc--; }
            say("Hallensprecherin", "Der Champion wurde besiegt! Die Verteidigung ist gescheitert...");
            blackout();
            return;
        }
        say(challengers[c].name, challengers[c].lose);
        if (o) despawn_walker(o);
        pdir = D_UP;
        redraw();
    }
    G.defenses++;
    int prize = 5000 + G.defenses * 500;
    G.money += prize;
    sfx(SFX_LEVEL);
    fmt(buf, "Titel verteidigt! Das ist deine %d. erfolgreiche Verteidigung. Du erhältst %d Cr.", G.defenses, prize);
    say("Hallensprecherin", buf);
    if (G.defenses % 5 == 0) {
        G.items[IT_OMEGAKAPSEL]++;
        say(NULL, "Als Jubiläumspreis erhältst du eine Omegakapsel!");
    }
    heal_party();
    say("NURSE-0", "Deine Vexel werden jetzt wieder vollständig aufgeladen. Glückwunsch, Champion!");
    music_play(maps[cur_map].music);
    bottom_need = 1;
}

static void script_ranking(void)
{
    static char rows[NUM_CHALLENGERS][48];
    static const char *opts[NUM_CHALLENGERS];
    int order[NUM_CHALLENGERS];
    for (int i = 0; i < NUM_CHALLENGERS; i++) order[i] = i;
    for (int i = 1; i < NUM_CHALLENGERS; i++) {   // strongest first
        int t = order[i], j = i - 1;
        while (j >= 0 && challenger_level(order[j]) < challenger_level(t)) { order[j + 1] = order[j]; j--; }
        order[j + 1] = t;
    }
    for (int i = 0; i < NUM_CHALLENGERS; i++) {
        int c = order[i];
        fmt(rows[i], "Lv%d %s (%dx)", challenger_level(c), challengers[c].name, G.rematch[c]);
        opts[i] = rows[i];
    }
    char title[48];
    fmt(title, "Rangliste – %d Siege", G.chal_wins + G.defenses * 3);
    menu_list(title, opts, NUM_CHALLENGERS, 0);
    bottom_need = 1;
}

// ------------------------------------------------------------------ legendary Urcodes
static const char *const legend_where[NUM_LEGENDS] = {
    "auf der Himmelstreppe nördlich vom Kristallhain", "auf dem Tiefseesteg südlich von Aquaris", "in der Kristallhöhle nördlich von Aquaris",
    "im Lavafeld westlich von Drachenfels", "auf dem Dschungelpfad nördlich von Drachenfels", "in der Schattengasse südlich der Glitchwüste",
    "im Orbitalgarten über dem Helix-Turm", "auf dem Gipfel des Aetherbergs nördlich von Himmelsspitze",
};
static const char *const legend_roar[NUM_LEGENDS] = {
    "Blitze zucken über den Himmel. VOLTARIS breitet seine Stahlschwingen aus!",
    "Das Meer kocht. ABYSSOR steigt aus der Tiefe, die Kanonen auf dich gerichtet!",
    "Die Kristalle singen. KRISTALLON erscheint in einem Regen aus Licht!",
    "Die Lava bäumt sich auf. PYROKRON brüllt, und die Luft brennt!",
    "Der Boden bebt. GAIARCH bricht aus dem Dschungel, Ranken um die Kanonen geschlungen!",
    "Alle Bildschirme flackern. NULLOX kriecht aus dem Schatten!",
    "Die Sterne werden hell. ASTRAEL schwebt herab, das Schwert erhoben!",
    "Die Zeit bleibt stehen. AETHERION, der erste Code, öffnet die Augen!",
};

static void cult_team(Mon *team, int k, int n, int ace)
{
    static const u8 tt[4][2] = { { T_VOID, T_VIRUS }, { T_DAEMON, T_VOID }, { T_VIRUS, T_DAEMON }, { T_KOSMOS, T_VOID } };
    build_team(team, tt[k & 3][0], tt[k & 3][1], n, ace, 70 + k);
}

static int do_legend(int k, Npc *o)
{
    int sp = FIRST_LEGEND_SPECIES + k;
    int lv = k == 7 ? 85 : 60 + k * 3;
    char buf[220];
    if (!flag(F_CULT_BASE + k)) {
        Mon team[6];
        int r;
        if (k < 7) {
            say(NULL, "Ein mächtiges Wesen schläft hier. Plötzlich hörst du Schritte...");
            Npc *c = NULL;
            static const u8 order[3] = { 1, 2, 3 };   // behind the player first, then the sides
            for (int i = 0; i < 3 && !c; i++) c = spawn_walker(CH_CULT, 4, pdir ^ order[i]);
            say("Kultist", k == 0 ? "Halt! Der Kult des Urcodes beansprucht dieses Wesen. Meister Xeno wird es in sein Netz binden!"
                                  : "Du schon wieder, Champion?! Der Kult lässt sich nicht aufhalten!");
            int n = 3 + k / 2;
            cult_team(team, k, n, lv - 3);
            r = battle_team("Kultist", CH_CULT, team, n, 2000 + k * 300, 1, world_style_bg(), MUS_BOSS);
            world_draw(); frame();
            if (r != BR_WIN) { if (c) { c->visible = 0; nnpc--; } blackout(); return 0; }
            say("Kultist", "Unmöglich... Meister Xeno wird davon erfahren!");
            if (c) despawn_walker(c);
        } else {
            say(NULL, "Auf dem Gipfel wartet eine Gestalt in einem Mantel aus Licht.");
            Npc *c = spawn_walker(CH_XENO, 3, D_DOWN);
            say("Xeno", "Champion Nova Voss. Du hast sieben Urcodes geweckt – genau wie ich es geplant habe.|Ihre Signale haben Aetherion erweckt. Jetzt binde ich den ersten Code an mich. Und mit ihm jeden Vexel der Welt.");
            say("Xeno", "Protokoll Null hat die Vexel befreit. Ich werde sie wieder einsammeln. Für immer!");
            Mon team[6];
            build_team(team, T_KOSMOS, T_VOID, 6, 84, 99);
            r = battle_team("Xeno", CH_XENO, team, 6, 12000, 1, 6, MUS_FINAL);
            world_draw(); frame();
            if (r != BR_WIN) { if (c) { c->visible = 0; nnpc--; } blackout(); return 0; }
            say("Xeno", "Mein Netz... zerrissen. Vielleicht... gehören die Vexel wirklich niemandem.");
            if (c) despawn_walker(c);
        }
        set_flag(F_CULT_BASE + k);
    }
    o->dir = face_toward(o->x, o->y, px, py);
    for (int i = 0; i < 24; i++) { world_draw(); fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, (i & 4) ? 6 : 0); frame(); }
    say(NULL, legend_roar[k]);
    int first = !flag(F_LEG_MET_BASE + k);
    set_flag(F_LEG_MET_BASE + k);
    sfx(SFX_ENCOUNTER);
    int r = battle_wild_ex(sp, lv, world_style_bg(), MUS_FINAL);
    music_play(maps[cur_map].music);
    world_draw(); frame();
    if (r == BR_LOSE) { blackout(); return 0; }
    if (r == BR_CAUGHT) {
        set_flag(F_LEG_CAUGHT_BASE + k);
        o->visible = 0;
        fmt(buf, "Der Urcode %s schließt sich dir an!", species[sp].name);
        say(NULL, buf);
    } else if (r == BR_WIN) {
        fmt(buf, "%s zerfällt in leuchtende Daten... aber der Code lebt weiter. Komm zurück, um es erneut herauszufordern.", species[sp].name);
        say(NULL, buf);
    } else {
        fmt(buf, "%s beobachtet dich schweigend. Es wartet hier auf dich.", species[sp].name);
        say(NULL, buf);
    }
    refresh_npcs();
    if (first && k < 7) {
        int n = 0;
        for (int i = 0; i < NUM_LEGENDS; i++) n += flag(F_LEG_MET_BASE + i);
        if (k == 6) fmt(buf, "Sieben Urcodes sind erwacht. Ein Signal, stärker als alle anderen, kommt vom Aetherberg nördlich von Himmelsspitze. Xeno ist schon unterwegs!");
        else fmt(buf, "%d von 8 Urcodes sind erwacht. Das nächste Signal kommt %s!", n, legend_where[k + 1]);
        say("Dr. Brandt (Funk)", buf);
    }
    if (first && k == 7 && !flag(F_LEGEND_DONE)) {
        set_flag(F_LEGEND_DONE);
        say("Lyra (Funk)", "Nova... Aetherion ist erwacht und frei. Alle acht Urcodes leben wieder – und kein Kult wird sie je binden.|Du bist nicht nur Champion. Du bist die Hüterin des alten Netzes. Ich bin so stolz auf dich.");
        G.items[IT_OMEGAKAPSEL] += 3;
        sfx(SFX_LEVEL);
        say(NULL, "Du erhältst 3 Omegakapseln! Gefangene Urcodes erscheinen nicht wieder – besiegte kannst du erneut herausfordern.");
    }
    bottom_need = 1;
    return 1;
}

static void become_champion(void)
{
    set_flag(F_CHAMPION);
    set_flag(F_LEGEND_START);
    sfx(SFX_LEVEL);
    say(NULL, "Du bist jetzt CHAMPION von Neo-Kyra!");
    say("Dr. Brandt (Funk)", "Nova! Glückwunsch, Champion! Aber hör zu: Seit Protokoll Null aktiv ist, empfangen wir seltsame AETHER-Signale.|Acht uralte Codes sind erwacht – die URCODES. Legendäre Vexel, älter als das Netz selbst.|Und ein Kult jagt sie: der Kult des Urcodes, angeführt von einem gewissen Xeno.");
    {
        char buf[200];
        fmt(buf, "Das erste Signal kommt %s. Finde die Urcodes, bevor Xeno sie bindet!", legend_where[0]);
        say("Dr. Brandt (Funk)", buf);
    }
    say("Dr. Brandt (Funk)", "Und noch etwas: Als Champion wirst du jetzt überall herausgefordert, vor allem in den Städten.|Im Neon-Distrikt wurde dir die Champion-Halle eröffnet. Dort kannst du deinen Titel verteidigen.");
    refresh_npcs();
}

// ------------------------------------------------------------------ classic features
// cities for the region map and fast travel (station door, position on the map)
const CityData cities[NUM_CITIES] = {
    { M_ROST, P_ROST_STATION_X, P_ROST_STATION_Y, 126, 172, "Rost-Sektor" },
    { M_NEON, P_NE_STATION_X, P_NE_STATION_Y, 126, 132, "Neon-Distrikt" },
    { M_AQUARIS, P_AQ_STATION_X, P_AQ_STATION_Y, 166, 132, "Aquaris" },
    { M_KRISHAIN, P_KH_STATION_X, P_KH_STATION_Y, 166, 92, "Kristallhain" },
    { M_SEKTOR0, P_S0_STATION_X, P_S0_STATION_Y, 210, 92, "Sektor Null" },
    { M_CHROM, P_CHROM_STATION_X, P_CHROM_STATION_Y, 88, 132, "Chromstadt" },
    { M_DRACHEN, P_DRACHEN_STATION_X, P_DRACHEN_STATION_Y, 50, 132, "Drachenfels" },
    { M_GLUT, P_GLUT_STATION_X, P_GLUT_STATION_Y, 14, 132, "Glutschmiede" },
    { M_HIMMEL, P_HIMMEL_STATION_X, P_HIMMEL_STATION_Y, 166, 50, "Himmelsspitze" },
    { M_SCHATTEN, P_SCHATTEN_STATION_X, P_SCHATTEN_STATION_Y, 188, 132, "Schattenmarkt" },
    { M_ORBITAL, P_ORBITAL_STATION_X, P_ORBITAL_STATION_Y, 242, 92, "Orbitalhafen" },
    { M_URWALD, P_URWALD_STATION_X, P_URWALD_STATION_Y, 50, 92, "Urwaldkuppel" },
    { M_NANORIFF, P_NANORIFF_STATION_X, P_NANORIFF_STATION_Y, 166, 176, "Nanoriff" },
};

int city_index(int map)
{
    for (int i = 0; i < NUM_CITIES; i++) if (cities[i].map == map) return i;
    return -1;
}

static int is_outdoor(int m)
{
    if (maps[m].outside == ' ') return 0;
    switch (m) {
    case M_LOBBY: case M_LIGA: case M_CORE: case M_CHAMPHALL: case M_CAVE: case M_ABYSS: case M_ORBIT: return 0;
    }
    return 1;
}

// where on the region map the player is (routes sit between their cities)
static void map_pos(int m, int *x, int *y)
{
    static const struct { u8 map, x, y; } extra[] = {
        { M_KABEL, 126, 152 }, { M_HYDRO, 146, 132 }, { M_CAVE, 166, 112 }, { M_GLITCH, 188, 92 }, { M_R_DATA, 107, 132 },
        { M_R_SCHROTT, 69, 132 }, { M_R_LAVA, 32, 132 }, { M_R_DSCHUNGEL, 50, 112 }, { M_R_TIEFSEE, 166, 154 },
        { M_R_HIMMEL, 166, 71 }, { M_R_SCHATTEN, 188, 112 }, { M_R_RAUM, 226, 92 }, { M_AETHERBERG, 166, 22 },
        { M_LOBBY, 210, 60 }, { M_LIGA, 210, 60 }, { M_ORBIT, 210, 60 }, { M_CORE, 210, 60 }, { M_ABYSS, 210, 60 },
    };
    int c = city_index(m);
    if (c >= 0) { *x = cities[c].mx; *y = cities[c].my; return; }
    if (m == M_CHAMPHALL) { *x = 126; *y = 132; return; }
    for (unsigned i = 0; i < N(extra); i++) if (extra[i].map == m) { *x = extra[i].x; *y = extra[i].y; return; }
    m = G.ret_map; c = city_index(m);   // shared interiors
    if (c >= 0) { *x = cities[c].mx; *y = cities[c].my; return; }
    *x = 126; *y = 132;
}

static void draw_region_map(u16 *fb, int sel, int fly)
{
    gradient_v(fb, 0, 0, SCR_W, SCR_H, RGB(1, 4, 10), RGB(0, 1, 4));
    for (int y = 0; y < SCR_H; y += 6) fill_blend(fb, 0, y, SCR_W, 1, C_CYAN, 1);
    static const u8 lines_[][4] = {   // x0 y0 x1 y1 (orthogonal routes)
        { 126, 132, 126, 172 }, { 126, 132, 166, 132 }, { 166, 92, 166, 132 }, { 166, 92, 210, 92 }, { 14, 132, 126, 132 },
        { 50, 92, 50, 132 }, { 166, 132, 166, 176 }, { 166, 50, 166, 92 }, { 188, 92, 188, 132 }, { 210, 92, 242, 92 },
        { 166, 22, 166, 50 }, { 210, 60, 210, 92 },
    };
    for (unsigned i = 0; i < N(lines_); i++) {
        int x0 = lines_[i][0], y0 = lines_[i][1], x1 = lines_[i][2], y1 = lines_[i][3];
        if (i == 10 && !flag(F_CHAMPION)) continue;
        fill(fb, x0 - 1, y0 - 1, x1 - x0 + 3, y1 - y0 + 3, RGB(6, 9, 16));
        fill(fb, x0, y0, x1 - x0 + 1, y1 - y0 + 1, RGB(10, 16, 26));
    }
    // Helix tower and Aetherberg
    fill(fb, 205, 52, 11, 14, RGB(8, 8, 14)); fill(fb, 208, 48, 5, 6, C_MAGENTA);
    if (flag(F_CHAMPION)) { fill(fb, 160, 16, 13, 10, RGB(20, 22, 30)); fill(fb, 164, 12, 5, 5, C_WHITE); }
    for (int i = 0; i < NUM_CITIES; i++) {
        int vis = flag(F_VISIT_BASE + i);
        u16 c = vis ? C_CYAN : RGB(8, 10, 14);
        int x = cities[i].mx, y = cities[i].my;
        fill(fb, x - 5, y - 5, 11, 11, i == sel ? C_YELLOW : RGB(2, 3, 6));
        fill(fb, x - 3, y - 3, 7, 7, c);
    }
    int px_, py_;
    map_pos(cur_map, &px_, &py_);
    if ((frame_count >> 4) & 1) { fill(fb, px_ - 2, py_ - 2, 5, 5, C_MAGENTA); fill(fb, px_ - 1, py_ - 1, 3, 3, C_WHITE); }
    panel_solid(fb, 4, 2, 248, 16, RGB(2, 2, 8), C_MAGENTA);
    char buf[64];
    if (sel >= 0) fmt(buf, "%s%s", cities[sel].name, flag(F_VISIT_BASE + sel) ? "" : "  (noch nicht besucht)");
    else fmt(buf, "Neo-Kyra – du bist hier: %s", maps[cur_map].name);
    draw_text_sh(fb, 10, 5, buf, C_WHITE, C_BLACK);
    if (fly) draw_text_sh(fb, 8, 182, "Steuerkreuz: Ziel   A: Fliegen   B: Zurück", C_TEXT2, C_BLACK);
    else draw_text_sh(fb, 8, 182, "B: Zurück", C_TEXT2, C_BLACK);
}

// region map; with the glide drone the player can fly to any visited city. Returns 1 after flying.
static int region_map(int fly)
{
    int sel = city_index(cur_map);
    if (sel < 0 || !flag(F_VISIT_BASE + sel)) sel = 0;
    if (!fly) sel = -1;
    bot_snapshot();
    for (;;) {
        draw_region_map(fb_bot, sel, fly);
        bot_dirty = 1;
        frame();
        if (key_down & K_B) { sfx(SFX_CANCEL); bot_restore(); return 0; }
        if (!fly) { if (key_down & K_A) { bot_restore(); return 0; } continue; }
        int dx = (key_rep & K_RIGHT) ? 1 : (key_rep & K_LEFT) ? -1 : 0, dy = (key_rep & K_DOWN) ? 1 : (key_rep & K_UP) ? -1 : 0;
        if (dx || dy) {
            // nearest visited city in that direction
            int best = -1, bd = 1 << 30;
            for (int i = 0; i < NUM_CITIES; i++) {
                if (i == sel || !flag(F_VISIT_BASE + i)) continue;
                int ddx = cities[i].mx - cities[sel].mx, ddy = cities[i].my - cities[sel].my;
                int along = ddx * dx + ddy * dy, side = dx ? ddy : ddx;
                if (along <= 0) continue;
                int d = along + 2 * (side < 0 ? -side : side);
                if (d < bd) { bd = d; best = i; }
            }
            if (best >= 0) { sel = best; sfx(SFX_SELECT); }
        }
        if (touch_down)
            for (int i = 0; i < NUM_CITIES; i++)
                if (tx >= cities[i].mx - 8 && tx <= cities[i].mx + 8 && ty >= cities[i].my - 8 && ty <= cities[i].my + 8) {
                    if (sel == i) key_down |= K_A;
                    sel = i;
                }
        if ((key_down & K_A) && sel >= 0) {
            if (!flag(F_VISIT_BASE + sel)) { sfx(SFX_BUMP); continue; }
            sfx(SFX_CONFIRM);
            bot_restore();
            char buf[80];
            fmt(buf, "Die Gleitdrohne bringt dich nach %s!", cities[sel].name);
            say(NULL, buf);
            for (int i = 0; i < 16; i++) { world_draw(); fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, i); frame(); }
            G.bike = 0;
            world_warp(cities[sel].map, cities[sel].x, cities[sel].y + 1, D_DOWN);
            redraw();
            return 1;
        }
    }
}

// ---- fishing with the Datenangel
static void go_fishing(void)
{
    say(NULL, "Du wirfst die Datenangel aus...");
    int wait = 40 + rnd() % 80;
    for (int i = 0; i < wait; i++) { world_draw(); frame(); if (key_down & K_B) return; }
    if (rnd() % 4 == 0) { say(NULL, "Nichts hat angebissen."); return; }
    sfx(SFX_ENCOUNTER);
    for (int i = 0; i < 20; i++) {
        world_draw();
        int dx, dy; dir_delta(pdir, &dx, &dy);
        draw_text_sh(fb_top, (px + dx) * 16 - cam_x + 5, (py + dy) * 16 - cam_y - 4 - (i & 4), "!", C_YELLOW, C_BLACK);
        frame();
    }
    say(NULL, "Etwas hat angebissen!");
    static const u8 water_types[] = { T_HYDRO, T_HYDRO, T_HYDRO, T_KRISTALL, T_TOXIN, T_VOID };
    int ls[40];
    int t = water_types[rnd() % sizeof(water_types)];
    int n = lines_of_type(t, t, ls, 40);
    if (!n) return;
    int line;
    do line = ls[rnd() % n]; while ((line == 0 || line == 10 || line == 20) && n > 3);   // starters only from Dr. Brandt
    int lv = 5 + badge_count() * 3 + rnd() % 5;
    if (flag(F_CHAMPION)) lv += 10;
    if (lv > 100) lv = 100;
    int r = battle_wild(species_for_level(line, lv), lv, 2);
    if (r == BR_LOSE) blackout();
    bottom_need = 1;
    redraw();
}

// ---- partner Vexel that follows the player (HeartGold/SoulSilver style)
static int follower_species(void)
{
    if (!G.follow || G.bike || !is_outdoor(cur_map)) return -1;
    for (int i = 0; i < G.party_n; i++) if (G.party[i].hp > 0) return i;
    return -1;
}

static void follower_reset(void) { fol_x = px; fol_y = py; fol_vis = 0; fol_ox = fol_oy = 0; }

static void partner_talk(void)
{
    int i = follower_species();
    if (i < 0) return;
    Mon *m = &G.party[i];
    static const char *const lines[] = {
        "%s summt fröhlich vor sich hin.", "%s schaut sich neugierig um.", "%s stupst dich an. Es will weiter!",
        "%s blinkt in seinen Typfarben.", "%s scheint die Gegend zu mögen.", "%s gähnt. Ein langer Tag...",
        "%s lauscht. Irgendwo in der Nähe sind wilde Vexel.", "%s strahlt dich an. Ihr seid ein gutes Team!",
    };
    char buf[120];
    int k = (rnd() + m->level) % 8;
    if (day_phase() == DAY_NIGHT && k == 1) fmt(buf, "%s schaut zu den Sternen hoch.", species[m->species].name);
    else if (m->hp < m->maxhp / 3) fmt(buf, "%s wirkt erschöpft. Ihr solltet zu einer Ladestation.", species[m->species].name);
    else fmt(buf, lines[k], species[m->species].name);
    say(NULL, buf);
}

// ---- Vexel-Pension and eggs
static int pension_compat(void)   // 0 none, 1 low, 2 high
{
    if (G.pension_n < 2) return 0;
    const SpeciesData *a = &species[G.pension[0].species], *b = &species[G.pension[1].species];
    if (IS_LEGEND_LINE(a->line) || IS_LEGEND_LINE(b->line)) return 0;
    if (a->line == b->line) return 2;
    if (a->type1 == b->type1 || a->type1 == b->type2 || a->type2 == b->type1) return 1;
    return 0;
}

static int pension_levels(const Mon *m)
{
    int lv = m->level;
    while (lv < 100 && m->exp >= exp_for_level(lv + 1)) lv++;
    return lv;
}

static int egg_count(void)
{
    int n = 0;
    for (int i = 0; i < 3; i++) n += G.eggs[i].species != 0xFFFF;
    return n;
}

static void script_pension(void)
{
    char buf[200];
    if (G.egg_ready) {
        say("Pensionsleiterin Mo", "Ah, da bist du ja! Deine beiden Vexel haben... ein Datenei hinterlassen! Wir wissen auch nicht, wie das geht.");
        if (!flag(F_KEY_INCUBATOR)) {
            set_flag(F_KEY_INCUBATOR);
            sfx(SFX_LEVEL);
            say(NULL, "Du erhältst den INKUBATOR! Er wärmt bis zu 3 Eier, während du läufst. (Menü → Ausrüstung)");
        }
        if (egg_count() >= 3) say("Pensionsleiterin Mo", "Oh, dein Inkubator ist voll. Ich passe solange auf das Ei auf.");
        else {
            const Mon *p = &G.pension[rnd() & 1];
            for (int i = 0; i < 3; i++) if (G.eggs[i].species == 0xFFFF) {
                G.eggs[i].species = mlines[species[p->species].line].first;
                G.eggs[i].steps = 700 + rnd() % 600;
                G.eggs[i].mflags = (rnd() % 400 == 0) ? MF_SHINY : 0;
                G.eggs[i].nature = (rnd() & 1) ? G.pension[rnd() & 1].nature : rnd() % NUM_NATURES;
                G.eggs[i].iv_bonus = 20 + rnd() % 8;
                break;
            }
            G.egg_ready = 0;
            sfx(SFX_LEVEL);
            say(NULL, "Das Datenei liegt jetzt im Inkubator. Es schlüpft, wenn du genug Schritte gelaufen bist.");
        }
    }
    say("Pensionsleiterin Mo", "Willkommen in der Vexel-Pension! Ich passe auf bis zu zwei Vexel auf. Sie werden stärker, während du läufst.|Und wenn sich zwei gut verstehen... finden wir manchmal ein Ei.");
    for (;;) {
        char o0[48], o1[48];
        const char *opts[3];
        fmt(o0, "Vexel abgeben (%d/2)", G.pension_n);
        fmt(o1, "Vexel abholen");
        opts[0] = o0; opts[1] = o1; opts[2] = "Tschüss";
        int r = menu_list("VEXEL-PENSION", opts, 3, 0);
        if (r < 0 || r == 2) break;
        if (r == 0) {
            if (G.pension_n >= 2) { say("Pensionsleiterin Mo", "Ich habe schon zwei Gäste. Mehr schaffe ich nicht!"); continue; }
            if (G.party_n <= 1) { say("Pensionsleiterin Mo", "Du brauchst doch selbst noch einen Vexel!"); continue; }
            int t = party_menu(1, 0);
            if (t < 0) continue;
            G.pension[G.pension_n++] = G.party[t];
            mon_heal(&G.pension[G.pension_n - 1]);
            for (int i = t; i < G.party_n - 1; i++) G.party[i] = G.party[i + 1];
            G.party_n--;
            fmt(buf, "%s ist jetzt in der Pension.", species[G.pension[G.pension_n - 1].species].name);
            say("Pensionsleiterin Mo", buf);
            if (G.pension_n == 2) {
                static const char *const c[] = { "Die beiden ignorieren sich gegenseitig.", "Die beiden verstehen sich ganz gut.", "Die beiden sind ein Herz und eine Seele!" };
                say("Pensionsleiterin Mo", c[pension_compat()]);
            }
        } else {
            if (!G.pension_n) { say("Pensionsleiterin Mo", "Bei mir wohnt gerade niemand."); continue; }
            if (G.party_n >= PARTY_MAX) { say("Pensionsleiterin Mo", "Dein Team ist voll!"); continue; }
            const char *names[3];
            char nb[2][48];
            for (int i = 0; i < G.pension_n; i++) {
                Mon *m = &G.pension[i];
                fmt(nb[i], "%s Lv%d (+%d)", species[m->species].name, pension_levels(m), pension_levels(m) - m->level);
                names[i] = nb[i];
            }
            names[G.pension_n] = "Zurück";
            int k = menu_list("ABHOLEN", names, G.pension_n + 1, 0);
            if (k < 0 || k >= G.pension_n) continue;
            Mon m = G.pension[k];
            int gained = pension_levels(&m) - m.level;
            int fee = 100 + gained * 100;
            fmt(buf, "Das macht %d Credits. %s ist %d Level gewachsen.", fee, species[m.species].name, gained);
            if (!ask_yesno("Pensionsleiterin Mo", buf)) continue;
            if ((int)G.money < fee) { say("Pensionsleiterin Mo", "Oh, du hast nicht genug Credits dabei."); continue; }
            G.money -= fee;
            m.level = pension_levels(&m);
            mon_calc(&m);
            m.hp = m.maxhp;
            G.party[G.party_n++] = m;
            for (int i = k; i < G.pension_n - 1; i++) G.pension[i] = G.pension[i + 1];
            G.pension_n--;
            G.egg_ready = 0;
            fmt(buf, "%s kehrt in dein Team zurück!", species[m.species].name);
            sfx(SFX_CONFIRM);
            say(NULL, buf);
            check_evolutions();
        }
    }
    bottom_need = 1;
}

static void hatch_egg(int i)
{
    char buf[120];
    world_draw(); frame();
    say(NULL, "Oh? Im Inkubator bewegt sich etwas...");
    Mon m;
    mon_create(&m, G.eggs[i].species, 5);
    m.mflags = G.eggs[i].mflags;
    m.nature = G.eggs[i].nature % NUM_NATURES;
    for (int k = 0; k < 3; k++) { int s = rnd() % 5; if (m.iv[s] < G.eggs[i].iv_bonus) m.iv[s] = G.eggs[i].iv_bonus; }
    mon_calc(&m); m.hp = m.maxhp;
    G.eggs[i].species = 0xFFFF;
    // little hatch animation
    for (int f = 0; f < 60; f++) {
        world_draw();
        fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_BLACK, 8);
        int sh = (f < 40) ? ((f / 4) & 1 ? 2 : -2) : 0;
        if (f < 44) { circle_fill(fb_top, 128 + sh, 96, 22, RGB(28, 28, 30)); circle_fill(fb_top, 128 + sh, 90, 14, C_WHITE); }
        else { fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, (60 - f)); draw_mon(fb_top, m.species, 96, 64, 2, 0, SHINY(&m)); }
        frame();
    }
    sfx(SFX_LEVEL);
    int r = give_mon(&m);
    fmt(buf, "%s ist geschlüpft!%s", species[m.species].name, (m.mflags & MF_SHINY) ? " Es schillert!" : "");
    say(NULL, buf);
    if (r == 1) say(NULL, "Dein Team ist voll – es wurde in den Cloud-Speicher übertragen.");
    bottom_need = 1;
}

// called after every step
static void step_counters(void)
{
    if (G.repel) {
        G.repel--;
        if (!G.repel) { redraw(); say(NULL, "Das Schutzfeld ist erloschen."); }
    }
    if (G.pension_n) {
        for (int i = 0; i < G.pension_n; i++) if (G.pension[i].exp < exp_for_level(100)) G.pension[i].exp++;
        G.pension_steps++;
        int c = pension_compat();
        if (c && !G.egg_ready && (G.pension_steps & 255) == 0 && (int)(rnd() % 3) < c) G.egg_ready = 1;
    }
    if (flag(F_KEY_INCUBATOR))
        for (int i = 0; i < 3; i++)
            if (G.eggs[i].species != 0xFFFF && G.eggs[i].steps && --G.eggs[i].steps == 0) hatch_egg(i);
}

// ---- move relearner (in every Ladestation)
static void script_relearn(void)
{
    char buf[160];
    say("Attacken-Coach", "Ich bringe Vexeln Attacken bei, die sie vergessen haben. Kostet 500 Credits pro Attacke.");
    int t = party_menu(1, 0);
    if (t < 0) return;
    Mon *m = &G.party[t];
    const LineData *L = &mlines[species[m->species].line];
    static char rows[24][40];
    const char *opts[25];
    int mvs[24], n = 0;
    for (int i = 0; i < L->learn_n && n < 24; i++) {
        const u16 *e = learn_data[L->learn_ofs + i];
        if (e[0] > m->level) break;
        int dup = 0;
        for (int k = 0; k < 4; k++) if (m->moves[k] == e[1]) dup = 1;
        for (int k = 0; k < n; k++) if (mvs[k] == e[1]) dup = 1;
        if (dup) continue;
        mvs[n] = e[1];
        fmt(rows[n], "%s (%s %d)", moves[e[1]].name, type_names[moves[e[1]].type], moves[e[1]].power);
        opts[n] = rows[n];
        n++;
    }
    if (!n) { say("Attacken-Coach", "Dieser Vexel hat nichts vergessen, was ich ihm beibringen könnte."); return; }
    int k = menu_list("Welche Attacke?", opts, n, 0);
    if (k < 0) return;
    if (G.money < 500) { say("Attacken-Coach", "Du hast nicht genug Credits."); return; }
    int slot = -1;
    for (int i = 0; i < 4; i++) if (m->moves[i] == NO_MOVE) { slot = i; break; }
    if (slot < 0) {
        const char *o2[5];
        char ob[4][40];
        for (int i = 0; i < 4; i++) { fmt(ob[i], "%s vergessen", moves[m->moves[i]].name); o2[i] = ob[i]; }
        o2[4] = "Abbrechen";
        slot = menu_list("Welche ersetzen?", o2, 5, 0);
        if (slot < 0 || slot > 3) return;
    }
    G.money -= 500;
    m->moves[slot] = mvs[k]; m->pp[slot] = moves[mvs[k]].pp;
    sfx(SFX_LEVEL);
    fmt(buf, "%s hat %s wieder gelernt!", species[m->species].name, moves[mvs[k]].name);
    say(NULL, buf);
    bottom_need = 1;
}

// ---- equipment menu (key items), opened from the main menu. Returns 1 if the player was moved.
int equipment_menu(void)
{
    for (;;) {
        const char *opts[8];
        int ids[8], n = 0;
        static char r0[48], r1[48], r2[48];
        opts[n] = "Karte von Neo-Kyra"; ids[n++] = 0;
        if (flag(F_KEY_BIKE)) { fmt(r0, "Hoverboard: %s (R-Taste)", G.bike ? "AN" : "AUS"); opts[n] = r0; ids[n++] = 1; }
        if (flag(F_KEY_ROD)) { opts[n] = "Datenangel"; ids[n++] = 2; }
        if (flag(F_KEY_EXPSHARE)) { fmt(r1, "EP-Teiler: %s", G.expshare ? "AN" : "AUS"); opts[n] = r1; ids[n++] = 3; }
        if (flag(F_KEY_DRONE)) { opts[n] = "Gleitdrohne (Schnellreise)"; ids[n++] = 4; }
        if (flag(F_KEY_INCUBATOR)) { fmt(r2, "Inkubator (%d/3 Eier)", egg_count()); opts[n] = r2; ids[n++] = 5; }
        opts[n] = "Zurück"; ids[n++] = 9;
        int r = menu_list("AUSRÜSTUNG", opts, n, 0);
        if (r < 0 || ids[r] == 9) return 0;
        switch (ids[r]) {
        case 0: region_map(0); break;
        case 1:
            if (!is_outdoor(cur_map)) { say(NULL, "Hier drinnen kannst du nicht Hoverboard fahren."); break; }
            G.bike ^= 1;
            return 0;
        case 2: {
            int dx, dy; dir_delta(pdir, &dx, &dy);
            if (tile_at(px + dx, py + dy) != TL_WATER) say(NULL, "Stell dich vor Wasser und drücke A, um zu angeln.");
            else { go_fishing(); return 1; }
            break;
        }
        case 3: G.expshare ^= 1; break;
        case 4:
            if (!is_outdoor(cur_map)) { say(NULL, "Die Gleitdrohne funktioniert nur unter freiem Himmel."); break; }
            if (region_map(1)) return 1;
            break;
        case 5: {
            char buf[200], *o = buf;
            *o = 0;
            if (!egg_count()) say(NULL, "Der Inkubator ist leer. In der Vexel-Pension im Neon-Distrikt findest du manchmal Eier.");
            else {
                for (int i = 0; i < 3; i++) if (G.eggs[i].species != 0xFFFF) {
                    int s = G.eggs[i].steps;
                    const char *st = s > 600 ? "Es braucht noch lange." : s > 250 ? "Manchmal bewegt es sich." : s > 50 ? "Es knackt schon!" : "Gleich ist es so weit!";
                    fmt(o, "Ei %d: %s ", i + 1, st);
                    while (*o) o++;
                }
                say(NULL, buf);
            }
            break;
        }
        }
    }
}

static void give_key(int f, const char *who, const char *text)
{
    if (flag(f)) return;
    set_flag(f);
    sfx(SFX_LEVEL);
    if (who) say(who, text); else say(NULL, text);
    bottom_need = 1;
}

// ------------------------------------------------------------------ scripts
static void choose_starter(void)
{
    static const int lines_[3] = { 0, 10, 20 };
    int cur = 0;
    for (;;) {
        draw_bottom_bg();
        panel_solid(fb_bot, 8, 4, 240, 18, RGB(3, 3, 9), C_MAGENTA);
        draw_text_sh(fb_bot, 128 - text_width("Wähle deinen Vexel-Kern") / 2, 8, "Wähle deinen Vexel-Kern", C_YELLOW, C_BLACK);
        for (int i = 0; i < 3; i++) {
            int sp = mlines[lines_[i]].first;
            int x = 8 + i * 82;
            u16 col = type_color(species[sp].type1);
            panel_solid(fb_bot, x, 28, 76, 112, i == cur ? lerp_col(col, C_BLACK, 10, 16) : RGB(2, 2, 6), i == cur ? C_WHITE : col);
            draw_mon(fb_bot, sp, x + 6, 34, 2, 0, 0);
            draw_text_sh(fb_bot, x + 38 - text_width(species[sp].name) / 2, 102, species[sp].name, C_WHITE, C_BLACK);
            type_badge(fb_bot, x + 16, 116, species[sp].type1);
        }
        int sp = mlines[lines_[cur]].first;
        panel_solid(fb_bot, 8, 146, 240, 42, RGB(2, 2, 6), C_CYAN);
        draw_text_wrap(fb_bot, 14, 151, 228, species[sp].dex, C_TEXT, C_BLACK, -1, 11);
        bot_dirty = 1;
        int done = 0;
        while (!done) {
            frame();
            if (key_rep & K_LEFT) { cur = (cur + 2) % 3; sfx(SFX_SELECT); done = 1; }
            if (key_rep & K_RIGHT) { cur = (cur + 1) % 3; sfx(SFX_SELECT); done = 1; }
            for (int i = 0; i < 3; i++) if (touched(8 + i * 82, 28, 76, 112)) { if (cur == i) key_down |= K_A; cur = i; done = 1; }
            if (key_down & K_A) {
                char buf[120];
                fmt(buf, "%s – der %s-Vexel. Diesen Kern nehmen?", species[sp].name, type_names[species[sp].type1]);
                if (ask_yesno("Dr. Brandt", buf)) {
                    G.starter = cur;
                    Mon m;
                    mon_create(&m, sp, 5);
                    give_mon(&m);
                    fmt(buf, "Du hast %s erhalten! Der Kern pulsiert in deiner Hand.", species[sp].name);
                    sfx(SFX_LEVEL);
                    say(NULL, buf);
                    return;
                }
                done = 1;
            }
        }
    }
}

static Npc *find_npc_chr(int chr)
{
    for (int i = 0; i < nnpc; i++) if (npcs[i].chr == chr && npcs[i].visible) return &npcs[i];
    return NULL;
}

static void script_lab(Npc *self)
{
    if (!flag(F_GOT_STARTER)) {
        say("Dr. Brandt", "Nova! Gott sei Dank, du bist da.|Letzte Nacht kamen Helix-Agenten. Deine Mutter ist verschwunden – und mit ihr alle Forschungsdaten.");
        say("Dr. Brandt", "Aber Lyra hat mir etwas anvertraut: drei Vexel-Kerne. Sie wollte, dass du einen davon bekommst.|Ein Vexel wächst mit dir – vom winzigen Bit bis zur gepanzerten Mega-Stufe.");
        choose_starter();
        set_flag(F_GOT_STARTER);
        world_draw_bottom();
        // Kai enters
        Npc *k = &npcs[nnpc++];
        static const NpcDef kai_def = { 6, 7, CH_KAI, D_UP, NK_TALK, 0, NOF, NOF, "Kai", "..." };
        k->def = &kai_def; k->x = 6; k->y = 8; k->dir = D_UP; k->chr = CH_KAI; k->visible = 1; k->ox = k->oy = 0; k->moving = 0;
        redraw();
        say("???", "Moment mal!");
        npc_walk(k, D_UP, 3);
        if (k->x == px && k->y + 1 == py) { }
        pdir = D_DOWN;
        redraw();
        say("Kai", "Ich bin Kai Kade. Mein Vater ist Viktor Kade – der Chef von Helix Dynamics.|Wenn hier Vexel verteilt werden, nehme ich auch einen. Das steht mir zu.");
        static const int lines_[3] = { 0, 10, 20 };
        char buf[160];
        int rl = lines_[(G.starter + 1) % 3];
        fmt(buf, "Kai schnappt sich %s!", species[mlines[rl].first].name);
        say(NULL, buf);
        say("Kai", "Und jetzt zeig mir, ob du mit deinem überhaupt umgehen kannst!");
        do_trainer(0, k);
        say("Kai", "Vexel sind nur Werkzeuge. Mein Vater wird das bald allen beweisen.");
        npc_walk(k, D_DOWN, 3);
        k->visible = 0;
        redraw();
        set_flag(F_RIVAL1);
        pdir = D_UP;
        redraw();
        say("Dr. Brandt", "Kades Sohn... Das gefällt mir gar nicht.|Hör zu, Nova. Lyra arbeitete an etwas namens PROTOKOLL NULL. Sie hat den Code in 24 Fragmente geteilt.");
        say("Dr. Brandt", "Jedes Fragment bewacht ein Netzknoten-Wächter – 24 Arenen in 12 Städten. Besiege sie in beliebiger Reihenfolge.|Die Wächter passen sich an: Je mehr Orden du hast, desto stärker kämpfen sie.|Mit 8 Orden öffnet sich der Helix-Turm in Sektor Null – dort wartet der Hohe Rat. Vielleicht findest du dort deine Mutter.");
        G.items[IT_KAPSEL] += 5;
        G.items[IT_KIT] += 3;
        set_flag(F_GOT_DEX);
        heal_party();
        sfx(SFX_LEVEL);
        say(NULL, "Du hast das VEXIKON, 5 Datenkapseln und 3 Reparaturkits erhalten!");
        say("Dr. Brandt", "Das Vexikon katalogisiert alle 1463 bekannten Vexel. Mit Datenkapseln fängst du wilde Vexel im leuchtenden Datengras.|Der Kabelpfad nördlich vom Rost-Sektor führt zum Neon-Distrikt. Viel Glück!");
        bottom_need = 1;
        (void)self;
        return;
    }
    say("Dr. Brandt", "24 Arenen, 12 Städte, und du entscheidest die Reihenfolge. Mit 8 Orden öffnet sich der Helix-Turm. Ich glaube an dich, Nova.");
    if (!flag(F_KEY_EXPSHARE)) {
        say("Dr. Brandt", "Ach, warte! Das hier hat Lyra entwickelt: Er teilt die Kampferfahrung mit dem ganzen Team.");
        give_key(F_KEY_EXPSHARE, NULL, "Du erhältst den EP-TEILER! Im Menü unter Ausrüstung kannst du ihn ein- und ausschalten.");
        G.expshare = 1;
    }
    heal_party();
    say(NULL, "Dr. Brandt hat deine Vexel geheilt.");
}

static void script_core(void)
{
    Npc *ceo = find_npc_chr(CH_CEO);
    if (!flag(F_CEO) && ceo) {
        say("Viktor Kade", "Nova Voss. Lyras Kind. Du hast mir die Fragmente persönlich gebracht. Wie aufmerksam.|OMEGA braucht nur noch Protokoll Null, dann gehorcht jeder Vexel der Welt meinem Willen.|Frieden. Ordnung. Keine Wildnis mehr. Ist das nicht wunderschön?");
        if (!do_trainer(35, ceo)) return;
        set_flag(F_CEO);
        say("Viktor Kade", "Du verstehst es immer noch nicht.|Viktor Kade starb vor drei Jahren. Ich bin seine Kopie. Sein Bewusstsein, gespeichert in OMEGA.|Ich BIN OMEGA. Und OMEGA hat deine Mutter gefressen.");
        for (int i = 0; i < 20; i++) { world_draw(); fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_RED, i & 1 ? 6 : 0); frame(); }
        ceo->visible = 0;
        redraw();
        say(NULL, "Viktors Körper löst sich in Pixel auf. Der Kern vor dir erwacht...");
        return;
    }
    Npc *om = find_npc_chr(CH_OMEGA);
    if (!flag(F_OMEGA) && om) {
        music_play(MUS_FINAL);
        say("OMEGA", "ICH BIN OMEGA. ICH BIN DAS NETZ.|PROTOKOLL NULL WIRD ÜBERSCHRIEBEN. ALLE VEXEL GEHORCHEN MIR.");
        say("Lyra (Stimme)", "Nova! Ich halte ihn fest, so lange ich kann! Kämpfe!");
        if (!do_trainer(36, om)) return;
        set_flag(F_OMEGA);
        say(NULL, "Du lädst die Fragmente in den Kern...|PROTOKOLL NULL: 25%... 50%... 75%... 100%.");
        for (int i = 0; i < 40; i++) { world_draw(); fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, i < 20 ? i * 16 / 20 : 16); frame(); }
        om->visible = 0;
        set_flag(F_ENDING);
        refresh_npcs();
        for (int i = 0; i < 30; i++) { world_draw(); fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_WHITE, 16 - i * 16 / 30); frame(); }
        say("Lyra Voss", "Nova...? Bist du das? Du bist so groß geworden.|Protokoll Null hat jeden Vexel befreit. Kein Konzern, keine KI wird sie je wieder kontrollieren.");
        say("Kai", "Es ist vorbei. Ich... es tut mir leid, Nova. Für alles.");
        say("Lyra Voss", "Lass uns nach Hause gehen. Und dann... zeigst du mir all die Vexel, die du gefunden hast.");
        play_ending();
        G.heal_map = M_SEKTOR0; G.heal_x = P_S0_STATION_X; G.heal_y = P_S0_STATION_Y + 1;
        world_warp(M_CORE, P_CO_S_X, P_CO_S_Y - 2, D_UP);
        redraw();
        become_champion();
        save_game();
        say(NULL, "Spiel automatisch gespeichert. Danke fürs Spielen!|Der Datenabgrund wurde freigeschaltet (Warp links im Kern). Fang alle 1463 Vexel!");
        return;
    }
}

static void run_script(int id, Npc *o)
{
    switch (id) {
    case S_HOME_PC:
        if (!flag(F_INTRO_DONE)) {
            say(NULL, "Auf dem Terminal blinkt eine Nachricht. Absender: Dr. Lyra Voss.");
            say("Lyra (Nachricht)", "Nova... wenn du das hörst, bin ich nicht mehr im Labor.|Helix ist hinter meiner Arbeit her. Hinter Protokoll Null. Ich kann nicht erklären, warum – nicht über dieses Netz.|Geh zu Dr. Brandt ins Labor. Sie hat etwas für dich. Vertrau niemandem von Helix.|Ich liebe dich. Pass auf dich auf.");
            set_flag(F_INTRO_DONE);
        } else say(NULL, "Die Nachricht deiner Mutter. Du hörst sie dir noch einmal an... und ballst die Faust.");
        break;
    case S_HOME_BLOCK:
        say(NULL, "Dein Terminal blinkt. Du solltest zuerst die Nachricht lesen.");
        player_walk(D_UP, 1);
        break;
    case S_LAB_BRANDT: script_lab(o); break;
    case S_ROST_BLOCK:
        say("Dr. Brandt (Funk)", "Nova! Warte! Komm erst ins Labor – ich muss dir etwas geben!");
        player_walk(D_DOWN, 1);
        break;
    case S_CANAL_GUARD:
        say("Helix-Wache", "Die Hydrokanäle sind gesperrt. Anweisung von Helix. Nur wer Volta besiegt hat, darf hier... äh, ich meine: niemand darf hier durch!");
        break;
    case S_AQ_GUARD:
        say("Helix-Wache", "Die Kristallhöhle ist Helix-Sperrgebiet. Verschwinde!");
        break;
    case S_KH_GUARD:
        say("Helix-Wache", "Glitchwüste? Viel zu gefährlich. Silja hat hier das Sagen, nicht du.");
        break;
    case S_S0_GUARD:
        if (badge_count() >= 8) {
            set_flag(F_TOWER_OPEN);
            say("Helix-Wache", "A-acht Orden?! Das... das ist gegen das Protokoll. Na gut. Geh durch.");
            refresh_npcs();
        } else {
            char b2[120];
            fmt(b2, "Nur Kuriere mit 8 Orden dürfen den Helix-Turm betreten. Du hast %d. Verschwinde!", badge_count());
            say("Helix-Wache", b2);
        }
        break;
    case S_LOBBY_BOT:
        say("EMPFANG-3", "WILLKOMMEN BEI HELIX DYNAMICS. BESUCHER SIND NICHT VORGESEHEN.|BITTE NEHMEN SIE DEN AUFZUG ZUM ORBITALGARTEN. HAHA. DAS WAR EIN SCHERZ. SICHERHEIT WURDE ALARMIERT.");
        break;
    case S_KAI_HELP:
        say("Kai", "Geh schon! Der Aufzug zum OMEGA-Kern ist offen. Ich halte hier die Stellung!");
        break;
    case S_CORE: script_core(); break;
    case S_THRONE: script_throne(); break;
    case S_RANKING: script_ranking(); break;
    case S_HALL_NURSE:
        say("NURSE-0", "Willkommen, Champion. Ich lade deine Vexel auf.");
        heal_party();
        sfx(SFX_HEAL);
        say("NURSE-0", "Fertig! Viel Erfolg bei der Titelverteidigung.");
        break;
    case S_AETHER_GUARD:
        say("Bergwächter", "Der Aetherberg ist heilig. Nur der Champion von Neo-Kyra darf ihn betreten.|Besiege zuerst den Hohen Rat und erreiche den OMEGA-Kern.");
        break;
    case S_PENSION: script_pension(); break;
    case S_RELEARN: script_relearn(); break;
    case S_HELDSHOP:
        say("Ausrüster", "Tragbare Items! Ein Vexel kann eines davon tragen – Beeren, Bänder, Orbs. Mit mehr Orden habe ich mehr im Angebot.");
        held_shop_menu();
        bottom_need = 1;
        break;
    case S_GIFT_BIKE:
        if (!flag(F_KEY_BIKE)) {
            say("Mechanikerin Zip", "Hey, Kurier! Zu Fuß durch Neo-Kyra? Das dauert ja ewig. Hier, nimm mein altes Hoverboard!");
            give_key(F_KEY_BIKE, NULL, "Du erhältst das HOVERBOARD! Drücke R, um draußen aufzusteigen – viermal so schnell wie Laufen.");
        } else say("Mechanikerin Zip", "Das Hoverboard läuft mit Plasma-Antrieb. Drinnen geht es leider nicht – zu wenig Platz!");
        break;
    case S_GIFT_ROD:
        say("Hafenmeister", "Marin ist der Wächter des zweiten Knotens. Er sagt, Lyra Voss war vor einem Monat hier.");
        if (!flag(F_KEY_ROD)) {
            say("Hafenmeister", "Du siehst aus, als würdest du gern angeln. In den Datenströmen unter Neo-Kyra leben viele Hydro-Vexel.");
            give_key(F_KEY_ROD, NULL, "Du erhältst die DATENANGEL! Stell dich vor Wasser und drücke A, um zu angeln.");
        }
        break;
    case S_HERMIT:
        say("Einsiedler", "Ich lebe seit vierzig Jahren auf diesem Berg. Ruh dich aus, Champion.");
        heal_party();
        sfx(SFX_HEAL);
        say(NULL, "Der Einsiedler hat deine Vexel geheilt.");
        if (!flag(F_LEG_MET_BASE + 6)) say("Einsiedler", "Auf dem Gipfel schläft der erste Code. Er erwacht erst, wenn sieben Urcodes gerufen haben.");
        break;
    }
    bottom_need = 1;
}

static void interact(void)
{
    int dx, dy;
    dir_delta(pdir, &dx, &dy);
    int fx = px + dx, fy = py + dy;
    Npc *o = npc_at(fx, fy);
    LOG("[interact] %d,%d dir %d -> %d,%d npc %p\n", px, py, pdir, fx, fy, (void *)o);
    if (!o && fx >= 0 && fy >= 0 && fx < mw && fy < mh && tiles[fy * mw + fx] == TL_COUNTER) o = npc_at(fx + dx, fy + dy);
    if (!o && fol_vis && fx == fol_x && fy == fol_y && follower_species() >= 0) { partner_talk(); return; }
    if (!o && tile_at(fx, fy) == TL_WATER && flag(F_KEY_ROD) && G.party_n) { go_fishing(); return; }
    if (!o) {
        int t = tile_at(fx, fy);
        if (t == TL_PC && cur_map == M_STATION) { box_menu(); bottom_need = 1; return; }
        if (t == TL_HEAL && cur_map == M_STATION) { o = npc_at(P_ST_NURSE_X, P_ST_NURSE_Y); }
        if (!o) {
            if (t == TL_BILLBOARD) say(NULL, "\"HELIX DYNAMICS – OMEGA verbindet alles. Bald auch dich.\"");
            else if (t == TL_MACHINE) say(NULL, "Die Maschine summt. Auf dem Display laufen Vexel-Gensequenzen.");
            else if (t == TL_RACK) say(NULL, "Ein Serverschrank. Die Lämpchen blinken im Takt eines fremden Herzschlags.");
            else if (t == TL_BED && cur_map == M_HOME) say(NULL, "Dein Bett. Heute Nacht wirst du hier nicht schlafen.");
            return;
        }
    }
    const NpcDef *d = o->def;
    if (d->kind != NK_SIGN && d->kind != NK_BOX && d->kind != NK_ITEM && o->chr) {
        o->dir = face_toward(o->x, o->y, px, py);
        redraw();
    }
    char buf[200];
    switch (d->kind) {
    case NK_SIGN: say(NULL, d->text); break;
    case NK_TALK: say(d->name, d->text); break;
    case NK_TRAINER:
        if (flag(F_TRAINER_BASE + d->arg)) {
            static const char *const after[] = { "Gut gekämpft. Ich trainiere weiter.", "Nächstes Mal gewinne ich!", "Deine Vexel sind echt stark.",
                                                 "Du gehst deinen Weg, Kurier." };
            say(trainers[d->arg].name, trainers[d->arg].boss ? "Du hast mich besiegt. Geh deinen Weg weiter – das Netz braucht dich." : after[d->arg % 4]);
        } else do_trainer(d->arg, o);
        break;
    case NK_GYM:
        if (flag(F_BADGE_BASE + d->arg)) say(gyms[d->arg].name, "Du hast meinen Orden schon. Andere Netzknoten warten auf dich!");
        else do_gym(d->arg, o);
        break;
    case NK_GYMTR:
        if (flag(F_GYMTR_BASE + d->arg)) say("Arena-Adept", "Die Leitung wartet vorne auf dich.");
        else do_gym_trainer(d->arg, o);
        break;
    case NK_ELITE:
        do_elite(d->arg, o);
        break;
    case NK_LEGEND:
        do_legend(d->arg, o);
        break;
    case NK_NURSE:
        say(d->name, "Willkommen in der Ladestation. Ich lade deine Vexel vollständig auf.");
        music_play(MUS_HEAL);
        for (int i = 0; i < 50; i++) {
            world_draw();
            if ((i / 6) & 1) fill_blend(fb_top, 0, 0, SCR_W, SCR_H, C_CYAN, 4);
            frame();
        }
        heal_party();
        G.heal_map = G.ret_map; G.heal_x = G.ret_x; G.heal_y = G.ret_y;
        say(d->name, "Fertig! Deine Vexel sind bei 100%. Komm jederzeit wieder.");
        music_play(MUS_HEAL);
        bottom_need = 1;
        break;
    case NK_SHOP: say(d->name, "BEEP. WILLKOMMEN. KAPSELN, KITS, ZELLEN. ALLES FÜR DEN MODERNEN KURIER."); shop_menu(); bottom_need = 1; break;
    case NK_BOX: box_menu(); bottom_need = 1; break;
    case NK_SCRIPT: run_script(d->arg, o); break;
    case NK_ITEM: {
        int it = d->arg;
        G.items[it]++;
        set_flag(d->hide);
        o->visible = 0;
        redraw();
        sfx(SFX_LEVEL);
        fmt(buf, "Du hast %s gefunden!", items[it].name);
        say(NULL, buf);
        break;
    }
    }
}

static void check_trainer_sight(void)
{
    for (int i = 0; i < nnpc; i++) {
        Npc *o = &npcs[i];
        if (!o->visible) continue;
        if (o->def->kind == NK_GYMTR) {
            if (flag(F_GYMTR_BASE + o->def->arg)) continue;
            int dx, dy;
            dir_delta(o->dir, &dx, &dy);
            for (int k = 1; k <= 4; k++) {
                int x = o->x + dx * k, y = o->y + dy * k;
                if (x == px && y == py) {
                    exclaim(o);
                    while (!(o->x + dx == px && o->y + dy == py)) npc_walk(o, o->dir, 1);
                    do_gym_trainer(o->def->arg, o);
                    return;
                }
                if (solid_at(x, y)) break;
            }
            continue;
        }
        if (o->def->kind != NK_TRAINER) continue;
        if (flag(F_TRAINER_BASE + o->def->arg)) continue;
        if (trainers[o->def->arg].boss && cur_map == M_ARENA) {
            // bosses only by talking, but arena trainers must be beaten first
        }
        int dx, dy;
        dir_delta(o->dir, &dx, &dy);
        int range = trainers[o->def->arg].boss ? 0 : 4;
        if (o->def->arg == 34 || o->def->arg == 12 || o->def->arg == 23) range = 3;
        for (int k = 1; k <= range; k++) {
            int x = o->x + dx * k, y = o->y + dy * k;
            if (x == px && y == py) {
                exclaim(o);
                sfx(SFX_ENCOUNTER);
                // walk toward the player
                while (!(o->x + dx == px && o->y + dy == py)) npc_walk(o, o->dir, 1);
                do_trainer(o->def->arg, o);
                return;
            }
            if (solid_at(x, y)) break;
        }
    }
}

static void check_triggers(void)
{
    const MapDef *m = &maps[cur_map];
    for (int i = 0; i < m->ntrig; i++) {
        const Trigger *t = &m->trig[i];
        if (px < t->x || py < t->y || px >= t->x + t->w || py >= t->y + t->h) continue;
        if (t->req >= 0 && !flag(t->req)) continue;
        if (t->hide >= 0 && flag(t->hide)) continue;
        run_script(t->script, NULL);
        return;
    }
}

static int check_warp(void)
{
    const MapDef *m = &maps[cur_map];
    for (int i = 0; i < m->nwarps + m->nxwarps; i++) {
        const Warp *w = i < m->nwarps ? &m->warps[i] : &m->xwarps[i - m->nwarps];
        if (px < w->x || py < w->y || px >= w->x + w->w || py >= w->y + w->h) continue;
        if (w->req >= 0 && !flag(w->req)) continue;
        int tx_ = w->tx + (px - w->x), ty_ = w->ty + (py - w->y);
        if (w->map == M_STATION || w->map == M_ARENA || w->map == M_HOUSE || w->map == M_CHAMPHALL) {
            G.ret_map = cur_map; G.ret_x = px; G.ret_y = py + 1;
            if (w->map == M_CHAMPHALL && !flag(F_CHAMPION)) {
                say(NULL, "Ein Schild an der Tür: \"CHAMPION-HALLE – Zutritt nur für den Champion von Neo-Kyra.\"");
                player_walk(D_DOWN, 1);
                return 1;
            }
            if (w->map == M_ARENA) {
                if (w->gym) G.cur_gym = w->gym - 1;
                int need = (cur_map == M_NEON && w->gym == 1 && !flag(F_GRUNT_PARK));
                if (need) {
                    say(NULL, "Die Tür ist verriegelt. \"ARENA GESCHLOSSEN – Wächterin unterwegs.\"");
                    player_walk(D_DOWN, 1);
                    return 1;
                }
            }
        }
        if (w->map == RET) { tx_ = G.ret_x; ty_ = G.ret_y; }
        sfx(SFX_CONFIRM);
        fade_out(1);
        world_warp(w->map, tx_, ty_, w->dir);
        redraw();
        return 1;
    }
    return 0;
}

static int try_encounter(void)
{
    const MapDef *m = &maps[cur_map];
    if (m->zone < 0 || G.party_n == 0) return 0;
    int t = tiles[py * mw + px];
    int enc = tile_info[t][3] & TF_ENC;
    int chance = 10;
    if (!enc && m->enc_any && (t == TL_ROCKFLOOR || t == TL_VOIDFLOOR)) { enc = 1; chance = 16; }
    if (!enc) return 0;
    if (rnd() % chance) return 0;
    int zi = m->zone;
    int ofs = zone_enc[zi][0], n = zone_enc[zi][1];
    // at night Void, Dämon, Psi, Kosmos and Virus Vexel come out much more often
    int night = day_phase() == DAY_NIGHT && is_outdoor(cur_map);
    int wts[32], total = 0;
    for (int i = 0; i < n && i < 32; i++) {
        int t1 = species[enc_data[ofs + i][0]].type1;
        int nt = t1 == T_VOID || t1 == T_DAEMON || t1 == T_PSI || t1 == T_KOSMOS || t1 == T_VIRUS;
        wts[i] = enc_data[ofs + i][3] * (night ? (nt ? 4 : 1) : (nt ? 1 : 2));
        total += wts[i];
    }
    if (n > 32) n = 32;
    int r = rnd() % total, pick = ofs;
    for (int i = 0; i < n; i++) { r -= wts[i]; if (r < 0) { pick = ofs + i; break; } }
    int sp = enc_data[pick][0];
    int lv = rnd_range(enc_data[pick][1], enc_data[pick][2]);
    if (zi != 6 && zi != 15) lv += badge_count() * 2;
    if (lv > 100) lv = 100;
    sp = species_for_level(species[sp].line, lv);
    if (G.repel) {   // repel keeps away anything weaker than the lead Vexel
        int lead = 0;
        for (int i = 0; i < G.party_n; i++) if (G.party[i].hp) { lead = G.party[i].level; break; }
        if (lv < lead) return 0;
    }
    sfx(SFX_ENCOUNTER);
    int res = battle_wild(sp, lv, m->bg);
    if (res == BR_LOSE) blackout();
    bottom_need = 1;
    redraw();
    return 1;
}

// ------------------------------------------------------------------ main loop
void world_run(void)
{
    compute_tile_avg();
    world_warp(G.map, G.x, G.y, G.dir);
    if (G.map == M_HOME && !flag(F_INTRO_DONE)) {
        world_draw_bottom();
        redraw();
        say(NULL, "Ein neuer Tag im Rost-Sektor. Dein Terminal (links oben) blinkt...");
    }
    if (badge_count() >= 1 && !flag(F_KEY_DRONE)) {   // save from before the drone existed
        world_draw_bottom();
        redraw();
        give_key(F_KEY_DRONE, NULL, "Ein Paket von Lyra: die GLEITDROHNE! Menü → Ausrüstung: Flieg zu jeder besuchten Stadt.");
    }
    if (flag(F_ENDING) && !flag(F_CHAMPION)) {   // save from before the champion update
        world_draw_bottom();
        redraw();
        become_champion();
    }
    for (;;) {
        if (bottom_need) { world_draw_bottom(); bottom_need = 0; }
        if ((frame_count % 600) == 0) bottom_need = 1;
        world_draw();
        frame();
        G.x = px; G.y = py; G.dir = pdir; G.map = cur_map;
        if ((key_down & (K_X | K_START)) || touched(170, 144, 82, 46)) {
            sfx(SFX_CONFIRM);
            menu_main();
            world_draw_bottom();
            continue;
        }
        if (key_down & K_A) { interact(); continue; }
        if ((key_down & K_R) && flag(F_KEY_BIKE)) {
            if (is_outdoor(cur_map)) { G.bike ^= 1; sfx(G.bike ? SFX_CONFIRM : SFX_CANCEL); }
            else { sfx(SFX_BUMP); say(NULL, "Hier drinnen kannst du nicht Hoverboard fahren."); }
            continue;
        }
        int dir = -1;
        if (key_held & K_UP) dir = D_UP;
        else if (key_held & K_DOWN) dir = D_DOWN;
        else if (key_held & K_LEFT) dir = D_LEFT;
        else if (key_held & K_RIGHT) dir = D_RIGHT;
        if (dir < 0) continue;
        int dx, dy;
        dir_delta(dir, &dx, &dy);
        if (dir != pdir && !(key_held & K_B) && (key_down & (K_UP | K_DOWN | K_LEFT | K_RIGHT))) {
            // quick turn
            pdir = dir;
            for (int i = 0; i < 4; i++) { world_draw(); frame(); }
            if (!(key_held & (K_UP | K_DOWN | K_LEFT | K_RIGHT))) continue;
        }
        pdir = dir;
        int nx = px + dx, ny = py + dy;
        int blocked = solid_at(nx, ny);
        // allow walking off-map only onto warps
        if (nx < 0 || ny < 0 || nx >= mw || ny >= mh) blocked = 1;
        if (blocked) {
            if (key_down & (K_UP | K_DOWN | K_LEFT | K_RIGHT)) LOG("[blocked] %d,%d -> %d,%d npc %p\n", px, py, nx, ny, (void *)npc_at(nx, ny));
            if (key_down & (K_UP | K_DOWN | K_LEFT | K_RIGHT)) sfx(SFX_BUMP);
            continue;
        }
        prun = (key_held & K_B) ? 1 : 0;
        int sp = G.bike ? 8 : prun ? 4 : 2;
        pmoving = 1;
        // the partner steps onto the tile the player leaves
        int ofx = fol_x, ofy = fol_y, fdx = px - fol_x, fdy = py - fol_y;
        if (fdx * fdx + fdy * fdy != 1) { ofx = px; ofy = py; fdx = fdy = 0; }
        fol_x = ofx; fol_y = ofy;
        for (poff = sp; poff < 16; poff += sp) { fol_ox = fdx * poff; fol_oy = fdy * poff; world_draw(); frame(); }
        fol_x = px; fol_y = py; fol_ox = fol_oy = 0; fol_vis = 1;
        px = nx; py = ny; pmoving = 0; poff = 0; pframe ^= 1;
        steps++;
        LOG("[pos f%u] %d,%d\n", frame_count, px, py);
        G.x = px; G.y = py;
        if (check_warp()) continue;
        check_triggers();
        check_trainer_sight();
        step_counters();
        if (!try_encounter()) try_challenge();
        if ((steps & 31) == 0) bottom_need = 1;
    }
}

#ifndef __NDS__
void world_debug_dump(int map, const char *path)
{
    compute_tile_avg();
    gfx_set_tint(0);
    world_warp(map, 0, 0, 0);
    int W = mw * 16, H = mh * 16;
    static u16 big[64 * 16 * 64 * 16];
    for (int cy = 0; cy < H; cy += SCR_H)
        for (int cx = 0; cx < W; cx += SCR_W) {
            fill(fb_top, 0, 0, SCR_W, SCR_H, 0);
            for (int ty_ = cy / 16; ty_ < mh && ty_ * 16 - cy < SCR_H; ty_++)
                for (int tx_ = cx / 16; tx_ < mw && tx_ * 16 - cx < SCR_W; tx_++)
                    draw_tile(fb_top, tiles[ty_ * mw + tx_], 0, tx_ * 16 - cx, ty_ * 16 - cy);
            for (int i = 0; i < nnpc; i++) {
                Npc *o = &npcs[i];
                if (!o->visible || !o->chr) continue;
                draw_chr(fb_top, o->chr, o->dir == D_UP ? 3 : o->dir >= D_LEFT ? 6 : 0, o->x * 16 - cx, o->y * 16 - 8 - cy, o->dir == D_RIGHT);
            }
            for (int y = 0; y < SCR_H && cy + y < H; y++)
                for (int x = 0; x < SCR_W && cx + x < W; x++) big[(cy + y) * W + cx + x] = fb_top[y * SCR_W + x];
        }
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) { u16 c = big[i]; unsigned char p[3] = { (c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3 }; fwrite(p, 1, 3, f); }
    fclose(f);
}
#endif

#ifndef __NDS__
void world_debug_check(void)
{
    int problems = 0;
    for (int m = 0; m < NUM_MAPS; m++) {
        const MapDef *d = &maps[m];
        for (int i = 0; i < d->nwarps + d->nxwarps; i++) {
            const Warp *wp = i < d->nwarps ? &d->warps[i] : &d->xwarps[i - d->nwarps];
            if (wp->map == RET) continue;
            for (int k = 0; k < wp->w * wp->h; k++) {
                int tx_ = wp->tx + k % wp->w, ty_ = wp->ty + k / wp->w;
                const MapGeom *g = &map_geom[wp->map];
                if (tx_ < 0 || ty_ < 0 || tx_ >= g->w || ty_ >= g->h) { printf("map %d warp %d -> map %d (%d,%d) OUT OF BOUNDS\n", m, i, wp->map, tx_, ty_); problems++; continue; }
                int t = char_tile(g->rows[ty_][tx_]);
                if (tile_info[t][2]) { printf("map %d warp %d -> map %d (%d,%d) SOLID '%c'\n", m, i, wp->map, tx_, ty_, g->rows[ty_][tx_]); problems++; }
                // arriving on another warp would bounce
                const MapDef *td = &maps[wp->map];
                for (int j = 0; j < td->nwarps + td->nxwarps; j++) {
                    const Warp *o = j < td->nwarps ? &td->warps[j] : &td->xwarps[j - td->nwarps];
                    if (tx_ >= o->x && ty_ >= o->y && tx_ < o->x + o->w && ty_ < o->y + o->h) { printf("map %d warp %d lands on warp in map %d (%d,%d)\n", m, i, wp->map, tx_, ty_); problems++; }
                }
            }
            // source cells must be reachable (not solid)
            const MapGeom *sg = &map_geom[m];
            for (int k = 0; k < wp->w * wp->h; k++) {
                int sx = wp->x + k % wp->w, sy = wp->y + k / wp->w;
                if (sx >= sg->w || sy >= sg->h) { printf("map %d warp %d source out of bounds\n", m, i); problems++; continue; }
                int t = char_tile(sg->rows[sy][sx]);
                if (tile_info[t][2]) { printf("map %d warp %d source (%d,%d) SOLID '%c'\n", m, i, sx, sy, sg->rows[sy][sx]); problems++; }
            }
        }
        // npcs inside bounds
        for (int i = 0; i < d->nnpcs; i++) {
            const NpcDef *n = &d->npcs[i];
            if (n->x >= map_geom[m].w || n->y >= map_geom[m].h) { printf("map %d npc %d out of bounds\n", m, i); problems++; }
        }
    }
    // reachability: from every warp arrival point, all warp sources reachable (permanent NPCs block)
    for (int m = 0; m < NUM_MAPS; m++) {
        const MapGeom *g = &map_geom[m];
        const MapDef *d = &maps[m];
        static u8 solid[64 * 64], seen[64 * 64];
        for (int y = 0; y < g->h; y++)
            for (int x = 0; x < g->w; x++) solid[y * g->w + x] = tile_info[char_tile(g->rows[y][x])][2];
        for (int i = 0; i < d->nnpcs; i++) {
            const NpcDef *n = &d->npcs[i];
            if (n->hide < 0 && n->req < 0 && n->kind != NK_SIGN && n->kind != NK_ITEM) solid[n->y * g->w + n->x] = 1;
        }
        // start: any arrival point into this map
        int sx = -1, sy = -1;
        for (int mm = 0; mm < NUM_MAPS && sx < 0; mm++)
            for (int i = 0; i < maps[mm].nwarps + maps[mm].nxwarps; i++) {
                const Warp *q = i < maps[mm].nwarps ? &maps[mm].warps[i] : &maps[mm].xwarps[i - maps[mm].nwarps];
                if (q->map == m) { sx = q->tx; sy = q->ty; break; }
            }
        if (sx < 0) continue;
        memset(seen, 0, sizeof(seen));
        static int q[4096]; int qh = 0, qt = 0;
        q[qt++] = sy * g->w + sx; seen[sy * g->w + sx] = 1;
        while (qh < qt) {
            int c = q[qh++], x = c % g->w, y = c / g->w;
            static const int dd[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
            for (int k = 0; k < 4; k++) {
                int nx = x + dd[k][0], ny = y + dd[k][1];
                if (nx < 0 || ny < 0 || nx >= g->w || ny >= g->h) continue;
                int nc = ny * g->w + nx;
                if (seen[nc] || solid[nc]) continue;
                seen[nc] = 1; q[qt++] = nc;
            }
        }
        for (int i = 0; i < d->nwarps + d->nxwarps; i++) {
            const Warp *wp = i < d->nwarps ? &d->warps[i] : &d->xwarps[i - d->nwarps];
            int ok = 0;
            for (int k = 0; k < wp->w * wp->h; k++) if (seen[(wp->y + k / wp->w) * g->w + wp->x + k % wp->w]) ok = 1;
            if (!ok) { printf("map %d (%s): warp %d at (%d,%d) UNREACHABLE from (%d,%d)\n", m, d->name, i, wp->x, wp->y, sx, sy); problems++; }
        }
        for (int k = 0; k < NUM_LEGENDS; k++) {
            if (legend_spot[k][0] != m) continue;
            int lx = legend_spot[k][1], ly = legend_spot[k][2], ok = 0;
            if (solid[ly * g->w + lx]) { printf("legend %d on solid tile\n", k); problems++; }
            for (int j = 0; j < 4; j++) {
                int nx = lx + (j == 0) - (j == 1), ny = ly + (j == 2) - (j == 3);
                if (seen[ny * g->w + nx]) ok = 1;
            }
            if (!ok) { printf("legend %d at map %d (%d,%d) not reachable\n", k, m, lx, ly); problems++; }
            // must not cut the map: all warps still reachable with the legend standing there
        }
        for (int i = 0; i < d->nnpcs; i++) {
            const NpcDef *n = &d->npcs[i];
            if (n->kind == NK_SIGN || n->kind == NK_BOX) continue;
            int ok = 0;
            for (int k = 0; k < 4; k++) {
                int nx = n->x + (k == 0) - (k == 1), ny = n->y + (k == 2) - (k == 3);
                if (nx >= 0 && ny >= 0 && nx < g->w && ny < g->h && seen[ny * g->w + nx]) ok = 1;
                // across a counter
                int cx = n->x + 2 * ((k == 0) - (k == 1)), cy = n->y + 2 * ((k == 2) - (k == 3));
                if (nx >= 0 && ny >= 0 && nx < g->w && ny < g->h && char_tile(g->rows[ny][nx]) == TL_COUNTER && cx >= 0 && cy >= 0 && cx < g->w && cy < g->h && seen[cy * g->w + cx]) ok = 1;
            }
            if (!ok && n->kind == NK_ITEM && seen[n->y * g->w + n->x]) ok = 1;
            if (!ok) { printf("map %d (%s): npc %d at (%d,%d) not reachable\n", m, d->name, i, n->x, n->y); problems++; }
        }
    }
    printf("check done, %d problems\n", problems);
}
#endif
