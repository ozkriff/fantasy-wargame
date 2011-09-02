/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_net.h"
#include "list.h"
#include "structs.h"
#include "net.h"
#include "core.h"
#include "misc.h"
#include "path.h"
#include "ai.h"


/* screen coordinates */
typedef Vec2i Scrd;

typedef SDL_Surface * Img;


static Img terrain_tiles[10];
static Img img_units[10];
static Img img_selected_hex;
static Img img_reacheble_tile;
static Img img_fog_of_war;
static Img img_arrow;
static Img screen = NULL;

static Img img_rings[10];

static TTF_Font * font = NULL;

static Vec2i map_offset = {72, 72/4};

static Uint32 red, black, green, blue, white;

/* mouse points to this hex */
static Mcrd selected_tile; /* selected hex */

#define MODE_SELECT     0
#define MODE_SHOW_EVENT 1
static int ui_mode = MODE_SELECT;

static Event e;    /* current [e]vent */

/* event progress? */
/* Internal index of event visualisation. */
static int eindex;

/*If eindex==eindex=last, then event is shown.*/
static int final_eindex;
static int steps = 6;

static bool done;



static Img
loadimg (char * str){
  Img original = IMG_Load(str);
  Img optimized; /*optimized*/
  if(!original){
    printf("loadimg error: %s/ \n", str);
    exit(EXIT_FAILURE);
  }
  optimized = SDL_DisplayFormatAlpha(original);
  SDL_FreeSurface(original);  
  return(optimized);
}



static void
free_sprites (){
  SDL_FreeSurface(terrain_tiles[0]  );
  SDL_FreeSurface(terrain_tiles[1]  );
  SDL_FreeSurface(terrain_tiles[2]  );
  SDL_FreeSurface(terrain_tiles[3]  );
  SDL_FreeSurface(terrain_tiles[4]  );
  SDL_FreeSurface(img_arrow         );
  SDL_FreeSurface(img_selected_hex  );
  SDL_FreeSurface(img_reacheble_tile);
  SDL_FreeSurface(img_fog_of_war    );
  SDL_FreeSurface(img_rings[0]      );
  SDL_FreeSurface(img_rings[1]      );
  SDL_FreeSurface(img_rings[2]      );
  SDL_FreeSurface(img_units[UNIT_TYPE_DEFENDER]);
  SDL_FreeSurface(img_units[UNIT_TYPE_ARCHER]  );
  SDL_FreeSurface(img_units[UNIT_TYPE_HUNTER]  );
}



static void
load_sprites (){
  terrain_tiles[TILE_GRASS    ] = loadimg("img/grass.png"   );
  terrain_tiles[TILE_FOREST   ] = loadimg("img/tree.png"    );
  terrain_tiles[TILE_WATER    ] = loadimg("img/water.png"   );
  terrain_tiles[TILE_HILLS    ] = loadimg("img/hills.png"   );
  terrain_tiles[TILE_MOUNTEENS] = loadimg("img/mounteen.png");
  img_arrow          = loadimg("img/arrow.png");
  img_selected_hex   = loadimg("img/sel.png");
  img_reacheble_tile = loadimg("img/reacheble_tile.png");
  img_fog_of_war     = loadimg("img/fow.png");
  img_rings[0]       = loadimg("img/ring_red.png");
  img_rings[1]       = loadimg("img/ring_blue.png");
  img_rings[2]       = loadimg("img/ring_green.png");
  img_units[UNIT_TYPE_DEFENDER] = loadimg("img/defender.png");
  img_units[UNIT_TYPE_ARCHER]   = loadimg("img/archer.png"  );
  img_units[UNIT_TYPE_HUNTER]   = loadimg("img/hunter.png" );
}



static Scrd
mk_scrd (int x, int y){
  Scrd s;
  s.x = x;
  s.y = y;
  return(s);
}



/* Get distanse between two screen points. */
static int
sdist (Scrd a, Scrd b) {
  int dx = abs(b.x - a.x);
  int dy = abs(b.y - a.y);
  return( sqrt(pow(dx, 2)+pow(dy, 2)) );
}



