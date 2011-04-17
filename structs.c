#define defstruct  typedef struct 

defstruct { int x, y; } Vec2i;
typedef Vec2i Mcrd; //[m]ap coords
typedef Vec2i Scrd; //[s]creen coords


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
} Unit_type;



defstruct {
  Unit_type *  type;
  int   id;
  int   health;
  int   player;
  bool  can_attack;
  int   mvp;
  Mcrd  mcrd;
  Scrd  scrd;
  List * features;
} Unit;



defstruct {
  int  fog;  // fog of war. how many units see this tile
  int  cost; // цена полного пути до клетки
  int  type;
  Mcrd parent;
} Tile;


#define FEATURE_RNG       1
#define FEATURE_BRSK      2
#define FEATURE_INVIS     3
#define FEATURE_IGNR      4
#define FEATURE_ARMORED   5
#define FEATURE_ARMPIERC  6
#define FEATURE_NORETURN  7

defstruct { int power;              } Feature_berserk;
defstruct { int power, range, ammo; } Feature_range;

typedef union {
  Feature_range     rng;   // has ranged attack
  Feature_berserk   brsk;  // berserk TODO
  bool invis;              // invisible
  bool ignr;               // ignore enemys ZOC
  bool armored;
  bool armour_piercing;    // ignores armor
  bool noreturn;
} Feature_data;

defstruct {int type; Feature_data data;} Feature;


#define EVENT_MOVE   0
#define EVENT_MELEE  1
#define EVENT_RANGE  2

// 'int t;' means [t]ype
// md - Mcrd defender
defstruct { int t; int u; Mcrd dest; }   Event_move;
defstruct { int t; int a, d; Mcrd md; int dmg; }  Event_melee;
defstruct { int t; int a, d; Mcrd md; int dmg; }  Event_range;
typedef union {
  Event_move   move;
  Event_melee  melee;
  Event_range  range;
  /* ... */
  int t; // [t]ype
} Event;


defstruct {
  Tile * map;
  List * units; // stores units
  List * eq; // events queue
} World;


