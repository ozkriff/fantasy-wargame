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
#include "utype.h"
#include "net.h"
#include "core.h"
#include "misc.h"
#include "path.h"
#include "ai.h"

/* screen coordinates */
typedef Vec2i Scrd;

typedef SDL_Surface * Img;

static Img img_tiles[10];
static Img img_units[30];
static Img img_selected_hex;
static Img img_reacheble;
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


#define SCREEN_MENU     0 /*select scenario*/
#define SCREEN_SCENARIO 1
static int screen_id = SCREEN_MENU;

static Event e;    /* current [e]vent */

/* event progress? */
/* Internal index of event visualisation. */
static int eindex;

/*if eindex==final_eindex then event is shown.*/
static int final_eindex;
static int steps = 6;

static bool done;

static Scrd mouse_pos = {0, 0};

static Img
loadimg (char * str){
  Img original = IMG_Load(str);
  Img optimized;
  if(!original)
    die("ui_sdl: loadimg(): No file '%s'\n", str);
  optimized = SDL_DisplayFormatAlpha(original);
  SDL_FreeSurface(original);  
  return(optimized);
}

static void
free_sprites (void){
  SDL_FreeSurface(img_tiles[0]         );
  SDL_FreeSurface(img_tiles[1]         );
  SDL_FreeSurface(img_tiles[2]         );
  SDL_FreeSurface(img_tiles[3]         );
  SDL_FreeSurface(img_tiles[4]         );
  SDL_FreeSurface(img_arrow            );
  SDL_FreeSurface(img_selected_hex     );
  SDL_FreeSurface(img_reacheble        );
  SDL_FreeSurface(img_fog_of_war       );
  SDL_FreeSurface(img_rings[0]         );
  SDL_FreeSurface(img_rings[1]         );
  SDL_FreeSurface(img_rings[2]         );
  SDL_FreeSurface(img_units[U_PEASANT       ]);
  SDL_FreeSurface(img_units[U_SWORDSMAN     ]);
  SDL_FreeSurface(img_units[U_FOOT_KNIGHT   ]);
  SDL_FreeSurface(img_units[U_POOR_ARCHER   ]);
  SDL_FreeSurface(img_units[U_SCOUT         ]);
  SDL_FreeSurface(img_units[U_GOBLIN        ]);
  SDL_FreeSurface(img_units[U_GOBLIN_SLINGER]);
  SDL_FreeSurface(img_units[U_GOBLIN_SCOUT  ]);
  SDL_FreeSurface(img_units[U_ORC           ]);
  SDL_FreeSurface(img_units[U_ARMORED_ORC   ]);
  SDL_FreeSurface(img_units[U_CRAZY_ORC     ]);
  SDL_FreeSurface(img_units[U_TROLL         ]);
}

