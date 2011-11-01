/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_net.h"
#include "bool.h"
#include "list.h"
#include "utype.h"
#include "core.h"
#include "net.h"
#include "misc.h"
#include "path.h"
#include "ai.h"

/* screen coordinates */
typedef Vec2i Scrd;

typedef struct {
  SDL_Surface *bitmap;
  int w, h;
} Font;

typedef struct {
  int frames_left;
  Mcrd pos;
  SDL_Surface *img;
} Label;

static List labels = {NULL, NULL, 0};

static Font font;

static SDL_Surface *img_tiles[10];
static SDL_Surface *img_units[30];
static SDL_Surface *img_selected_hex;
static SDL_Surface *img_reacheble;
static SDL_Surface *img_fog_of_war;
static SDL_Surface *img_arrow;
static SDL_Surface *screen = NULL;

static SDL_Surface *img_rings[10];

static Vec2i map_offset = {72, 72/4};

static Uint32 red, black, green, blue, white;

/* mouse points to this hex */
static Mcrd selected_tile;

#define MODE_SELECT     0
#define MODE_SHOW_EVENT 1
static int ui_mode = MODE_SELECT;


#define SCREEN_MENU     0 /*select scenario*/
#define SCREEN_SCENARIO 1
static int screen_id = SCREEN_MENU;

static Event *current_event;

/* event progress? */
/* Internal index of event visualisation. */
static int eindex;

/*if eindex==final_eindex then event is shown.*/
static int final_eindex;
static int steps = 6;

static bool done;
static bool is_dirty = true;

static Scrd mouse_pos = {0, 0};

static SDL_Surface *
loadimg (char * str){
  SDL_Surface *original = IMG_Load(str);
  SDL_Surface *optimized;
  if(!original)
    die("ui_sdl: loadimg(): No file '%s'\n", str);
  optimized = SDL_DisplayFormatAlpha(original);
  SDL_FreeSurface(original);  
  return(optimized);
}

static void
free_sprites (void){
  SDL_FreeSurface(font.bitmap          );
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
  img_tiles[T_MOUNTAINS] = loadimg("img/mounteen.png"  );
  img_arrow              = loadimg("img/arrow.png"     );
  img_selected_hex       = loadimg("img/sel.png"       );
  img_reacheble          = loadimg("img/reacheble.png" );
  img_fog_of_war         = loadimg("img/fog.png"       );
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
  float dx = (float)abs(b.x - a.x);
  float dy = (float)abs(b.y - a.y);
  return( (int)sqrt(pow(dx, 2.0)+pow(dy, 2.0)) );
}

static Scrd
map2scr (Mcrd map) {
  Scrd scr;
  scr.y = map_offset.y + map.y*(96*3/4) + 96/2;
  scr.x = map_offset.x + map.x*(96)     + 92/2;
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
draw_img (SDL_Surface *src, Scrd s) {
  SDL_Rect rect;
  rect.x = (Sint16)s.x - src->w/2;
  rect.y = (Sint16)s.y - src->h/2;
  SDL_BlitSurface(src, NULL, screen, &rect);
}

static void
draw_fog (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(!tile(m)->visible)
      draw_img(img_fog_of_war, map2scr(m));
  }
}

static void
draw_map (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    SDL_Surface *s = img_tiles[tile(m)->t];
    draw_img(s, map2scr(m));
  }
}

/* Draw line between 2 tiles. */
static void 
mbzline (Mcrd a, Mcrd b){
  Scrd sa = map2scr(a);
  Scrd sb = map2scr(b);
  bzline(sa, sb, red);
}

static void
draw_bg (Uint32 clr){
  SDL_FillRect(screen, NULL, clr);
}

static Font
build_font (SDL_Surface *surface, int w, int h){
  Font font;
  font.bitmap = surface;
  font.w = w;
  font.h = h;
  return(font);
}

static Scrd
get_rendered_size (Font *font, char *s){
  Scrd size;
  int y = font->h;
  int x = 0;
  int max_x = 0;
  int i;
  size.y = font->h;
  for(i = 0; s[i] != '\0'; i++){
    if(s[i] == '\n'){
      y += font->h; 
      x = 0;
    }else{
      x += font->w;
      if(x > max_x)
        max_x = x;
    }
  }
  size.x = max_x;
  size.y = y;
  return(size);
}

/*Go through the text.
  If meet ' ' then move over.
  If meet '\n' then move down and move back.
  If meet normal character then show the character
  and move over the width of the character*/