static Scrd
map2scr (Mcrd map) {
  Scrd scr;
  scr.y = map_offset.y  + map.y*(96*3/4);
  scr.x = map_offset.x  + map.x*(96);
  if(map.y%2) scr.x -= 96/2;
  return(scr);
}



/* find tile with nearest coords */
static Mcrd
scr2map (Scrd s) {
  Mcrd min;
  int min_dist = 9000;
  Mcrd m;
  FOR_EACH_MCRD(m){
    Scrd wp = map2scr(m);
    wp.x += 96/2; wp.y += 96/2;
    if(sdist(s, wp) < min_dist){
      min_dist = sdist(s, wp);
      min = m;
    }
  }
  return(min);
}



/* Find point between two points. */
static Scrd
get_midpoint (Scrd a, Scrd b, int i) {
  float dx = (float)(b.x-a.x)/steps;
  float dy = (float)(b.y-a.y)/steps;
  return( mk_scrd(a.x+dx*i, a.y+dy*i) );
}



static Scrd
mbetween(Mcrd a, Mcrd b, int i){
  return(  get_midpoint(map2scr(a), map2scr(b), i)  );
}



static void
pixel (int x, int y, Uint32 pixel) {
  Uint32 * pixels = (Uint32 *)screen->pixels;
  if( x<0 || y<0 || x >= screen->w || y >= screen->h )
    return;
  pixels[ (y * screen->w) + x ] = pixel;
}



/* en.wikipedia.org/wiki/Bresenham's_line_algorithm */

#define bzpxl(x,y,clr) \
  if(!steep) pixel(x,y,clr); else pixel(y,x,clr);
#define swap(a,b) { int tmp;  tmp=a; a=b; b=tmp; }

static void
bzline (Scrd a, Scrd b, Uint32 clr){
  int deltax, deltay;
  int error, y, ystep, x;
  bool steep;
  steep = abs(b.y-a.y) > abs(b.x-a.x);
  if(steep)   { swap(a.x,a.y); swap(b.x,b.y); }
  if(a.x>b.x) { swap(a.x,b.x); swap(a.y,b.y); }
  deltax = b.x-a.x;
  deltay = abs(b.y-a.y);
  error = deltax >> 1;
  y = a.y;
  ystep = (a.y<b.y) ? 1 : -1;
  for (x = a.x; x <= b.x; x++) {
    bzpxl(x,y,clr);
    error -= deltay;
    if(error<0){ y+=ystep; error+=deltax; }
  }
}



static void
draw_img (Img src, Scrd crd) {
  SDL_Rect rect;
  rect.x = (Sint16)crd.x;
  rect.y = (Sint16)crd.y;
  SDL_BlitSurface(src, NULL, screen, &rect);
}



static void
draw_map (){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Img s = terrain_tiles[tile(m)->t];
    draw_img(s, map2scr(m));
    if(tile(m)->fog <= 0)
      draw_img(img_fog_of_war, map2scr(m));
  }
}



/* Draw line between 2 tiles. */
static void 
m2m_bzline (Mcrd a, Mcrd b){
  Scrd sa = map2scr(a), sb = map2scr(b);
  sa.x += 96/2;
  sa.y += 96/2;
  sb.x += 96/2;
  sb.y += 96/2;
  bzline(sa, sb, red);
}



static void
draw_bg (Uint32 clr){
  SDL_FillRect(screen, NULL, clr);
}



static void
text (char * str, Scrd crd, bool is_centred){
  SDL_Rect rect;
  SDL_Color col = {255, 255, 255, 255};
  Img img = TTF_RenderText_Blended(font, str, col);
  rect.x = (Sint16)crd.x;
  rect.y = (Sint16)crd.y;
  if(is_centred){
    rect.x -= img->w / 2;
    rect.y -= img->h / 2;
  }
  SDL_BlitSurface(img, NULL, screen, &rect);
  SDL_FreeSurface(img);
}



static Img
type2img (int t){
  return(img_units[t]);
}



static void
draw_unit (Unit *u){
  Scrd s = map2scr(u->m);
  draw_img(img_rings[u->player], s);
  draw_img(type2img(u->t), s);
  if(1){
    char str[100];
    sprintf(str, "%i", (int)u->count);
    text(str, mk_scrd(s.x+10, s.y+60), false);
  }
}



