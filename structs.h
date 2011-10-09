/* See LICENSE file for copyright and license details. */

typedef struct { int x, y; } Vec2i;

/* map coordinates */
typedef Vec2i Mcrd; 

/* skills */
#define S_RANGE     1
#define S_INVIS     3
#define S_IGNR      4
#define S_ARMORED   5
#define S_ARMPIERC  6
#define S_NORETURN  7

typedef struct {
  int skill;
  int strength;
  int range;
} Skill_range;

typedef struct {
  int t; /* type: S_* */
  Skill_range    range;
} Skill;

typedef struct {
  int v; /*range of vision*/
  int morale;
  int count;
  int ms; /*melee_skill*/
  int strength;
  int toughness;
  int attacks;
  int armor;
  int mvp; /*move points*/
  int ter_mvp[10]; /* terrain move cost */
  int ter_ms[10]; /* melee_skill bonuses at different terrains */
  Skill skills[10];
  int skills_n;
  int energy; /*max unit's energy */
  int energy_rg; /*regeneration per turn*/
} Unit_type;

typedef struct {
  int   t; /* U_ */
  int   id;
  int   count;
  int   player;
  bool  can_attack;
  int   mvp;
  Mcrd  m;
  Skill skills[10];
  int   skills_n;
  bool  is_visible;
  int   energy;
} Unit;

#define T_GRASS     0
#define T_FOREST    1
#define T_WATER     2
#define T_HILLS     3
#define T_MOUNTEENS 4

typedef struct {
  bool visible; /*fog of war*/
  int  cost; /* cost of path for selunit to this tile */
  int  t; /* T_ */
  Mcrd parent; /* used in pathfinding */
} Tile;

#define E_MOVE    0
#define E_MELEE   1
#define E_RANGE   2
#define E_ENDTURN 3
#define E_DEATH   4

typedef struct {
  int u; /*unit's id*/
  int dir; /*direction index*/
  int cost;
}  Event_move;

typedef struct {
  int a, d; /* attacker, defender */
  int attackers_killed;
  int defenders_killed;
} Event_melee;

typedef struct {
  int a, d;
  int dmg;
} Event_range;

typedef struct {
  int old_id;
  int new_id;
} Event_endturn;

typedef struct {
  int id; /*Dead unit's id*/
} Event_death;

typedef struct {
  int t; /* E_ */
  int id;
  union {
    Event_move   move;
    Event_melee  melee;
    Event_range  range;
    Event_endturn endturn;
    Event_death  death;
  } e;
} Event;

typedef struct {
  int   id;
  bool  is_ai;
  int   last_event_id;
} Player;

typedef struct {
  int   players_count;
  Mcrd  map_size;
  char *map;
  void (*init)(void);
  void (*logic)(void);
} Scenario;