static void
render_text (SDL_Surface *dest, Font *font, char *s, Scrd pos){
  Scrd cursor = pos;
  int i;
  if(font->bitmap == NULL)
    return;
  for(i = 0; s[i] != '\0'; i++){
    if(s[i] == ' '){
      cursor.x += font->w; 
    }else if(s[i] == '\n'){
      cursor.y += font->h; 
      cursor.x = pos.x; 
    }else{
      int n = s[i] - 32;
      SDL_Rect clip, offset;
      clip.x = (Sint16)font->w * (n % 16);
      clip.y = (Sint16)font->h * (n / 16);
      clip.w = (Sint16)font->w;
      clip.h = (Sint16)font->h;
      offset.x = (Sint16)cursor.x;
      offset.y = (Sint16)cursor.y;
      SDL_BlitSurface(font->bitmap, &clip, dest, &offset);
      cursor.x += font->w;
    }
  }
}

static void
text (Font *font, char *s, Scrd pos){
  render_text(screen, font, s, pos);
}

static SDL_Surface *
create_text (Font *f, char *s){
  Scrd pos = {0, 0};
  Scrd size = get_rendered_size(f, s);
  SDL_PixelFormat* fmt = screen->format;
  SDL_Surface *srf = SDL_CreateRGBSurface(0,
      size.x, size.y, (int)fmt->BitsPerPixel, 0, 0, 0, 0);
  Uint32 color = SDL_MapRGBA(srf->format, 0, 255, 255, 255);
  SDL_FillRect(srf, NULL, color);
  SDL_SetColorKey(srf, SDL_SRCCOLORKEY, color); 
  render_text(srf, f, s, pos);
  return(srf);
}

static SDL_Surface *
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
    text(&font, str, mk_scrd(s.x, s.y+20));
  }
}

static void
draw_units (void){
  Node * node;
  FOR_EACH_NODE(units, node){
    Unit * u = node->d;
    if(ui_mode == MODE_SHOW_EVENT){
      if(current_event->t == E_MOVE
      && current_event->e.move.u == u->id)
        continue;
      if(current_event->t == E_MELEE
      && current_event->e.melee.a == u->id)
        continue;
    }
    if(u->is_visible){
      draw_unit(u);
    }
  }
}

/*Mark tiles that selected_unit can reach 
  during this turn.*/
static void
draw_reachable_tiles (void){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Mcrd p = tile(m)->parent;
    if(tile(m)->cost <= selected_unit->mv) {
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
      text(str, s);
    }
  }
}
*/

static void
draw_move_event (Event_move e){
  Unit * u = id2unit(e.u);
  Scrd s = mbetween(u->m, neib(u->m, e.dir), eindex);
  draw_img(img_rings[u->player], s);
  draw_img(type2img(u->t), s);
}

static void
draw_melee_event (Event_melee e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  int i = (eindex<steps/2) ? (eindex) : (steps-eindex);
  draw_img(img_rings[a->player], map2scr(a->m));
  draw_img(type2img(a->t), mbetween(a->m, d->m, i));
}

/*get vertical correction.
  used in draw_range_event()
  dist       - distance (in pixels)
  step       - current step (index)
  final_step - total steps count
*/
static int
get_vc (int dist, int step, int final_step){
  float n = sin( step * 3.14 / final_step );
  float vc = n * dist * 28;
  return((int)vc);
}

static void
draw_range_event (Event_range e){
  Unit *a   = id2unit(e.a);
  Unit *d   = id2unit(e.d);
  Scrd sa   = map2scr(a->m);
  Scrd sd   = map2scr(d->m);
  int dist  = mdist(a->m, d->m);
  int xstep = (sd.x-sa.x)/final_eindex;
  int ystep = (sd.y-sa.y)/final_eindex;
  Scrd s    = sa;
  Scrd new_s;
  int i;
  for(i=0; i<eindex; i++, s=new_s){
    int vc_old  = get_vc(dist, i, final_eindex);
    int vc_new  = get_vc(dist, i+1, final_eindex);
    int vc_diff = vc_new - vc_old;
    new_s       = mk_scrd(s.x+xstep, s.y+ystep-vc_diff);
    bzline(s, new_s, white);
  }
  draw_img(img_arrow, s);
}

static void
draw_event (Event e){
  switch(e.t){
    case E_MOVE:
      draw_move_event(e.e.move);
      break;
    case E_MELEE:
      draw_melee_event(e.e.melee);
      break;
    case E_RANGE:
      draw_range_event(e.e.range);
      break;
    case E_ENDTURN:
      break;
    case E_DEATH:
      break;
    default:
      die("ui_sdl: draw_event(): "
          "Unknown event type: '%i'\n", e.t);
      break;
  }
}

static char *
tiletype2name (int tiletype){
  switch(tiletype){
    case T_GRASS:     return("(grass)");
    case T_FOREST:    return("(forest)");
    case T_WATER:     return("(water)");
    case T_HILLS:     return("(hills)");
    case T_MOUNTAINS: return("(mounteens)");
    default:
      die("ui_sdl: tiletype2name(): "
          "Unknown tile type - '%i'\n",
          tiletype);
      break;
  }
  return(NULL);
}
 
