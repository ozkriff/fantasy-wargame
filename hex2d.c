
/* =================================================== 
====================================================== TODO
- усталость юнитов от атак и слишком быстрого движения
  отдых юнитов для удаления усталости и лечения
- добавить конницу со способностью разбега
  основной атакой должен быть проскок через врага
- дальнобойных лучников (но с возможностью рукопашного боя)
- наверное, должны быть разные функции для начала
  рукопашной и стрельбы
- способность 'метание' (как у разведчиков и КВ)
- боевой дух, разбитые отряды, отступление
- огонь прикрытия
- добавить местность: болото
- добавить объекты: замок. деревня.
- хранить в юните его эранные координаты и рисовать по ним
- вынести таблицы местности из unit_type
- добавить зоны видимости, туман войны,
  засады, невидимок (видимы только в радиусе 1 клетки)
  прим: зона видимости полностью пересчитывается 
  на начало хода. при движении юнитов она только
  наращивается(т.е. уже известное не пропадает)
- updatefog должен вызывать не из draw(), а при изменении
  положения отрядов
- добавить базовую систему особенностей и состояний
- функция добавления в список типов юнитов. принимает кучу
  всего и строковое название типа, а не указаель
- читать карту, расстановку юнитов, типы юнитов из файла
- назвать move_index и attack_index понятнее
  move_index -> move_stepnumber/ move_stepcount
- заменить attack_shoot_index обычным индексом атаки.
  тогда приедтся рисовать стрелка отдельным макаром
- isinvis можно переписать и проверять не просто
  соседние клетки, а проходить по всем юнитам врага
  и проверять растояние до нашего юнита. тогда можно
  будет делать обнаружение юнитов на разных дистанциях
- добавить "хвост" за летящим снарядом. 
  рисовать в цикле bzline'ом
- хранить типы юнитов со связанной инфой в текстовых файлах
- особенность: бешенный. пока в радиусе 1 клетки есть враг
  юнит не будет передвигаться
- добавить для некоторых отрядов особенность "построение"
  т.е. построение для атаки, оборонительное, ченить
  увеличивающее скорость передвижения, т.д.
- присмотреться к либам:
  sdl_blitpool - вставить для опитмизации (потом)
  sdl_picofont - создать свой шрифт (на основе terminus) 
- добавить особенность: несколько атак за ход
- map должен быть динамически выделяемой памятью,
  а размеры - переменные а не константы
- убрать неиспользуемые/лишние поверхности
  назвать их понятнее
  удалить все .xpm файлы и назвать их понятнее
=================================================== BUGS
- при атаке область видимости атакующего юнита пропадает
  возможно, исправится обновлением области только
  при движении
- баг: если солдат окружен и ему надо отступить -
  - будет фигня.
=================================================== NOTES */
- старатсья писать все на английском
- писать короткие функции, что бы пустые строки в них были
  не нужны
- именовать функции согласно тому,что они делают.
  если имена выходят длинные - разбивать функции
- хорошенько разобраться с .png форматом
- начать вести лог изменений (комменты для каждого бекапа)
//==========================================================


#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

//==========================================================
#if 1 //============================================= L_LIST

// реализация двусвязного списка. списки, очереди и стеки.
// functions return a pointer to the node that was 
// inserted or removed.
// n-next p-prev h-head t-tail rem-remove

typedef struct l_node l_node;
typedef struct l_list l_list;
struct l_node { l_node * n; l_node * p; };
struct l_list { l_node * h; l_node * t; long count; };

#define l_new(L)  l_list (L)[1] = {{NULL, NULL, 0}}

// If after == NULL, then <new> will be added at the head 
// of the list, else it will be added following <after>.
l_node * l_ins(l_list * lst, l_node * new, l_node * after){
  l_node * pnode = after ? after : (l_node*)lst ;
  new->n = pnode->n;
  new->p = after;
  pnode->n = new;
  if(new->n)  new->n->p=new;  else  lst->t=new;
  lst->count++;
  return(new);
}

l_node * l_remv(l_list * lst, l_node * old){
  if(old){
    if(old->n)  old->n->p=old->p;  else  lst->t=old->p;
    if(old->p)  old->p->n=old->n;  else  lst->h=old->n;
    lst->count--;
  }
  return(old);
}

