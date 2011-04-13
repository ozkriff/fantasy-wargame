#define defstruct  typedef struct 

defstruct { int x, y; } vec2i;
typedef vec2i mcrd; //[m]ap coords
typedef vec2i scrd; //[s]creen coords


// TODO переставить местами поля
defstruct {
  int   see;
  int   morale;
  int   health;
  int   attack; // meele damage
  int   defence;
  int   mvp;
  char *  name;
  int *	ter_mvp; // terrain move cost
  int * ter_def; // terrain defence bonus
  int * ter_atk; // terrain attack bonus
} unit_type;



defstruct {
  l_node  n;
  unit_type *  type;
  int   id;
  int   health;
  int   player;
  bool  can_attack;
  int   mvp;
  mcrd  mcrd;
  scrd  scrd;
  l_list  features[1];
} unit;



defstruct {
  int  fog;  // fog of war. how many units see this tile
  int  cost; // цена полного пути до клетки
  int  type;
  mcrd parent;
} tile;


#define FEATURE_RNG       1
#define FEATURE_BRSK      2
#define FEATURE_INVIS     3
#define FEATURE_IGNR      4
#define FEATURE_ARMORED   5
#define FEATURE_ARMPIERC  6
#define FEATURE_NORETURN  7

defstruct { int power;              } feature_berserk;
defstruct { int power, range, ammo; } feature_range;

typedef union {
  feature_range     rng;   // has ranged attack
  feature_berserk   brsk;  // berserk TODO
  bool invis;              // invisible
  bool ignr;               // ignore enemys ZOC
  bool armored;
  bool armour_piercing;    // ignores armor
  bool noreturn;
} feature_data;

defstruct {l_node n; int type; feature_data data;} feature;


#define EVENT_MOVE   0
#define EVENT_MELEE  1
#define EVENT_RANGE  2

// byte-count  event_move   unit-id     directon[0..5]
// byte-count  event_melee  attacker-id direction   dead
// byte-count  event_range  attacker-id defender-id dead

defstruct { l_node n; int * data; } event ;



defstruct {
  tile * map;
  l_list * st;    // stack for filling map
  l_list * path;  // stores path
  l_list * units; // stores units
  l_list * eq; // events queue
  int * e; //event data
  //unit * su;
  unit * selunit;
  mcrd selhex;
  int mode;
  int index; // внутренний индекс события
  //int i; // внутренний индекс события
} world;