static void
draw_units (){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(ui_mode == MODE_SHOW_EVENT){
      if(e.t==EVENT_MOVE  && u->id == e.move.u)
        continue;
      if(e.t==EVENT_MELEE && u->id == e.melee.a)
        continue;
    }
    if(u->visible){
      draw_unit(u);
    }
  }
}



/*Mark tiles that sulunit can reach during this turn.*/
static void
draw_reachable_tiles(){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Mcrd p = tile(m)->parent;
    if(tile(m)->cost <= selunit->mvp) {
      draw_img(img_reacheble_tile, map2scr(m));
      m2m_bzline(m, p);
    }
  }
}



/*debugging*/
/*
static void
maptext (){
  char str[40];
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(tile(m)->cost != 30000 
    && tile(m)->cost != 0){
      Scrd s = map2scr(m);
      s.x += 36;
      s.y += 54;
      sprintf(str, "%i", tile(m)->cost);
      text(str, s, true);
    }
  }
}
*/



static void
draw_move_event (){
  Unit * u = id2unit(e.move.u);
  Scrd crd = mbetween(u->m, neib(u->m, e.move.dir), eindex);
  draw_img(type2img(u->t), crd);
}



static void
draw_melee_event (){
  Mcrd a = id2unit(e.melee.a)->m;
  Mcrd b = id2unit(e.melee.d)->m;
  int i = (eindex<steps/2) ? (eindex) : (steps-eindex);
  Scrd crd = mbetween(a, b, i);
  draw_img(type2img(id2unit(e.melee.a)->t), crd);
}



static void
draw_range_event (){
  Unit *u1 = id2unit(e.range.a);
  Unit *u2 = id2unit(e.range.d);
  int dist = mdist(u1->m, u2->m);
  /*[v]ertical [c]orrection*/
  int vc   = (int)(dist*14 * sin((eindex*3.14)/final_eindex));
  Scrd a   = map2scr(u1->m);
  Scrd b   = map2scr(u2->m);
  a.x     += ((b.x-a.x)*eindex)/final_eindex;
  a.y     += ((b.y-a.y)*eindex)/final_eindex;
  draw_img(img_arrow, mk_scrd(a.x, a.y-vc));
}



static void
draw_event (){
  if(e.t==EVENT_MOVE ) 
    draw_move_event ();
  if(e.t==EVENT_MELEE) 
    draw_melee_event();
  if(e.t==EVENT_RANGE) 
    draw_range_event();
}



static void
draw_labels (){
  char str[100];
  sprintf(str, "[pl:%i]", cw->id);
  text(str, mk_scrd(0,0), false);
}



static void
draw (){
  draw_bg(black);
  draw_map();
  if(ui_mode==MODE_SELECT && selunit)
    draw_reachable_tiles();
  if(selunit)
    draw_img(img_selected_hex, map2scr(selunit->m));
  draw_img(img_selected_hex, map2scr(selected_tile));
  draw_units();
  if(ui_mode==MODE_SHOW_EVENT)
    draw_event();
  /*maptext();*/
  draw_labels();
  SDL_Flip(screen);
}



static void
init_colors (){
  Uint8 x = 255;
  SDL_PixelFormat * fmt = screen->format;
  red   = SDL_MapRGBA(fmt, x, 0, 0, x);
  green = SDL_MapRGBA(fmt, 0, x, 0, x);
  blue  = SDL_MapRGBA(fmt, 0, 0, x, x);
  white = SDL_MapRGBA(fmt, x, x, x, x);
  black = SDL_MapRGBA(fmt, 0, 0, 0, x);
}



static void
init_draw (){
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  atexit(SDL_Quit);
  screen = SDL_SetVideoMode(640, 480,
      32, SDL_SWSURFACE | SDL_RESIZABLE);
  font = TTF_OpenFont("font.ttf", 12);
  load_sprites();
  init_colors();
  selected_tile = mk_mcrd(-1,-1);
}