// L-list;  N-node;  A-after(insert N after this node);
#define l__ins_(L,N,A) l_ins((l_list*)L, (l_node*)N, A)
#define l_addh(L,N)    l__ins_( L, N, NULL            )
#define l_addn(L,N,A)  l__ins_( L, N, (l_node*)A      )
#define l_addt(L,N)    l__ins_( L, N, ((l_list*)L)->t )

// double linked list
#define l_remh(L ) l_remv((l_list*)L, ((l_list*)L)->h)
#define l_remt(L ) l_remv((l_list*)L, ((l_list*)L)->t)
#define l_rem(L,N) l_remv((l_list*)L, (l_node*)N     )
#define l_first(L) ((l_list*)L)->h
#define l_last(L)  ((l_list*)L)->t
#define l_next(N)  ((l_node*)N)->n
#define l_prev(N)  ((l_node*)N)->p

// stack
#define l_push l_addh
#define l_pop  l_remh

// queue
#define l_enq  l_addt
#define l_deq  l_remh

#endif //============================================ L_LIST
//==========================================================
#if 1 //============================================ STRUCTS
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

#endif //=========================================== STRUCTS
//==========================================================
#if 1 //=================================== GLOBAL VARIABLES

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

#endif //================================== GLOBAL VARIABLES
//==========================================================
#if 1 //============================================== UNITS

// это что угодно, только не модуль UNITS. думать.

#define FOR_EACH_UNIT \
  for(unit * u=(unit*)l_first(units); u; u=(unit*)l_next(u))

#define FOR_EACH_TILE \
  for(mcrd mc={0,0}; mc.y<MAP_H; mc.y++) \
    for(mc.x=0; mc.x<MAP_W; mc.x++)    
    
feature * find_feature(unit * u, int type){
  feature * f = (feature*)l_first(u->features);
  while(f){
    if(f->type == type)
      return(f);
    f = (feature*)l_next(f);
  }
  return(NULL);
}

#endif //============================================= UNITS
//==========================================================
#if 1 //========================================== ACCESSORY

// вспомогательные/служебные функции

void PrintFError (char * format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsprintf(buffer,format, args);
  perror(buffer);
  va_end(args);
}



// равны ли mcrd?
bool mcrdeq(mcrd a, mcrd b){ return(a.x==b.x&&a.y==b.y); }



// доступ к ячейке по mcrd
tile * mp(mcrd c){ return(&map[c.y][c.x]); }



// дистанция между клетками
int mdist(mcrd a, mcrd b) {
  a.x += a.y/2;
  b.x += b.y/2;
  
  int dx = b.x-a.x;
  int dy = b.y-a.y;
  
  return( (abs(dx) + abs(dy) + abs(dx-dy)) / 2 );
}



// int i - neib index
// получить соседа клетки
mcrd neib(mcrd a, int i) {
  int d[2][6][2] = {
    { {1,-1}, {1,0}, {1,1}, { 0,1}, {-1,0}, { 0,-1}, },
    { {0,-1}, {1,0}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, } };
  int dx = d[a.y%2][i][0];
  int dy = d[a.y%2][i][1];
  return( (mcrd){a.x+dx, a.y+dy} );
}



// в пределах карты?
bool inboard(mcrd t){
  return( t.x>=0 && t.x<MAP_W && t.y>=0 && t.y<MAP_H );
}



// расстояние между двумя экранными точками
int sdist(scrd a, scrd b) {
  int dx = abs(b.x - a.x);
  int dy = abs(b.y - a.y);

  return( sqrt(pow(dx, 2)+pow(dy, 2)) );
}



scrd map2scr(mcrd map) {
  scrd scr;

  // space bwetween tiles
  int space = 0;
  
  scr.y = map_offset.y  + map.y*(29+space);
  scr.x = map_offset.x  + map.x*(72+space);
  //if(map.y%2) scr.x += 36;
  if(map.y%2) scr.x -= 36;
  
  return(scr);
}



