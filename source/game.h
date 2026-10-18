#pragma once
#include "plat.h"
#include "gfx.h"
#include "gen_gfx.h"
#include "gen_mon.h"

// ------------------------------------------------------------------ creatures
typedef struct {
    u16 species;
    u8 level;
    u8 status;      // low nibble: ST_*, high nibble: sleep turns left
    u16 hp;
    u16 maxhp, atk, def, tec, spd;
    u32 exp;
    u16 moves[4];   // NO_MOVE = empty
    u8 pp[4];
    u8 iv[5];
    u8 mflags;      // MF_SHINY
    u8 nature;      // 0..15: +10% stat nature/4, -10% stat nature%4 (equal = neutral)
    u8 held;        // held item (HI_*), 0 = none
} Mon;
#define MF_SHINY 1
#define NUM_NATURES 16
extern const char *const nature_names[NUM_NATURES];
const char *nature_desc(int n, char *buf);

enum { ST_OK, ST_BURN, ST_PARA, ST_POISON, ST_SLEEP, ST_FREEZE, ST_TOXIC };
#define STATUS(m) ((m)->status & 15)
extern const char *const status_short[7];

// held items (one per Vexel)
enum { HI_NONE, HI_NANOREP, HI_DATENBEERE, HI_KLARBEERE, HI_NOTFALLCHIP, HI_KRAFTMODUL, HI_TECHNIKMODUL, HI_UEBERLAST,
       HI_ZIELCHIP, HI_TURBOCHIP, HI_SAUGMODUL, HI_STASISKERN, HI_LERNCHIP, HI_CREDITCHIP, HI_NEBELKAPSEL, HI_KLIMAKERN,
       HI_PANZERWESTE, NUM_HELD };


void mon_calc(Mon *m);
void mon_create(Mon *m, int species, int level);
u32 exp_for_level(int lv);
int mon_nmoves(const Mon *m);
int mon_learn_at(const Mon *m, int level, int *moves_out, int maxn); // moves learned exactly at level
void mon_heal(Mon *m);
#define SHINY(m) (((m)->mflags & MF_SHINY) ? 0x10 : 0)   // draw_mon mode flag

// ------------------------------------------------------------------ items
enum { IT_KAPSEL, IT_SUPERKAPSEL, IT_HYPERKAPSEL, IT_OMEGAKAPSEL, IT_KIT, IT_SUPERKIT, IT_MAXKIT, IT_REBOOT, IT_ANTIVIRUS, IT_ZELLE, IT_SCHUTZ, IT_SUPERSCHUTZ, NUM_ITEMS };
#define ITEM_SLOTS 16
typedef struct { const char *name; const char *desc; u16 price; } ItemData;
extern const ItemData items[NUM_ITEMS];
extern const ItemData held_items[NUM_HELD];

// ------------------------------------------------------------------ save state
#define PARTY_MAX 6
#define BOX_MAX 120
#define NUM_FLAGS 512

typedef struct {
    u32 magic;
    u16 version;
    u8 map, dir;
    u8 x, y;
    u8 ret_map, ret_x, ret_y;   // return point for shared interiors
    u8 heal_map, heal_x, heal_y; // last station
    u8 party_n;
    u8 box_n;
    u8 starter;
    u8 textspeed;
    u8 music_on;
    u32 money;
    u32 playtime;   // frames
    u8 flags[NUM_FLAGS / 8];
    u8 seen[128];
    u8 caught[128];
    u8 cur_gym;
    u8 rival_starter;   // 1 + index into starter_lines (0 = old save: classic rule)
    u8 pad2[2];
    u16 items[ITEM_SLOTS];
    Mon party[PARTY_MAX];
    Mon box[BOX_MAX];
    // v3: champion post-game
    u8 rematch[32];   // fights per recurring challenger (their level grows with it)
    u16 defenses;     // successful title defenses in the Champion-Halle
    u16 chal_wins;    // challenges won in the field
    u8 pad3[28];
    // v4: classic features
    Mon pension[2];       // Vexel-Pension (daycare)
    u8 pension_n, egg_ready;
    u16 pension_steps;
    struct { u16 species, steps; u8 mflags, nature, iv_bonus, pad; } eggs[3];   // incubator, species 0xFFFF = empty
    u16 repel;            // steps left
    u16 chal_prog;        // weighted steps since the last challenge (x4)
    u16 chal_target;      // steps until the next challenge (750..2500)
    u8 bike, expshare, follow, clock_off;
    u8 held_inv[16];      // counts of held items 1..16 (index = HI_* - 1)
    // v6: Vexikon bits for species 1024..1535
    u8 seen_hi[64];
    u8 caught_hi[64];
    // v7: Vexikon bits for species 1536..2047
    u8 seen_x[64];
    u8 caught_x[64];
    u32 checksum;
} SaveData;

extern SaveData G;

int flag(int f);
void set_flag(int f);
void clr_flag(int f);
void mark_seen(int sp);
void mark_caught(int sp);
int is_seen(int sp);
int is_caught(int sp);
int count_caught(void);
int count_seen(void);
int give_mon(const Mon *m);    // returns 0 party, 1 box, -1 full

int save_game(void);
int load_game(void);
int save_exists(void);
void save_init_new_fields(SaveData *s);

// ------------------------------------------------------------------ input / frame
extern u32 key_held, key_down, key_rep;
extern int touch_down, touch_held, tx, ty;
extern u32 frame_count;
extern int top_dirty, bot_dirty;
void frame(void);          // present + input + music
void wait_frames(int n);
int touched(int x, int y, int w, int h);   // tap started inside rect
u32 rnd(void);
int rnd_range(int a, int b); // inclusive

