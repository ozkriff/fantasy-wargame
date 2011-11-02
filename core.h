/* See LICENSE file for copyright and license details. */

typedef struct { int x, y; } Vec2i;

/* map coordinates */
typedef Vec2i Mcrd; 

typedef enum {
  D_S,
  D_SW,
  D_NW,
  D_N,
  D_NE,
  D_SE
} Direction;

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
  int shoots_max;
  int shoots; /*shoots left this turn*/
} Skill_range;

typedef struct {
  Skill_id t;
  Skill_range range;
} Skill;

typedef struct {
  int v; /*range of vision*/
  int morale;
  int morale_rg; /*regeneration per turn*/
  int count;
  int ms; /*melee_skill*/
  int strength;
  int toughness;
  int attacks;
  int armor;
  int mv; /*move points*/
  int ter_mv[10]; /* terrain move cost */
  int ter_ms[10]; /* melee_skill bonuses at different terrains */
  Skill skills[10];
  int skills_n;
  int energy; /*max unit's energy */
  int energy_rg; /*regeneration per turn*/
  int cost;
} Unit_type;

typedef struct {
  Unit_type_id t;
  int   id;
  int   count;
  int   player;
  bool  can_attack;
  int   mv;
  Mcrd  m;
  Skill skills[10];
  int   skills_n;
  bool  is_visible;
  int   energy;
  int   morale;
} Unit;

typedef enum {
  T_GRASS,
  T_FOREST,
  T_WATER,
  T_HILLS,
  T_MOUNTAINS
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
  void (*init)(void);
  void (*logic)(void);
} Scenario;

extern Mcrd    map_size;
extern Unit   *selected_unit;
extern Unit_type utypes[30];
extern bool    is_local;
extern bool    is_client_active;
extern Player *current_player;
extern List    units;
extern Tile   *map;

void  init (void);
void  cleanup (void);

void  move (Unit *moving_unit, Mcrd destination);
void  attack (Unit *a, Unit *d);
void  select_next_unit (void);
void  endturn (void);

bool  unshown_events_left (void);
Event *get_next_event (void);
void  apply_event (Event e);
void  undo_event ();

void  init_local_players (int n, int *ids);
void  init_local_players_s (char *s, ...);
void  set_scenario_id (int id);
void  init_net (char *host, int port);