static void
load_sprites (void){
  char *peasant        = "img/human/peasant.png";
  char *swordsman      = "img/human/swordsman.png";
  char *foot_knight    = "img/human/foot_knight.png";
  char *poor_archer    = "img/human/poor_archer.png";
  char *scout          = "img/human/scout.png";
  char *goblin         = "img/goblin/normal.png";
  char *goblin_slinger = "img/goblin/slinger.png";
  char *goblin_scout   = "img/goblin/scout.png";
  char *orc            = "img/orc/normal.png";
  char *armored_orc    = "img/orc/armored.png";
  char *crazy_orc      = "img/orc/crazy.png";
  char *troll          = "img/troll/normal.png";
  img_units[U_PEASANT       ] = loadimg(peasant);
  img_units[U_SWORDSMAN     ] = loadimg(swordsman);
  img_units[U_FOOT_KNIGHT   ] = loadimg(foot_knight);
  img_units[U_POOR_ARCHER   ] = loadimg(poor_archer);
  img_units[U_SCOUT         ] = loadimg(scout);
  img_units[U_GOBLIN        ] = loadimg(goblin);
  img_units[U_GOBLIN_SLINGER] = loadimg(goblin_slinger);
  img_units[U_GOBLIN_SCOUT  ] = loadimg(goblin_scout);
  img_units[U_ORC           ] = loadimg(orc);
  img_units[U_ARMORED_ORC   ] = loadimg(armored_orc);
  img_units[U_CRAZY_ORC     ] = loadimg(crazy_orc);
  img_units[U_TROLL         ] = loadimg(troll);
  img_tiles[T_GRASS    ] = loadimg("img/grass.png"     );
  img_tiles[T_FOREST   ] = loadimg("img/tree.png"      );
  img_tiles[T_WATER    ] = loadimg("img/water.png"     );
  img_tiles[T_HILLS    ] = loadimg("img/hills.png"     );
  img_tiles[T_MOUNTEENS] = loadimg("img/mounteen.png"  );
  img_arrow              = loadimg("img/arrow.png"     );
  img_selected_hex       = loadimg("img/sel.png"       );
  img_reacheble          = loadimg("img/reacheble.png" );
  img_fog_of_war         = loadimg("img/fow.png"       );
  img_rings[0]           = loadimg("img/ring_red.png"  );
  img_rings[1]           = loadimg("img/ring_blue.png" );
  img_rings[2]           = loadimg("img/ring_green.png");
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
  double dx = (double)abs(b.x - a.x);
  double dy = (double)abs(b.y - a.y);
  return( (int)sqrt(pow(dx, 2.0)+pow(dy, 2.0)) );
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
    wp.x += 96/2;
    wp.y += 96/2;
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
  if(x >= 0 && y >= 0 && x < screen->w && y < screen->h)
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
draw_img (Img src, Scrd s) {
  SDL_Rect rect;
  rect.x = (Sint16)s.x;
  rect.y = (Sint16)s.y;
  SDL_BlitSurface(src, NULL, screen, &rect);
}

static void
draw_fog (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(tile(m)->fog <= 0)
      draw_img(img_fog_of_war, map2scr(m));
  }
}

static void
draw_map (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Img s = img_tiles[tile(m)->t];
    draw_img(s, map2scr(m));
  }
}

/* Draw line between 2 tiles. */
static void 
mbzline (Mcrd a, Mcrd b){
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
  SDL_Color fg = {255, 255, 255, 255};
  SDL_Color bg = {  0,   0,   0, 155};
  Img img = TTF_RenderText_Shaded(font, str, fg, bg);
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
    sprintf(str, "%i,%i", u->count, u->energy);
    text(str, mk_scrd(s.x+10, s.y+60), false);
  }
}

static void
draw_units (void){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(ui_mode == MODE_SHOW_EVENT){
      if(e.t==E_MOVE  && u->id == e.move.u)
        continue;
      if(e.t==E_MELEE && u->id == e.melee.a)
        continue;
    }
    if(u->is_visible){
      draw_unit(u);
    }
  }
}

/*Mark tiles that sulelected_unit can reach 
  during this turn.*/
static void
draw_reachable_tiles (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Mcrd p = tile(m)->parent;
    if(tile(m)->cost <= selected_unit->mvp) {
      draw_img(img_reacheble, map2scr(m));
      mbzline(m, p);
    }
  }
}

