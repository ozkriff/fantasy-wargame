
Vec2i map_offset = {72,72/4};


SDL_Surface * sel; // selection
SDL_Surface * hl1; // draw possible tiles
SDL_Surface * hl3; // fog
SDL_Surface * hl5; // player 1 selection (red) ?
SDL_Surface * hl6; // player 2 selection (blue)?

SDL_Surface * grass;
SDL_Surface * water;
SDL_Surface * tree;  // rename: forest
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

#define WHITE SDL_MapRGBA(screen->format, 255,255,255, 255)
#define GREY  SDL_MapRGBA(screen->format, 235,235,235, 255)
#define BLACK SDL_MapRGBA(screen->format,   0,  0,  0, 255)
#define RED   SDL_MapRGBA(screen->format, 255,  0,  0, 255)
#define GREEN SDL_MapRGBA(screen->format,   0,255,  0, 255)
#define BLUE  SDL_MapRGBA(screen->format,   0,  0,255, 255)

Vec2i tile_center = {72*0.5, 72*0.75}; // 36 54



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


Unit_type utypes[] = {
  {1,5,10,5,6,3,"defc", tmvp1, tdef1, tatk1},
  {4,5,10,5,4,4,"hunt", tmvp2, tdef2, tatk1},
  {3,5,10,4,3,3,"arch", tmvp3, tdef3, tatk1},
};

//количество промежуточных положений между клетками
#define STEPS 6

int MAP_W = 7;
int MAP_H = 14;


int player; // current player's index
int players_count = 3;
World worlds[3];
World * cw = NULL; // current world


// TODO: 2 режима - МОД_ВЫБОР, МОД_ПОКАЗ_СОБЫТИЯ
// а сам тип события уже будет храниться в.. событии)
#define MODE_SELECT 0
#define MODE_MOVE   1
#define MODE_ATTACK 2
int mode;

// сделать локальными для путенахождения
List * st;    // stack for filling map
List * path;  // stores path

// сделать локальными для events.c
Unit * selunit;
Mcrd selhex;

// переименовать, слишком простое название
Event e;    // current event
int eindex;   // внутренний индекс визуализации события