// find tile with nearest coords
// деление надва нужно потому,
// что клетки `сплющены` по вертикали
mcrd scr2map(scrd m) {
  mcrd min;            
  int min_dist = 9000;

  m.x/=2;

  FOR_EACH_TILE{
    scrd wp = map2scr(mc);
    wp.x += 36; wp.y += 54; wp.x/=2;
    if(sdist(m, wp) < min_dist){
      min_dist = sdist(m, wp);
      min = mc;
    }
  }
  return(min);
}



// назвать понятнее
// возвращает промежуточные координаты между клетками
scrd between(scrd a, scrd b, int i) {
  float dx = (float)(b.x-a.x)/STEPS;
  float dy = (float)(b.y-a.y)/STEPS;
  return( (scrd){a.x+dx*i, a.y+dy*i} );
}



scrd mbetween(mcrd a, mcrd b, int i){
  return(  between(map2scr(a), map2scr(b), i)  );
}


#endif //========================================= ACCESSORY
//==========================================================
#if 1 //======================================== PATHFINDING


void push(mcrd tile, mcrd parent, int newcost) {
  mp(tile)->cost   = newcost;
  mp(tile)->parent = parent;
  
  mnode * new = malloc(sizeof(mnode));
  new->crd = tile;
  l_push(st, new);
}



mcrd pop(){
  mnode * tmp = (mnode*)l_pop(st);
  mcrd crd = tmp->crd;
  free(tmp);
  return(crd);
}



bool is_invis (unit * u){
  if(!find_feature(u, FEATURE_INVIS)) return(false);

  for(int i=0; i<6; i++){
    mcrd nb = neib(u->mcrd, i);
    unit * u2 = mp(nb)->unit;
    if(u2 && u2->player != u->player)
      return(false);
  }
  return(true);
}



// returns corrected newcost
// сделать так, что б ход заканчивался на этой клетке
// т.е. дополнить стоимость до ближайшего кратного 
// в этом случае он и так закончится тут
int zoc(mcrd a, unit * u,int cost){
  if(find_feature(u, FEATURE_IGNR)) return(cost);

  int mvp = u->type->mvp;
  for(int i=0; i<6; i++){
    mcrd n = neib(a, i);
    if(inboard(n)
    && mp(n)->unit
    && cost%mvp!=0
    && mp(n)->unit->player != u->player
    && mp(n)->fog>0
    && !is_invis(mp(n)->unit) )
      return(cost + mvp - (cost % mvp));
  }
  return(cost);
}



// process neiborhood
void process_nbh (unit * u, mcrd t, mcrd nb){
  if( ! inboard(nb) ) return;

  // что бы не проходить через видимых врагов
  if(mp(nb)->unit 
  && mp(nb)->unit->player != player 
  && mp(nb)->fog > 0
  && !is_invis(mp(nb)->unit) )
    return;
  
  int n       = u->type->ter_mvp[mp(nb)->type];
  int newcost = zoc(nb, u, mp(t)->cost + n);
  int mvp     = u->type->mvp;

  if(mp(nb)->cost>newcost && newcost<=mvp)
    push(nb, t, newcost);
}



void fill_map(unit * u) {
  FOR_EACH_TILE{
    mp(mc)->cost   = 30000;
    mp(mc)->parent = nmcrd;
  }  
  push(u->mcrd, u->mcrd, 0); // push start point
  while(st->count>0){
    mcrd t = pop();
    for(int i=0; i<6; i++)
      process_nbh(u, t, neib(t, i));
  }
}



void clear_path(){ while(path->count) free(l_pop(path)); }


void addwaypoint(mcrd wp){
  mnode * new = malloc(sizeof(mnode));
  new->crd = wp;
  l_push(path, new);
}



void get_path(mcrd a){
  clear_path();
  while(mp(a)->cost!=0){
    addwaypoint(a);
    a = mp(a)->parent;
  }
  // добавляем отправную точку(где стоит юнит)
  addwaypoint(a);
}


#endif //======================================= PATHFINDING
//==========================================================
#if(1) //============================================= LOGIC


// change tile type
void change_tile(mcrd m){
  if(mp(m)->type ++ == 4)
    mp(m)->type = 0;
  if(selunit) fill_map(selunit);
}



void select_next_unit(){
  do{
    if(selunit && selunit != (unit*)l_last(units))
      selunit = (unit*)l_next(selunit);
    else
      selunit = (unit*)l_first(units);
  }while(selunit->player != player);
  fill_map(selunit);
}