static void
draw_label (Label *l){
  Scrd pos = map2scr(l->pos);
  pos.y -= 4 * (60 - l->frames_left);
  draw_img(l->img, pos);
}

static void
draw_labels_2 (void){
  Node *node = labels.h;
  while(node){
    Label *l = node->d;
    draw_label(l);
    l->frames_left--;
    /*delete old nodes*/
    if(l->frames_left == 0){
      Node *prev = node->p;
      SDL_FreeSurface(l->img);
      delete_node(&labels, node);
      node = prev;
    }
    if(node)
      node = node->n;
  }
}

static void
draw_labels (void){
  char str[100];
  sprintf(str, "[pl:%i]", current_player->id);
  text(&font, str, mk_scrd(0, 0));
  if(inboard(selected_tile)){
    int t = tile(selected_tile)->t;
    char *s = tiletype2name(t);
    text(&font, s, mk_scrd(0, 20));
  }
  draw_labels_2();
}

static char*
unittype2name (Unit_type_id t){
  switch(t){
    /*humans*/
    case U_PEASANT:       return("peasant");
    case U_MILITIAMAN:    return("militiaman");
    case U_SPEARMAN:      return("spearman");
    case U_SWORDSMAN:     return("swordsman");
    case U_FOOT_KNIGHT:   return("foot knight");
    case U_KNIGHT:        return("knight");
    case U_POOR_ARCHER:   return("poor_archer");
    case U_ARCHER:        return("archer");
    case U_SCOUT:         return("scout");
    /*goblins*/
    case U_WEAK_GOBLIN:   return("weak goblin");
    case U_GOBLIN:        return("goblin");
    case U_GOBLIN_SLINGER:return("goblin slinger");
    case U_GOBLIN_SCOUT:  return("goblin scout");
    /*orcs*/
    case U_YOUNG_ORC:     return("young orc");
    case U_ORC:           return("orc");
    case U_ARMORED_ORC:   return("armored orc");
    case U_CRAZY_ORC:     return("crazy orc");
    /*trolls*/
    case U_YOUNG_TROLL:   return("young orc");
    case U_TROLL:         return("troll");
    default:
      die("ui_sdl: unittype2name(): "
          "Unknown unit type id: '%i'\n", t);
      break;
  }
  return(NULL);

}

static void
draw_unit_info (Unit *u, Scrd s){
  Unit_type *t = &utypes[u->t];
  char str[12 * 30];
  char *template =
      " type_id   %2i \n"
      " v         %2i \n"
      " morale    %2i \n"
      " morale_rg %2i \n"
      " ms        %2i \n"
      " strength  %2i \n"
      " toughness %2i \n"
      " attacks   %2i \n"
      " armor     %2i \n"
      " mv        %2i \n"
      " %s \n"
      " id        %2i \n";
  sprintf(str, template,
      u->t,
      t->v,
      t->morale,
      t->morale_rg,
      t->ms,
      t->strength,
      t->toughness,
      t->attacks,
      t->armor,
      t->mv,
      unittype2name(u->t),
      u->id);
  text(&font, str, s);
}

static void
draw (void){
  draw_bg(black);
  draw_map();
  draw_fog();
  draw_units();
  if(ui_mode==MODE_SELECT && selected_unit){
    draw_reachable_tiles();
    draw_img(img_selected_hex, map2scr(selected_unit->m));
  }
  draw_img(img_selected_hex, map2scr(selected_tile));
  if(ui_mode==MODE_SHOW_EVENT)
    draw_event(*current_event);
  /*maptext();*/
  if(ui_mode==MODE_SELECT
  && unit_at(selected_tile)
  && unit_at(selected_tile)->is_visible) {
    draw_unit_info(unit_at(selected_tile), mk_scrd(5, 40));
  }
  draw_labels();
  SDL_Flip(screen);
}