// ------------------------------------------------------------------ ui
void say(const char *speaker, const char *text);        // dialog box on top screen (blocks)
void say_keep(const char *speaker, const char *text);   // no wait for A at end
int ask_yesno(const char *speaker, const char *text);
int menu_list(const char *title, const char *const *opts, int n, int start); // bottom screen list, -1 = cancel
void draw_bottom_bg(void);
void draw_bottom_bg_dark(void);
void fade_out(int which);   // 1 top, 2 bottom, 3 both
void top_snapshot(void);
void top_restore(void);
void bot_snapshot(void);
void bot_restore(void);
void message_box_draw(u16 *fb, const char *speaker, const char *text, int chars, int y);
void draw_battle_bg(u16 *fb, int style, int t);
void str_cat_num(char *buf, int v);
void fmt(char *out, const char *f, ...);

// ------------------------------------------------------------------ music
enum { MUS_NONE, MUS_TITLE, MUS_TOWN, MUS_ROUTE, MUS_BATTLE, MUS_BOSS, MUS_VICTORY, MUS_CAVE, MUS_TOWER, MUS_HEAL, MUS_EVOLVE, MUS_FINAL, MUS_ENDING };
void music_play(int song);
void music_tick(void);
void sfx(int id);
enum { SFX_SELECT, SFX_CONFIRM, SFX_CANCEL, SFX_HIT, SFX_SUPER, SFX_WEAK, SFX_CATCH, SFX_SHAKE, SFX_LEVEL, SFX_HEAL, SFX_BUMP, SFX_ENCOUNTER, SFX_SAVE };

// ------------------------------------------------------------------ battle
typedef struct {
    const char *name;
    u8 chr;
    u8 n;
    u16 team[6][2];   // line << 3, level
    u16 money;
    const char *intro;
    const char *lose;  // what trainer says when defeated
    u8 boss;
    u8 bg;
} TrainerData;
extern const TrainerData trainers[];

enum { BR_WIN, BR_LOSE, BR_RUN, BR_CAUGHT };
int battle_wild(int species, int level, int bg);
int battle_wild_ex(int species, int level, int bg, int music);
int battle_trainer(int trainer_id);
int battle_party_alive(void);
void check_evolutions(void);
void evolve_scene(Mon *m, int new_species);

// ------------------------------------------------------------------ menus
void menu_main(void);
int party_menu(int mode, int cur);  // mode 0 view, 1 battle switch (returns index), 2 use item select
int bag_menu(int in_battle);        // returns item id or -1
void dex_menu(void);
void shop_menu(void);
void held_shop_menu(void);
void box_menu(void);
void mon_summary(Mon *m);

// ------------------------------------------------------------------ world
void world_run(void);
void world_warp(int map, int x, int y, int dir);
int world_style_bg(void);
void heal_party(void);
void world_draw(void);
void world_draw_bottom(void);
extern int cur_map;

// story flags
enum {
    F_INTRO_DONE = 1, F_GOT_STARTER, F_RIVAL1, F_GOT_DEX, F_LEFT_TOWN,
    F_GRUNT_PARK, F_UNUSED7, F_RIVAL2, F_UNUSED9, F_UNUSED10, F_RIVAL3, F_UNUSED12,
    F_TOWER_OPEN, F_ADMIN, F_RIVAL4, F_CEO, F_OMEGA, F_ENDING, F_MSG1, F_MSG2, F_MSG3, F_MSG4,
    F_ITEM_BASE = 40,     // 40..99 item pickups (flag number stored directly)
    F_TRAINER_BASE = 100, // 100..199 trainers
    F_BADGE_BASE = 200,   // 200..223 badges (gym leaders)
    F_GYMTR_BASE = 224,   // 224..271 gym trainers
    F_ELITE_BASE = 280,   // 280..283 Hoher Rat
    F_CHAMPION = 290, F_LEGEND_START, F_LEGEND_DONE,
    F_LEG_MET_BASE = 300,    // 300..307 legend encountered (unlocks the next one)
    F_CULT_BASE = 310,       // 310..317 cult battle at a legend won
    F_LEG_CAUGHT_BASE = 320, // 320..327 legend caught
    F_KEY_BIKE = 330, F_KEY_ROD, F_KEY_EXPSHARE, F_KEY_DRONE, F_KEY_INCUBATOR,
    F_VISIT_BASE = 340       // 340..352 cities visited (fast travel)
};
#define NUM_CITIES 13
typedef struct { u8 map, x, y; u8 mx, my; const char *name; } CityData;   // station door, region map position
extern const CityData cities[NUM_CITIES];
int city_index(int map);

// day / night (DS clock)
enum { DAY_MORNING, DAY_DAY, DAY_EVENING, DAY_NIGHT };
int day_phase(void);
int clock_minutes(void);   // 0..1439
extern const char *const phase_names[4];
#define NUM_GYMS 24
typedef struct { const char *name; u8 t1, t2, chr, map; const char *intro; const char *lose; } GymData;
extern const GymData gyms[NUM_GYMS];
int badge_count(void);
int rival_line(void);
extern const char *const alt_cond_names[5];
int alt_cond_met(const Mon *m, int cond);
int scaled_level(int base, int idx, int boss);
int species_for_level(int line, int level);
int battle_team(const char *name, int chr, Mon *team, int n, int money, int boss, int bg, int music);

void play_title(void);
void play_intro(void);
void play_ending(void);