void select_unit(mcrd m){
  selunit = mp(m)->unit;
  fill_map(selunit);
}



void kill_unit(unit * u){
  mp(u->mcrd)->unit = NULL;
  if(u == selunit) selunit = NULL;
  free( l_rem(units, u) );
}



// type: 0-рукопашаня, 1-стрельба, 2-метание, 3-таран
// u1 - атакующий, u2 - защищающийся
void start_attack(unit * u1, unit * u2, int type) {
  mode         = MODE_ATTACK;
  attack_u1    = u1;
  attack_u2    = u2;
  attack_crd   = attack_u2->mcrd;
  attack_index = 0;
  mp(attack_u1->mcrd)->unit = NULL;

  // стрельба или рукопашная?
  if(type==1){
    attack_shoot_index = 0;
    attack_is_shoot    = true;
  }else if(type==0){
    attack_stage      = 0; // наступление
    attack_is_shoot   = false;
    attack_is_counter = false;
  }
}



void start_moving(mcrd m) {
  if(mp(m)->cost > selunit->type->mvp || selunit->mvp == 0)
    return;

  move_unit = selunit;
  mp(selunit->mcrd)->unit = NULL;

  get_path(m);
  mode = MODE_MOVE;
  move_tile = (mnode*)l_first(path);
  move_index = 0;
}



void finish_movement(){
  mode = MODE_SELECT;

  selunit->mcrd = move_tile->crd;
  selunit->scrd = map2scr(move_tile->crd);
  mp(selunit->mcrd)->unit = move_unit;
  move_unit = NULL;

  if(find_feature(selunit, FEATURE_IGNR))
    selunit->mvp -= mp(selunit->mcrd)->cost;
  else
    selunit->mvp = 0;

  clear_path();
  fill_map(selunit);
}



void ambush(){
  mnode * mnd = (mnode*) l_next(move_tile);
  unit * u = mp(mnd->crd)->unit;
  if( u && u->player != player){
    finish_movement();
    start_attack(u, selunit, 0);
    //start_attack(selunit, u);

    // а можно разрешать окнтратаку, НО
    // но добавить специальный омдификатор "попал в засаду"
    // его действие будет прекращаться после первого боя
    // он будет уменьшать защиту и атаку
    
    // стыдно. исправить. что бы враг не ответил!
    attack_is_counter = 1;
    return;
  }
}



void move_logic(){
  if(move_index == STEPS){
    move_index = 0;
    move_tile = (mnode*)l_next(move_tile);
    if(move_tile == (mnode*)l_last(path))
      finish_movement();
    else
      ambush();
  }
  move_index++;
}



int calc_damage(unit * a, unit * b){
  a = b;
  return(4); // random
}



void on_reach_enemy(){
  int damage = calc_damage(attack_u1, attack_u2);
  attack_u2->health -= damage;
  if(attack_u2->health <= 0) {
    kill_unit(attack_u2);
    attack_u2 = NULL;
    fill_map(attack_u1);
  }
  attack_u1->can_attack = false;
  // теперь возвращаемся на тайл
  attack_stage = 1;
}



// если это была контратака - закончить махаться
// если нет и жив противник - начать контратаку
void on_return_after_attack(){
  mp(attack_u1->mcrd)->unit = attack_u1;
  if(attack_is_counter
  || !attack_u2
  //|| find_feature(attack_u2, FEATURE_RNG)){
  ){
    if(attack_u1->health<attack_u1->type->health){
      mp(attack_u1->mcrd)->unit = NULL;
      attack_crd = attack_u1->mcrd;
      for(int i=0; i<6; i++){
        mcrd xxx = neib(attack_u1->mcrd, i);
        if(!mp(xxx)->unit){
          attack_crd = xxx;
          attack_stage = 2;
          return;
        }
      }
    }else{
      attack_u1 = attack_u2 = NULL;
      mode = MODE_SELECT;
    }
  }else{
    start_attack(attack_u2, attack_u1, 0);
    attack_is_counter=true;
  }
}



void on_done_retreat(){
  mp(attack_crd)->unit = attack_u1;
  attack_u1->mcrd = attack_crd;
  attack_u1 = attack_u2 = NULL;
  mode = MODE_SELECT;
}



