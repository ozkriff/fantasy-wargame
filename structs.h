
#define defstruct  typedef struct 

defstruct { int x, y; } Vec2i;

/* map coordinates */
typedef Vec2i Mcrd; 


defstruct {
  int   range_of_vision;
  int   morale;
  int   count;
  int   ms; /*melee_skill*/
  int   strength;
  int   toughness;
  int   attacks;
  int   armor;
  int   mvp; /*move points*/
  char * name;
  int * ter_mvp; /* terrain move cost */
  int * ter_ms; /* melee_skill bonuses at different terrains */
} Unit_type;



defstruct {
  Unit_type *t;
  int   id;
  int   count;
  int   player;
  bool  can_attack;
  int   mvp;
  Mcrd  m;
  List  features;
  bool  visible;
} Unit;


/*Tile.type*/
#define TILE_GRASS 0
#define TILE_FOREST 1
#define TILE_WATER 2
#define TILE_HILLS 3
#define TILE_MOUNTEENS 4

defstruct {
  int  fog;  /* fog of war. how many units see this tile. */
  int  cost; /* cost of path for selunit to this tile */
  int  t; /*type*/
  Mcrd parent; /* used in pathfinding */
} Tile;


#define FEATURE_RNG       1
#define FEATURE_BRSK      2
#define FEATURE_INVIS     3
#define FEATURE_IGNR      4
#define FEATURE_ARMORED   5
#define FEATURE_ARMPIERC  6
#define FEATURE_NORETURN  7

defstruct { int t; int power;        } Feature_berserk;
defstruct { int t; int skill, power, range; } Feature_range;

typedef union {
  int t; /*type*/
  Feature_range    rng;
  Feature_berserk  brsk;
} Feature;


#define EVENT_MOVE   0
#define EVENT_MELEE  1
#define EVENT_RANGE  2
#define EVENT_ENDTURN 3

defstruct { int t; int u; Mcrd dest; int cost; }  Event_move;
defstruct {
  int t;
  int a, d;               /* attacker, defender */
  int attackers_killed;
  int defenders_killed;
} Event_melee;
defstruct { int t; int a, d; int dmg; }  Event_range;
defstruct { int t; int old_player, new_player; } Event_endturn;

typedef union {
  int t; /*type*/
  Event_move   move;
  Event_melee  melee;
  Event_range  range;
  Event_endturn endturn;
} Event;


defstruct {
  Tile * map;
  List   units;
  List   eq; /* events queue */
  bool   is_ai;
  int    id;
} World;


defstruct {
  Mcrd m; /*map position*/
  int  player;
  int  type;
} Initial_unit_info;


defstruct {
  Tile *map;
  Mcrd map_size;
  List units;
  int  players_count;
} Scenario;