/*debugging*/
/*
static void
maptext (void){
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
draw_move_event (void){
  Unit * u = id2unit(e.move.u);
  Scrd s = mbetween(u->m, neib(u->m, e.move.dir), eindex);
  draw_img(img_rings[u->player], s);
  draw_img(type2img(u->t), s);
}

static void
draw_melee_event (void){
  Unit *a = id2unit(e.melee.a);
  Unit *d = id2unit(e.melee.d);
  int i = (eindex<steps/2) ? (eindex) : (steps-eindex);
  draw_img(img_rings[a->player], map2scr(a->m));
  draw_img(type2img(a->t), mbetween(a->m, d->m, i));
}

static void
draw_range_event (void){
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
draw_event (void){
  if(e.t==E_MOVE ) 
    draw_move_event ();
  if(e.t==E_MELEE) 
    draw_melee_event();
  if(e.t==E_RANGE) 
    draw_range_event();
}

static void
draw_labels (void){
  char str[100];
  sprintf(str, "[pl:%i]", cw->id);
  text(str, mk_scrd(0,0), false);
  if(inboard(selected_tile)){
    char *s = NULL;
    switch(tile(selected_tile)->t){
      case T_GRASS:     s = "(grass)";     break;
      case T_FOREST:    s = "(forest)";    break;
      case T_WATER:     s = "(water)";     break;
      case T_HILLS:     s = "(hills)";     break;
      case T_MOUNTEENS: s = "(mounteens)"; break;
      default:
        die("ui_sdl: draw_labels(): "
            "Unknown tile type - '%i'\n",
            tile(selected_tile)->t);
        break;
    }
    text(s, mk_scrd(0, 20), false);
  }
}

static void
draw (void){
  draw_bg(black);
  draw_map();
  draw_fog();
  if(ui_mode==MODE_SELECT && selected_unit)
    draw_reachable_tiles();
  if(selected_unit)
    draw_img(img_selected_hex, map2scr(selected_unit->m));
  draw_units();
  draw_img(img_selected_hex, map2scr(selected_tile));
  if(ui_mode==MODE_SHOW_EVENT)
    draw_event();
  /*maptext();*/
  draw_labels();
  SDL_Flip(screen);
}

static void
draw_menu (void){
  draw_bg(black);
  text("q - quit",       mk_scrd(0,  0), false);
  text("0 - scenario_0", mk_scrd(0, 20), false);
  text("1 - scenario_1", mk_scrd(0, 40), false);
  text("2 - net localhost 2000 human 0",
                         mk_scrd(0, 60), false);
  text("3 - net localhost 2000 human 1",
                         mk_scrd(0, 80), false);
  SDL_Flip(screen);
}

static void
init_colors (void){
  Uint8 x = 255;
  SDL_PixelFormat * fmt = screen->format;
  red   = SDL_MapRGBA(fmt, x, 0, 0, x);
  green = SDL_MapRGBA(fmt, 0, x, 0, x);
  blue  = SDL_MapRGBA(fmt, 0, 0, x, x);
  white = SDL_MapRGBA(fmt, x, x, x, x);
  black = SDL_MapRGBA(fmt, 0, 0, 0, x);
}