void shoot_attack(){
  scrd a = attack_u1->scrd;
  scrd b = attack_u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(attack_shoot_index >= steps){
    int dmg = calc_damage(attack_u1, attack_u2);
    attack_u2->health -= dmg;
    if(attack_u2->health <= 0) {
      kill_unit(attack_u2);
      fill_map(attack_u1);
    }
    mp(attack_u1->mcrd)->unit = attack_u1;
    attack_u1->can_attack = false;
    attack_u1 = attack_u2 = NULL;
    mode = MODE_SELECT;
  }

  attack_shoot_index++;
}



void attack_logic() {
  if(attack_is_shoot)
    shoot_attack();
  else{
    if(attack_stage==0 && attack_index==STEPS/2+1)
      on_reach_enemy();
    if(attack_stage==1 && attack_index==0)
      on_return_after_attack();
    if(attack_stage==2 && attack_index==STEPS)
      on_done_retreat();

    if(attack_stage==0) attack_index++;
    if(attack_stage==1) attack_index--;
    if(attack_stage==2) attack_index++;
  }
}



void logic(){
  if(mode==MODE_MOVE)	  move_logic();
  if(mode==MODE_ATTACK)	attack_logic();
}



void updatefog(){
  FOR_EACH_TILE{
    mp(mc)->fog=0;
    FOR_EACH_UNIT{
      if(u->player == player
      && mdist(mc, u->mcrd) <= u->type->see)
        mp(mc)->fog++;
    }
  }
}


#endif //============================================= LOGIC
//==========================================================
#if 1 //============================================= EVENTS

bool checkunitsleft(){
  FOR_EACH_UNIT {
    if(u->player == player)
      return(true);
  }
  return(false);
}



void onspace(){
  player++; if(player==3) player=0;

#if 0
  if(!checkunitsleft()){
    puts("WINNER!");
    exit(EXIT_SUCCESS);
  }
#endif

  selunit = NULL;

  FOR_EACH_UNIT{
    if(u->player == player){
      u->mvp = u->type->mvp;
      u->can_attack = true;
    }
  }
  
  //select_next_unit();
}



void keys(SDL_Event E){
  if(mode!=MODE_SELECT) return;

  switch(E.key.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q:
      done = true;
      break;
    case SDLK_UP:    map_offset.y += 54; break;
    case SDLK_DOWN:  map_offset.y -= 54; break;
    case SDLK_LEFT:  map_offset.x += 72; break;
    case SDLK_RIGHT: map_offset.x -= 72; break;
    case SDLK_SPACE: onspace();          break;
    case SDLK_r:     change_tile(selhex);break;
    case SDLK_n:     select_next_unit(); break;
    default: break;
  }
}



void mouseclick(SDL_Event E){
  if(mode!=MODE_SELECT) return;

  mcrd m = scr2map((scrd){E.button.x, E.button.y});
  unit * u = mp(m)->unit;

  if(u && u->player == player){
    select_unit(m);
  }else if(selunit){
    if(!u || (u && (is_invis(u)||!mp(m)->fog))){
      start_moving(m);
      return;
    }

    if(u && u->player!=player && selunit 
    && selunit->can_attack
    && !is_invis(u) && mp(m)->fog > 0){
      feature * rng = find_feature(selunit, FEATURE_RNG);
      if(rng){
        if(mdist(selunit->mcrd, m) <= rng->data.rng.range)
          start_attack(selunit, mp(m)->unit, 1);
      }else{
        if(mdist(selunit->mcrd, m) <= 1)
          start_attack(selunit, mp(m)->unit, 0);
      }
    }
  }
}



void mousemove(SDL_Event E){
  selhex = scr2map((scrd){E.button.x, E.button.y});
}



void events() {
  SDL_Event E;
  while(SDL_PollEvent(&E)){
    if(E.type==SDL_QUIT)            done = true;
    if(E.type==SDL_KEYUP)           keys(E);
    if(E.type==SDL_MOUSEBUTTONDOWN) mouseclick(E);
    if(E.type==SDL_MOUSEMOTION)     mousemove(E);
    if(E.type==SDL_VIDEORESIZE){
      Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
      screen = SDL_SetVideoMode(E.resize.w,
          E.resize.h, 32, flags);
    }
  }
}

