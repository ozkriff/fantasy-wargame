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
  unit * unit;
} tile;


#define FEATURE_RNG       1
#define FEATURE_BRSK      2
#define FEATURE_INVIS     3
#define FEATURE_IGNR      4
#define FEATURE_ARMORED   5
#define FEATURE_ARMPIERC  6

defstruct { int power;              } feature_berserk;
defstruct { int power, range, ammo; } feature_range;

typedef union {
  feature_range     rng;   // has ranged attack
  feature_berserk   brsk;  // berserk TODO
  bool invis;              // invisible
  bool ignr;               // ignore enemys ZOC
  bool armored;
  bool armour_piercing;    // ignores armor
} feature_data;

defstruct {l_node n; int type; feature_data data;} feature;


// map node / path node / used in pathfinding
defstruct { l_node n; mcrd crd; } mnode;


defstruct {int id; mcrd dest;} event_move ;
defstruct {int id0, id1;     } event_attack ;
defstruct {
  int type;
  union {event_move mv; event_attack attck;} data;
} event ;


defstruct {
  tile * map;
  l_list * st;    // stack for filling map
  l_list * path;  // stores path
  l_list * units; // stires units

  l_list * event_queue;

  unit * selunit;
  mcrd selhex;

  int mode;

  // костыль: нужно на случай убийства защищающегося
  // хранит координаты защищающегося или координаты бегства
  mcrd   attack_crd; 
  int    attack_index;
  unit * attack_u1;
  unit * attack_u2;
  int    attack_stage; //0-наступление, 1-отход, 2-бегство
  int    attack_is_counter;
  bool   attack_is_shoot;
  int    attack_shoot_index;


  // положение юнита при движении между клетками(см. STEPS)
  int     move_index; // индекс движения между клетками
  mnode * move_tile;
  unit *  move_unit;
} world;