static void
init_draw (void){
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
tile_action (Mcrd m){
  Unit *u = unit_at(m);
  if(u && u->player == cw->id){
    selected_unit = u;
    fill_map(selected_unit);
  }else if(selected_unit){
    if(!u || (u && !u->is_visible)){
      move(selected_unit, m);
    }
    if(u && u->player != cw->id && u->is_visible
    && selected_unit && selected_unit->can_attack){
      attack(selected_unit, u);
    }
  }
}


static void
mouseclick (SDL_MouseButtonEvent e){
  Scrd s = mk_scrd((int)e.x, (int)e.y);
  tile_action(scr2map(s));
}

static void
mousemove (SDL_MouseMotionEvent e){
  mouse_pos = mk_scrd((int)e.x, (int)e.y);
  selected_tile = scr2map(mouse_pos);
}

static void
event_keys (SDL_KeyboardEvent e){
  switch(e.keysym.sym) {
    /* TODO finish event */
    default: break;
  }
}

static void
common_keys (SDL_KeyboardEvent e){
  switch(e.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q:
      done = true;
      break;
    case SDLK_UP:    map_offset.y += 2*96*3/4; break;
    case SDLK_DOWN:  map_offset.y -= 2*96*3/4; break;
    case SDLK_LEFT:  map_offset.x += 96;       break;
    case SDLK_RIGHT: map_offset.x -= 96;       break;
    default: break;
  }
}

static void
select_keys (SDL_KeyboardEvent e){
  switch(e.keysym.sym) {
    case SDLK_t:
      endturn();
      selected_unit = NULL;
      break;
    case SDLK_n:
      select_next_unit();
      break;
    case SDLK_h: selected_tile.x--; break;
    case SDLK_j: selected_tile.y++; break;
    case SDLK_k: selected_tile.y--; break;
    case SDLK_l: selected_tile.x++; break;
    case SDLK_v:
      tile_action(selected_tile);
      break;
    default: break;
  }
}

static void
menu_keys (SDL_KeyboardEvent e){
  switch(e.keysym.sym) {
    case SDLK_q:
      done = true;
      break;
    case SDLK_0:
      {
        int worlds[] = {0, 1};
        init_local_worlds(2, worlds);
        set_scenario_id(0);
        screen_id = SCREEN_SCENARIO;
      }
      break;
    case SDLK_1:
      init_local_worlds_s("hh", 0, 1);
      set_scenario_id(1);
      screen_id = SCREEN_SCENARIO;
      break;
    case SDLK_2:
      init_local_worlds_s("h", 0);
      init_net("localhost", 2000);
      screen_id = SCREEN_SCENARIO;
      break;
    case SDLK_3:
      init_local_worlds_s("h", 1);
      init_net("localhost", 2000);
      screen_id = SCREEN_SCENARIO;
      break;
    default:
      break;
  }
}

static void
keys (SDL_KeyboardEvent e){
  common_keys(e);
  if(is_client_active){
    if(ui_mode == MODE_SELECT)
      select_keys(e);
    else
      event_keys(e);
  }
}

static void
sdl_events (void){
  SDL_Event e;
  while(SDL_PollEvent(&e)){
    switch(e.type){
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYUP:
        if(screen_id == SCREEN_SCENARIO)
          keys(e.key);
        else
          menu_keys(e.key);
        break;
      case SDL_MOUSEMOTION:
        mousemove(e.motion);
        break;
      case SDL_MOUSEBUTTONDOWN:
        if(is_client_active
        && ui_mode == MODE_SELECT
        && screen_id == SCREEN_SCENARIO){
          mouseclick(e.button);
        }
        break;
      case SDL_VIDEORESIZE:
        screen = SDL_SetVideoMode(e.resize.w, e.resize.h,
            32, SDL_SWSURFACE | SDL_RESIZABLE);
        break;
    }
  }
}

static int
get_last_event_index (Event e){
  int n;
  if(e.t == E_ENDTURN || e.t == E_DEATH)
    n = 0;
  else if(e.t == E_MOVE || e.t == E_MELEE)
    n = steps-1;
  else if(e.t == E_RANGE){
    Mcrd a = id2unit(e.range.a)->m;
    Mcrd b = id2unit(e.range.d)->m;
    n = mdist(a, b)*steps;
  }else{
    die("ui_sdl: get_last_event_index(): "
        "unknow event '%i'.\n", e.t);
  }
  return(n);
}

static void
events (void){
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
scroll_map(Scrd s){
  int step = 10; /*pixels per frame*/
  int o = 15; /*offset*/
  if(s.x < o)
    map_offset.x += step;
  if(s.y < o)
    map_offset.y += step;
  if(s.x > screen->w - o)
    map_offset.x -= step;
  if(s.y > screen->h - o)
    map_offset.y -= step;
}

static void
mainloop (void){
  while(!done){
    sdl_events();
    if(screen_id == SCREEN_SCENARIO){
      if(cw->is_ai)
        ai();
      if(!is_local)
        do_network();
      events();
      scroll_map(mouse_pos);
      draw();
    }else{
      draw_menu();
    }
    SDL_Delay(1*33);
  }
}

#undef main
int
main (void){
  init();
  init_draw();
  mainloop();
  free_sprites();
  cleanup();
  return(EXIT_SUCCESS);
}