#endif //============================================ EVENTS
//==========================================================
#if 1 //=============================================== DRAW


// просто рисует пиксель
void pxl32(int x, int y, Uint32 pixel) {
  if( x<0 || y<0 || x >= screen->w || y >= screen->h )
    return;
  Uint32 * pixels = (Uint32 *)screen->pixels;
  pixels[ (y * screen->w) + x ] = pixel;
}


// рисует "большой"(size)  пиксель
void pxl(Uint32 colr, int size, int x, int y){
  SDL_Rect rect = {x, y, size, size};
  SDL_FillRect(screen, &rect, colr);
}



#define bzpxl(x,y,clr) \
  if(!steep) pxl32(x,y,clr); else pxl32(y,x,clr);
#define swap(a,b) { int tmp;  tmp=a; a=b; b=tmp; }

void bzline (scrd a, scrd b, Uint32 clr){
  bool steep = abs(b.y-a.y) > abs(b.x-a.x);
  if(steep)   { swap(a.x,a.y); swap(b.x,b.y); }
  if(a.x>b.x) { swap(a.x,b.x); swap(a.y,b.y); }
  int deltax = b.x-a.x;
  int deltay = abs(b.y-a.y);
  int error = deltax >> 1;
  int y = a.y, ystep = (a.y<b.y) ? 1 : -1;
  for (int x = a.x; x <= b.x; x++) {
    bzpxl(x,y,clr);
    error -= deltay;
    if(error<0){ y+=ystep; error+=deltax; }
  }
}



void blit(SDL_Surface *src, int x, int y) {
  SDL_Rect rect = {x, y, 0, 0};
  SDL_BlitSurface(src, NULL, screen, &rect);
}



void mblit(SDL_Surface *src, mcrd crd) {
  blit(src, crd.x, crd.y);
}



void draw_map() {
  FOR_EACH_TILE{
    mblit(terrsrf[ mp(mc)->type ], map2scr(mc));
    if(mp(mc)->fog<=0)  mblit(hl3, map2scr(mc));
  }
}
  


// TODO rename: mconnect? m2mline?
// draw line between 2 tiles
void mline(mcrd a, mcrd b){
  scrd sa = map2scr(a), sb = map2scr(b);
  sa.x+=36; sa.y+=54; sb.x+=36; sb.y+=54;
  if(mp(a)->cost <= selunit->mvp)
    bzline(sa, sb, BLUE);
  else
    bzline(sa, sb, RED);
}



// draw current path
void draw_path() {
  if(path->count>0){
    mnode * tile = (mnode*)l_first(path);
    while(l_next(tile)){
      if(l_next(tile)){
        mcrd a = tile->crd;
        mcrd b = ((mnode*)l_next(tile))->crd;
        mline(a,b);
      }
      tile = (mnode*)l_next(tile);
    }
  }
}



// draw path to some point
void draw_path_2_mcrd(mcrd a){
  if(mp(a)->cost==30000) return;
  mcrd tmp = a;
  while( ! mcrdeq(tmp,selunit->mcrd) ){
    mline(tmp, mp(tmp)->parent);
    tmp = mp(tmp)->parent;
  }
}



void draw_bg(Uint32 clr){ SDL_FillRect(screen,NULL,clr); }



void text(char * str, scrd crd, bool iscentred){
  SDL_Color col = {0xFF, 0xFF, 0xFF, 0xFF};
  SDL_Surface * s = TTF_RenderText_Blended(font, str, col);
  
  SDL_Rect rect;
  if(iscentred)
    rect = (SDL_Rect){crd.x-s->w/2, crd.y-s->h/2, 0,0};
  else
    rect = (SDL_Rect){crd.x, crd.y, 0,0};
    
  SDL_BlitSurface(s, NULL, screen, &rect);
  SDL_FreeSurface(s);
}



SDL_Surface * type2srf(unit_type * t){
       if(t==&utypes[0]) return(sldr_wd);
  else if(t==&utypes[1]) return(sldr_wh);
  else if(t==&utypes[2]) return(sldr_wa);
  else exit(1);
}