static void
mouseclick (SDL_Event e){
  Scrd s = mk_scrd((int)e.button.x, (int)e.button.y);
  Mcrd m = scr2map(s);
  Unit *u = unit_at(m);
  if(u && u->player == cw->id){
    selunit = u;
    fill_map(selunit);
  }else if(selunit){
    if( !u || (u && (!u->visible||!tile(m)->fog)) ){
      if(tile(m)->cost <= selunit->mvp)
        move(selunit, m);
      return;
    }
    if(u && u->player!=cw->id
    && selunit && selunit->can_attack
    && u->visible && tile(m)->fog > 0){
      attack(selunit, u);
    }
  }
}



static void
mousemove (SDL_Event e){
  Scrd s = mk_scrd((int)e.button.x, (int)e.button.y);
  selected_tile = scr2map(s);
}



/* Only in MODE_SHOW_EVENT */
static void
event_keys (SDL_Event e){
  switch(e.key.keysym.sym) {
    /* TODO finish event */
    default: break;
  }
}



/* In both ui_modes. */
static void
common_keys (SDL_Event e){
  switch(e.key.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q:
      done = true;
      break;
    case SDLK_UP:    map_offset.y += 58; break;
    case SDLK_DOWN:  map_offset.y -= 58; break;
    case SDLK_LEFT:  map_offset.x += 72; break;
    case SDLK_RIGHT: map_offset.x -= 72; break;
    default: break;
  }
}



/* Only in SELECT_MODE. */
static void
select_keys (SDL_Event e){
  switch(e.key.keysym.sym) {
    case SDLK_SPACE:
      endturn();
      selunit=0;
      break;
    /*case SDLK_r:     change_tile(selected_tile);break;*/
    case SDLK_n:     select_next_unit(); break;
    default: break;
  }
}



static void
keys (SDL_Event e){
  common_keys(e);
  if(!is_active)
    return;
  if(ui_mode == MODE_SELECT)
    select_keys(e);
  else
    event_keys(e);
}



static void
sdl_events (){
  SDL_Event e;
  while(SDL_PollEvent(&e)){
    switch(e.type){
      case SDL_QUIT:            done = true;   break;
      case SDL_KEYUP:           keys(e);       break;
      case SDL_MOUSEBUTTONDOWN:
        if(is_active && ui_mode == MODE_SELECT)
          mouseclick(e);
        break;
      case SDL_MOUSEMOTION:     mousemove(e);  break;
      case SDL_VIDEORESIZE:
        screen = SDL_SetVideoMode(e.resize.w, e.resize.h,
            32, SDL_SWSURFACE | SDL_RESIZABLE);
        break;
    }
  }
}



static int
get_last_event_index (Event e){
  if(e.t == EVENT_ENDTURN)
    return(0);
  else if(e.t == EVENT_MOVE || e.t == EVENT_MELEE)
    return(steps-1);
  else if(e.t == EVENT_RANGE){
    Mcrd a = id2unit(e.range.a)->m;
    Mcrd b = id2unit(e.range.d)->m;
    return(mdist(a, b)*steps);
  }else
    return(0);
}



static void
events (){
  if(ui_mode == MODE_SHOW_EVENT)
    eindex++;
  if(ui_mode==MODE_SHOW_EVENT && eindex >= final_eindex){
    apply_event(e);
    ui_mode = MODE_SELECT;
  }
  if(ui_mode == MODE_SELECT && !is_eq_empty()){
    e = get_next_event();
    ui_mode = MODE_SHOW_EVENT;
    eindex = 0;
    final_eindex = get_last_event_index(e);
  }
}



static void
mainloop(){
  while(!done){
    if(cw->is_ai)
      ai();
    if(!is_local)
      do_network();
    sdl_events();
    events();
    draw();
    SDL_Delay(1*33);
  }
}



#undef main
int
main(int ac, char **av){
#if 1
  char * s[7] = {
      "./ui_sdl[.exe]",
      "-local",
      "scenario2",
      "-human",
      "0",
      "-human",
      "1" };
  av = s;
  ac = 7;
#endif
  init(ac, av);
  init_draw();
  mainloop();
  free_sprites();
  cleanup();
  return(EXIT_SUCCESS);
}

