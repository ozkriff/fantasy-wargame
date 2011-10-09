/* See LICENSE file for copyright and license details. */

typedef struct { int x, y; } Vec2i;

/* map coordinates */
typedef Vec2i Mcrd; 

/* skills */
typedef enum {
  S_RANGE,
  S_INVIS,
  S_IGNR,
  S_ARMORED,
  S_ARMPIERC,
  S_NORETURN
} Skill_id;

typedef struct {
  int skill;
  int strength;
  int range;
} Skill_range;

typedef struct {
  Skill_id t;
  Skill_range range;
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
  Unit_type_id t;
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

typedef enum {
  T_GRASS,
  T_FOREST,
  T_WATER,
  T_HILLS,
  T_MOUNTEENS
} Tile_type_id;

typedef struct {
  bool visible; /*fog of war*/
  int  cost; /* cost of path for selunit to this tile */
  Tile_type_id t;
  Mcrd parent; /* used in pathfinding */
} Tile;

typedef enum {
  E_MOVE,
  E_MELEE,
  E_RANGE,
  E_ENDTURN,
  E_DEATH
} Event_type_id;

typedef struct {
  int u; /*unit's id*/
  int dir; /*direction index*/
  int cost;
} Event_move;

typedef struct {
  int a, d; /* attacker, defender */
  int attackers_killed;
  int defenders_killed;
} Event_melee;

typedef struct {
  int a, d;
  int defenders_killed;
} Event_range;

typedef struct {
  int old_id;
  int new_id;
} Event_endturn;

typedef struct {
  int id; /*Dead unit's id*/
} Event_death;

typedef struct {
  Event_type_id t;
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