void draw_unit(unit *u){
  scrd s = map2scr(u->mcrd);
  if(u->player==0) mblit(hl5, s);
  if(u->player==1) mblit(hl6, s);
  mblit(type2srf(u->type), s);

  if(0){
    char str[100];
    sprintf(str, "[%i.%i.%i.%i]",
        u->health, u->mvp>0, u->can_attack,
        (bool)find_feature(u, FEATURE_RNG));
    text(str, (scrd){s.x+10, s.y+60}, false);
  }
}



void draw_units(){
  FOR_EACH_UNIT{
    if(move_unit!=u && attack_u1!=u
    && mp(u->mcrd)->fog>0 
    && !(u->player!=player && is_invis(u)) )
      draw_unit(u);
  }
}



void draw_moving_unit(){
  mcrd a = move_tile->crd;
  mcrd b = ((mnode*)l_next(move_tile))->crd;
  scrd crd = mbetween(a, b, move_index);
  mblit(type2srf(move_unit->type), crd);
}



void draw_attacking_unit(){
  mcrd a = attack_u1->mcrd;
  mcrd b = attack_crd; //attack_u2->crd;
  scrd crd = mbetween(a, b, attack_index);
  mblit(type2srf(attack_u1->type), crd);
}



void draw_possible_tiles(){
  FOR_EACH_TILE{
    mcrd p = mp(mc)->parent;
    if(!(p.x==0 && p.y==0)
    && mp(mc)->cost <= selunit->mvp) {
      mblit(hl1, map2scr(mc));
      mline(mc, p);
    }
  }
}



void maptext(){
  FOR_EACH_TILE{
    if(mp(mc)->cost!=30000 && mp(mc)->cost!=0){
      scrd s = map2scr(mc);
      s.x+=36; s.y+=54;
      char str[100];
      sprintf(str, "%i", mp(mc)->cost);
      text(str, s, true);
    }
  }
}



void draw_shoot_attack(){
  scrd a = map2scr(attack_u1->mcrd);
  scrd b = map2scr(attack_u2->mcrd);
  int steps = sdist(a,b)/6;
  float dx  = (float)(b.x-a.x)/steps;
  float dy  = (float)(b.y-a.y)/steps;

  a.x += dx * attack_shoot_index;
  a.y += dy * attack_shoot_index;

  // пройденное снарядом расстояние. от 0.0 до 1.0
  float xxx = (float)attack_shoot_index/steps;

  // вертикальная поправка
  int dh = 36 * sinf(xxx*3.14);

  blit(arrow, a.x, a.y-dh);
}




void draw(){
  draw_bg(BLACK);

  // TODO нужно вызывать не отсюда. смотри NOTES
  updatefog(); 

  draw_map();
  if(mode==MODE_SELECT && selunit)
    draw_possible_tiles();
  mblit(sel, map2scr(selhex));
  if(selunit) mblit(sel, map2scr(selunit->mcrd));
  draw_units();
  if(mode==MODE_MOVE){ draw_moving_unit(); draw_path(); }
  if(mode==MODE_ATTACK){
    draw_attacking_unit();
    if(attack_is_shoot) draw_shoot_attack();
  }
  //maptext();
  text( (player==0)?"[pl:0]":"[pl:1]", (scrd){0,0}, false);
  
  SDL_Flip(screen);
}

#endif //============================================== DRAW
//==========================================================
#if 1 //=============================================== INIT


SDL_Surface * loadimg(char * str){
  SDL_Surface * surf = IMG_Load(str);
  if(!surf){ puts("img load error!"); exit(1); }
  SDL_Surface * optsurf = SDL_DisplayFormatAlpha(surf);
  free(surf);
  return(optsurf);
}



void load_sprites(){
  terrsrf[0] = loadimg("img/grass.xpm");
  terrsrf[1] = loadimg("img/tree.xpm");
  terrsrf[2] = loadimg("img/water.xpm");
  terrsrf[3] = loadimg("img/hills.xpm");
  terrsrf[4] = loadimg("img/mounteen.xpm");

  arrow = loadimg("img/w/arrow.xpm");

  sel = loadimg("img/sel.xpm");
  hl1 = loadimg("img/hl1.png");
  hl3 = loadimg("img/hl3.png");
  hl5 = loadimg("img/hl5.png");
  hl6 = loadimg("img/hl6.png");

  sldr_wa = loadimg("img/w/b.png");
  sldr_wd = loadimg("img/w/d.png");
  sldr_wb = loadimg("img/w/b.png");
  sldr_wh = loadimg("img/w/h.png");
}