static void
draw_menu (void){
  char *str = 
      "q - quit\n"
      "0 - scenario_0\n"
      "1 - scenario_1\n"
      "2 - net localhost 2000 human 0\n"
      "3 - net localhost 2000 human 1\n";
  draw_bg(black);
  text(&font, str, mk_scrd(10, 10));
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
add_label (char *str, Mcrd pos){
  Label *l = calloc(1, sizeof(Label));
  l->frames_left = 60;
  l->pos = pos;
  l->img = create_text(&font, str);
  push_node(&labels, l);
}

static void
init_draw (void){
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  screen = SDL_SetVideoMode(640, 480,
      32, SDL_SWSURFACE | SDL_RESIZABLE);
  font = build_font(loadimg("img/font_8x12.png"), 8, 12);
  load_sprites();
  init_colors();
  selected_tile = mk_mcrd(-1,-1);
}

static void
tile_action (Mcrd m){
  Unit *u = unit_at(m);
  if(u && u->player == current_player->id){
    selected_unit = u;
    fill_map(selected_unit);
  }else if(selected_unit){
    if(!u || (u && !u->is_visible)){
      move(selected_unit, m);
    }
    if(u && u->player != current_player->id && u->is_visible
    && selected_unit){
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
      if(screen_id == SCREEN_MENU)
        done = true;
      else{
        selected_unit = NULL;
        selected_tile = mk_mcrd(0, 0);
        cleanup();
        screen_id = SCREEN_MENU;
      }
      break;
    case SDLK_UP:    map_offset.y += 2*96*3/4; break;
    case SDLK_DOWN:  map_offset.y -= 2*96*3/4; break;
    case SDLK_LEFT:  map_offset.x += 2*96;     break;
    case SDLK_RIGHT: map_offset.x -= 2*96;     break;
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
        init_local_players(2, worlds);
        set_scenario_id(0);
        screen_id = SCREEN_SCENARIO;
      }
      break;
    case SDLK_1:
      init_local_players_s("hh", 0, 1);
      set_scenario_id(1);
      screen_id = SCREEN_SCENARIO;
      break;
    case SDLK_2:
      init_local_players_s("h", 0);
      init_net("localhost", 2000);
      screen_id = SCREEN_SCENARIO;
      break;
    case SDLK_3:
      init_local_players_s("h", 1);
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
    is_dirty = true;
  }
}

static int
get_last_event_index (Event e){
  int n;
  if(e.t == E_ENDTURN || e.t == E_DEATH)
    n = 0;
  else if(e.t == E_MOVE || e.t == E_MELEE)
    n = steps;
  else if(e.t == E_RANGE){
    Mcrd a = id2unit(e.e.range.a)->m;
    Mcrd b = id2unit(e.e.range.d)->m;
    n = mdist(a, b)*steps;
  }else{
    die("ui_sdl: get_last_event_index(): "
        "unknow event '%i'.\n", e.t);
  }
  return(n);
}

static void
add_event_melee_label (Event_melee e){
  char str_a[20];
  char str_d[20];
  sprintf(str_a, "-%i", e.attackers_killed);
  add_label(str_a, id2unit(e.a)->m);
  sprintf(str_d, "-%i", e.defenders_killed);
  add_label(str_d, id2unit(e.d)->m);
}

static void
add_event_range_label (Event_range e){
  char str[10];
  sprintf(str, "-%i", e.defenders_killed);
  add_label(str, id2unit(e.d)->m);
}

static void
events (void){
  if(ui_mode == MODE_SHOW_EVENT){
    eindex++;
    is_dirty = true;
  }
  if(ui_mode==MODE_SHOW_EVENT && eindex >= final_eindex){
    apply_event(*current_event);
    ui_mode = MODE_SELECT;
    is_dirty = true;
  }
  if(ui_mode == MODE_SELECT && unshown_events_left()){
    current_event = get_next_event();
    ui_mode = MODE_SHOW_EVENT;
    eindex = 0;
    final_eindex = get_last_event_index(*current_event);
    {
      if(current_event->t == E_MELEE){
        add_event_melee_label(current_event->e.melee);
      }else if(current_event->t == E_RANGE){
        add_event_range_label(current_event->e.range);
      }
    }
    is_dirty = true;
  }
}

static void
correct_map_offset (){
  int left   = 96/2;
  int right  = screen->w - map_size.x*96;
  int top    = 0;
  int bottom = screen->h - map_size.y*96*3/4 - 96/4;
  if(map_offset.x > left)   map_offset.x = left;
  if(map_offset.x < right)  map_offset.x = right;
  if(map_offset.y > top)    map_offset.y = top;
  if(map_offset.y < bottom) map_offset.y = bottom;
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
  correct_map_offset();
  is_dirty = true;
}

static void
mainloop (void){
  while(!done){
    sdl_events();
    if(screen_id == SCREEN_SCENARIO){
      if(current_player->is_ai)
        ai();
      if(!is_local)
        do_network();
      events();
      scroll_map(mouse_pos);
      if(is_dirty){
        draw();
        is_dirty = false;
      }
    }else{
      draw_menu();
      is_dirty = false;
    }
    SDL_Delay(1*33);
  }
}

static void
cleanup_ui(){
  free_sprites();
  SDL_Quit();
  IMG_Quit();
}

#undef main
int
main (void){
  init();
  init_draw();
  mainloop();
  cleanup_ui();
  return(EXIT_SUCCESS);
}

