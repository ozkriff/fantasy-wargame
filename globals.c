
#define MAP_W  7
#define MAP_H 14

tile map[MAP_H][MAP_W];
l_new(st);    // stack for filling map
l_new(path);  // stores path
l_new(units); // stires units

// map node / path node / used in pathfinding
typedef struct { l_node n; mcrd crd; } mnode;

vec2i map_offset = {72,72/4};


SDL_Surface * sel; // selection
SDL_Surface * hl1; // draw possible tiles
SDL_Surface * hl3; // fog
SDL_Surface * hl5; // player 1 selection (red) ?
SDL_Surface * hl6; // player 2 selection (blue)?

SDL_Surface * grass;
SDL_Surface * water;
SDL_Surface * tree; // rename: forest
SDL_Surface * hill;  // hills
SDL_Surface * mount; // mounteens

SDL_Surface * terrsrf[5]={NULL};

SDL_Surface * arrow;

SDL_Surface * sldr_wa; // atack unit
SDL_Surface * sldr_wd; // defence unit
SDL_Surface * sldr_wb; // bow unit
SDL_Surface * sldr_wh; // hunter unit


SDL_Surface * screen = NULL;
bool done = false;

TTF_Font * font = NULL;

unit * selunit = NULL; // selected unit
mcrd selhex = {-1,-1};

#define _clr(r,g,b,a) SDL_MapRGBA(screen->format, r,g,b,a)
#define WHITE _clr(255,255,255, 255)
#define GREY  _clr(235,235,235, 255)
#define BLACK _clr(  0,  0,  0, 255)
#define RED   _clr(255,  0,  0, 255)
#define GREEN _clr(  0,255,  0, 255)
#define BLUE  _clr(  0,  0,255, 255)

vec2i tile_center = {72/2, 72*0.75}; // 36 54


#define MODE_SELECT 0
#define MODE_MOVE   1
#define MODE_ATTACK 2
int	mode;
int	player; // current player's index


// костыль: нужно на случай убийства защищающегося
// хранит координаты защищающегося или координаты бегства
mcrd	attack_crd; 
int	attack_index;
unit * 	attack_u1;
unit * 	attack_u2;
int	attack_stage; //0-наступление, 1-отход, 2-бегство
int	attack_is_counter;
bool    attack_is_shoot;
int     attack_shoot_index;


//количество промежуточных положений между клетками
#define STEPS 6
// положение юнита при движении между клетками(см. STEPS)
int	move_index; // индекс движения между клетками
mnode *	move_tile;
unit * 	move_unit;

// они вообще нужны?
mcrd zmcrd = {0,0};   // zero
mcrd nmcrd = {-1,-1}; // null


//grass forest water hills mount

//terrain movepoints
int tmvp1[] = {  1, 4, 8, 4, 9 };
int tmvp2[] = {  1, 2, 4, 3, 6 };
int tmvp3[] = {  1, 3, 8, 4, 9 };

//terrain defence bonus
int tdef1[] = {  0, 3,-4, 4, 3 };
int tdef2[] = {  0, 5, 0, 4, 4 };
int tdef3[] = {  0, 5, 0, 4, 4 };

//terrain attack bonus
int tatk1[] = {  0, 0,-4, 0, 0 };
int tatk2[] = {  0, 3,-4, 0, 0 };
int tatk3[] = {  0, 1,-4, 0, 0 };


unit_type utypes[] = {
  {1,5,10,4,6,4,"defc", tmvp1, tdef1, tatk1},
  {4,5, 4,4,4,6,"hunt", tmvp2, tdef2, tatk1},
  {3,5, 8,4,2,4,"arch", tmvp3, tdef3, tatk1},
};