void initmapcost(){
  char * ini = 
    "   . . * . . . *  "
    "  * . . t . M .   "
    "   . . . . . . .  "
    "  . . h . M . .   "
    "   . . . . . . .  "
    "  . . . t M . .   "
    "   . . . t . * .  "
    "  t t . . . . .   "
    "   . . . . . . *  "
    "  . * . * * . *   "
    "   . * . . . * .  "
    "  . . * * * . .   "
    "   . . . . . . .  "
    "  . . . . . . .   "
    "\0";
    
  int mi = 0; // индекс в массиве 'map'
  for(int i=0; ini[i]!='\0'; i++){
    if(ini[i]!=' '){
      tile * t = (tile*)map + mi;
      if(ini[i]=='.') t->type = 0;
      if(ini[i]=='t') t->type = 1;
      if(ini[i]=='*') t->type = 2;
      if(ini[i]=='h') t->type = 3;
      if(ini[i]=='M') t->type = 4;
      mi++;
    }
  }
}


void add_feature(unit * u, int type, feature_data * data){
  feature * f = malloc(sizeof(feature));
  f->type = type;
  f->data = *data;
  l_push(u->features, f);
}



void add_unit(mcrd crd, int player, unit_type * type) {
  unit * u  = malloc(sizeof(unit));
  u->player = player;
  u->mvp    = type->mvp;
  u->can_attack = true;
  u->health = type->health;
  u->mcrd   = crd;
  u->scrd   = map2scr(crd);
  u->type   = type;
  // на всякий инициализировать список
  *(u->features) = (l_list){0, 0, 0};
  mp(crd)->unit = u;
  
  // инициализировать нужные особенности юнитов
  if(type == &utypes[2]){ // archer
    feature_range fd = {5,2,999};
    add_feature(u, FEATURE_RNG, (feature_data*)&fd );
  }
  if(type == &utypes[1]) { // hunter
    int btrue = 1;
    add_feature(u, FEATURE_IGNR,  (feature_data*)&btrue);
    add_feature(u, FEATURE_INVIS, (feature_data*)&btrue);
  }

  // add new unit to list "units"
  l_push(units, u);
}



void init(){
  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  atexit(SDL_Quit);

  Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
  //screen = SDL_SetVideoMode(320, 240, 32, flags);
  screen = SDL_SetVideoMode(640, 480, 32, flags);

  char * f = "LiberationMono-Regular.ttf";
  font = TTF_OpenFont(f, 12);

  load_sprites();
  initmapcost();

  player = 0;
  mode = MODE_SELECT;
  selunit = NULL;

  add_unit( (mcrd){1,2}, 0, &utypes[0] );
  add_unit( (mcrd){1,3}, 0, &utypes[0] );
  add_unit( (mcrd){1,4}, 0, &utypes[0] );
  add_unit( (mcrd){1,5}, 0, &utypes[1] );
  add_unit( (mcrd){2,5}, 0, &utypes[1] );
  add_unit( (mcrd){2,6}, 0, &utypes[1] );
            
  add_unit( (mcrd){3,5}, 1, &utypes[0] );
  add_unit( (mcrd){3,1}, 1, &utypes[2] );
  add_unit( (mcrd){3,2}, 1, &utypes[2] );
  add_unit( (mcrd){3,3}, 1, &utypes[2] );

  add_unit( (mcrd){6,6}, 2, &utypes[1] );
}


#endif //============================================== INIT
//==========================================================


void mainloop(){
  while(!done){
    events();
    logic();
    draw();
    SDL_Delay(1.0*33);
  }
}


void args(int ac, char **av){
  if(ac!=1 || av==NULL)
    exit(EXIT_FAILURE);
}



#undef main
int main(int ac, char **av){
  args(ac,av);
  init();
  mainloop();
  return(EXIT_SUCCESS);
}

