/* See LICENSE file for copyright and license details. */

typedef struct { int x, y; } Vec2i;

/* map coordinates */
typedef Vec2i Mcrd; 


/* skills */
#define S_RANGE     1
#define S_BRSK      2
#define S_INVIS     3
#define S_IGNR      4
#define S_ARMORED   5
#define S_ARMPIERC  6
#define S_NORETURN  7

typedef struct { int t; int power; } Skill_berserk;
typedef struct { int t; int skill, strength, range; } Skill_range;

typedef union {
  int t; /* type: SKILL_* */
  Skill_range    range;
  Skill_berserk  brsk;
} Skill;



typedef struct {
  int range_of_vision;
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
  int   skills_n;
} Unit_type;


#define U_DEFENDER 0
#define U_HUNTER   1
#define U_ARCHER   2

typedef struct {
  int   t; /*UNIT_TYPE_**/
  int   id;
  int   count;
  int   player;
  bool  can_attack;
  int   mvp;
  Mcrd  m;
  Skill skills[10];
  int   skills_n;
  bool  visible;
} Unit;


#define T_GRASS     0
#define T_FOREST    1
#define T_WATER     2
#define T_HILLS     3
#define T_MOUNTEENS 4

typedef struct {
  int  fog;  /* fog of war. how many units see this tile. */
  int  cost; /* cost of path for selunit to this tile */
  int  t; /* TILE_* */
  Mcrd parent; /* used in pathfinding */
} Tile;

#define EVENT_MOVE    0
#define EVENT_MELEE   1
#define EVENT_RANGE   2
#define EVENT_ENDTURN 3
#define EVENT_DEATH   4

typedef struct {
  int t;
  int u; /*unit's id*/
  int dir; /*direction index*/
  int cost;
}  Event_move;

typedef struct {
  int t;
  int a, d; /* attacker, defender */
  int attackers_killed;
  int defenders_killed;
} Event_melee;
typedef struct { int t; int a, d; int dmg; }  Event_range;
typedef struct { int t; int old_player, new_player; } Event_endturn;
typedef struct { int t; Unit u; } Event_death;

typedef union {
  int t; /* EVENT_* */
  Event_move   move;
  Event_melee  melee;
  Event_range  range;
  Event_endturn endturn;
  Event_death  death;
} Event;


typedef struct {
  Tile * map;
  List   units;
  List   eq; /* events queue */
  bool   is_ai;
  int    id;
} World;

 

typedef struct {
  int   players_count;
  Mcrd  map_size;
  char *map;
  void (*init)(World *w);
  void (*logic)();
} Scenario;

