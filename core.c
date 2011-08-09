
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "list.h"
#include "structs.h"
#include "path.h"
#include "misc.h"
#include "core.h"
#include "core_private.h"
#include "net.h"

static bool checkunitsleft ();

static void kill_unit (Unit * u);

static void update_fog_after_move (Unit * u);
static void updatefog (int player);

static int  melee_attack_damage (Unit * a, Unit * b);
static int  melee_return_damage (Unit * a, Unit * b);
static int         range_damage (Unit * a, Unit * b);

static void apply_move  (Event e);
static void apply_melee (Event e);
static void apply_range (Event e);

static void support_range (Unit * a, Unit * d);
static void attack_melee (Unit * a, Unit * d);

static bool  is_move_visible (Event e);
static bool is_melee_visible (Event e);
static bool is_range_visible (Event e);
static bool is_event_visible (Event e);

static void update_units_visibility();

/* initialization */
static void read_config(char * filename);
static void create_local_world (int id, bool is_ai);
static void create_local_human (int id);
static void create_local_ai    (int id);
static void local_arguments (int ac, char ** av);
static void   net_arguments (int ac, char ** av);

static Event mk_event_move  (Unit * u, Mcrd dest);
static Event mk_event_melee (Unit * a, Unit * d, int dmg);
static Event mk_event_range (Unit * a, Unit * d, int dmg);

Feature mk_feature_range (int power, int range);
Feature mk_feature_berserk (int power);
Feature mk_feature_bool (int type);

static bool   is_invis (Unit * u);
static Node * unit2node (Unit * u);

static void  add_feature (Unit * u, Feature f);
static void  add_default_features_to_unit (Unit * u);
static void  add_unit (Mcrd crd, int plr, Unit_type * type, World * wrld);




/* grass forest water hills mount */

/*terrain movepoints */
static int tmvp1[5] = {  1, 4, 8, 4, 9 };
static int tmvp2[5] = {  1, 2, 4, 3, 6 };
static int tmvp3[5] = {  1, 3, 8, 4, 9 };

/*terrain defence bonus */
static int tdef1[5] = {  0, 3,-4, 4, 3 };
static int tdef2[5] = {  0, 5, 0, 4, 4 };
static int tdef3[5] = {  0, 5, 0, 4, 4 };

/*terrain attack bonus */
static int tatk1[5] = {  0, 0,-4, 0, 0 };
static int tatk2[5] = {  0, 3,-4, 0, 0 };
static int tatk3[5] = {  0, 1,-4, 0, 0 };


/* GLOBAL VARIABLES */
Unit_type utypes[3] = {
  {1,5,10,5,6,3,"defc", tmvp1, tdef1, tatk1},
  {4,5,10,5,4,4,"hunt", tmvp2, tdef2, tatk2},
  {3,5,10,4,3,3,"arch", tmvp3, tdef3, tatk3} };
Mcrd    map_size;
List    worlds;
World * cw = NULL; /* current world */
bool    is_local;
Unit *  selunit = NULL; /* selected unit */



/* Find unit's node in cw->units list. */
static Node *
unit2node (Unit * u){
  Node * n = cw->units.h;
  while(u != (Unit*)n->d)
    n = n->n;
  return n;
}



static void
kill_unit (Unit * u){
  if(u == selunit)
    selunit = NULL;

  /* Delete fatures list. */
  while(u->features.count > 0)
    l_delete_node(&u->features, u->features.h);

  /* Find unit's node, free unit and node. */
  l_delete_node(&cw->units, unit2node(u));
}



static void
update_fog_after_move (Unit * u){
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(mdist(m, u->mcrd) <= u->type->see)
      tile(m)->fog++;
  }
}



static void
apply_move (Event e){
  Unit * u = id2unit(e.move.u);
  u->mvp -= e.move.cost;
  u->mcrd = e.move.dest;
  if(selunit==u){
    fill_map(selunit);
  }
  if(u->player==cw->id)
    update_fog_after_move(u);
}


/* dmg = E(attack_skill) / E(defence_skill) * HPi/HPo * K */

/* a - attacking unit, b - defender */
static int
melee_attack_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[tile(b->mcrd)->type];
  int terrain_defence = b->type->ter_def[tile(b->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;

  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(4 * attack/defence);
}



/* a - defender, a - attacking unit */
static int
melee_return_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[tile(a->mcrd)->type];
  int terrain_defence = b->type->ter_def[tile(a->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;

  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(3 * attack/defence);
}



/* a - shooting unit, a - target */
static int
range_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[tile(a->mcrd)->type];
  int terrain_defence = b->type->ter_def[tile(b->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;

  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(3 * attack/defence);
}



static void
apply_melee(Event e){
  int damage = e.melee.dmg;
  Unit * u1 = id2unit(e.melee.a);
  Unit * u2 = id2unit(e.melee.d);
  u2->health -= damage;
  if(u2->health <= 0) {
    kill_unit(u2);
    fill_map(u1);
  }
  u1->can_attack = false;
}



static void
apply_range (Event e){
  Unit * ua = id2unit(e.range.a);
  Unit * ud = id2unit(e.range.d);
  int dmg = e.range.dmg;
  ud->health -= dmg;
  if(ud->health <= 0){
    kill_unit(ud);
    fill_map(ua);
  }
  ua->can_attack = false;
}



static void
updatefog (int player){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Node * node;
    tile(m)->fog=0;
    FOR_EACH_NODE(cw->units, node){
      Unit * u = node->d;
      if(u->player == player
      && mdist(m, u->mcrd) <= u->type->see){
        tile(m)->fog++;
      }
    }
  }
}



static bool
is_move_visible (Event e){
  Unit * u = id2unit(e.move.u);
  bool fow = tile(e.move.dest)->fog || tile(u->mcrd)->fog;
  bool hidden = is_invis(u) && u->player!=cw->id;
  return(!hidden && fow);
}



static bool
is_melee_visible (Event e){
  Unit * u = id2unit( e.melee.a );
  return(tile(u->mcrd)->fog || tile(e.melee.md)->fog);
}



static bool
is_range_visible (Event e){
  Unit * u = id2unit( e.range.a );
  return(tile(u->mcrd)->fog || tile(e.range.md)->fog);
}



/* called before get_next_event */
void
update_eq (){
  Event * e;
  while(cw->eq.count > 0){
    e = cw->eq.h->d;
    if(!is_event_visible(*e)){
      e = l_dequeue(&cw->eq);
      apply_event(*e);
      free(e);
    }else{
      return;
    }
  }
}



/* always called after update_eq */
Event
get_next_event (){
  Event * tmp = l_dequeue(&cw->eq);
  Event e = *tmp;
  free(tmp);
  return(e);
}



static bool
checkunitsleft(){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id)
      return(true);
  }
  return(false);
}



/* change |c|urrent |w|orld */
static void
change_cw (){
  Node * nd;
  FOR_EACH_NODE(worlds, nd){
    if(cw == (World*)nd->d){
      if(nd->n)
        cw = nd->n->d;
      else
        cw = worlds.h->d;
      return;
    }
  }
}



static void
refresh_units (){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id){
      u->mvp = u->type->mvp;
      u->can_attack = true;
    }
  }
}



static void
check_win (){
  if(!checkunitsleft()){
    /* TODO generate special event. */
    puts("WINNER!");
    exit(EXIT_SUCCESS);
  }
}



void
endturn (){
  change_cw();
  check_win();
  refresh_units();
  updatefog(cw->id);
  update_units_visibility();
}



static Event
mk_event_move (Unit * u, Mcrd dest){
  Event e;
  e.move.t    = EVENT_MOVE;
  e.move.u    = u->id;
  e.move.dest = dest;
  e.move.cost = u->type->ter_mvp[tile(dest)->type];
  return(e);
}



static Event
mk_event_melee (Unit * a, Unit * d, int dmg){
  Event e;
  e.melee.t   = EVENT_MELEE;
  e.melee.a   = a->id;
  e.melee.d   = d->id;
  e.melee.md  = d->mcrd;
  e.melee.dmg = dmg;
  return(e);
}



static Event
mk_event_range (Unit * a, Unit * d, int dmg){
  Event e;
  e.range.t   = EVENT_RANGE;
  e.range.a   = a->id;
  e.range.d   = d->id;
  e.range.md  = d->mcrd;
  e.range.dmg = dmg;
  return(e);
}



static bool
ambush(Mcrd next, Mcrd current, Unit * moving_unit){
  Unit * u = find_unit_at(next);
  if(u && u->player != moving_unit->player){
#if 1
    Event e;
    e = mk_event_melee(u, moving_unit, 1);
    e.melee.md = current;
    add_event(e);
#else
    add_event( mk_event_melee(u, moving_unit, current, 1) );
#endif
    return(true);
  }else{
    return(false);
  }
}



void
move (Unit * moving_unit, Mcrd destination){
  List path = get_path(destination);
  Node * node;
  for(node=path.h; node->n; node=node->n){
    Mcrd * current = node->d;
    Mcrd * next    = node->n->d;
    if(ambush(*next, *current, moving_unit))
      break;
    add_event( mk_event_move(moving_unit, (*next)) );
  }
  while(path.count > 0)
    l_delete_node(&path, path.h);
}



/* [a]ttacker, [d]efender */
static void
support_range (Unit * a, Unit * d){
  int i;
  for(i=0; i<6; i++){
    /* [sup]porter */
    Unit * sup = find_unit_at( neib(d->mcrd, i) );
    if( sup && sup->player == d->player ){
      if(find_feature(sup, FEATURE_RNG)){
        Event range = mk_event_range(sup, a, 2);
        add_event( range );
        if(a->health - range.range.dmg <= 0)
          return;
      }
    }
  }
}



/* [a]ttacker, [d]efender */
/* TODO rewrite */
static void
attack_melee (Unit * a, Unit * d){
  Event melee;
  Mcrd md = d->mcrd;

  support_range(a, d);

  melee = mk_event_melee(a, d, melee_attack_damage(a, d));
  add_event( melee );

  /* check if opponent is still alive */
  if(d->health - melee.melee.dmg <= 0)
    return;

  /* counterattack */
  add_event( mk_event_melee(d, a, melee_return_damage(d, a)) );

  /* CHeck if unit will flee or panic */
  if(d->health - melee.melee.dmg > d->type->health / 2)
    return;

  /* try to flee in opposite direction or fight(panic) */
  {
    int dir3; /* direction */
    for(dir3=0; dir3<6; dir3++){
      if( ! find_unit_at(neib(md, dir3)) )
        break;
    }
    if(dir3==6){
      attack_melee(d, a); /* panic! */
    }else{
      add_event( mk_event_move(d, neib(md, dir3)) );
    }
  }
}



/* [a]ttacker, [d]efender */
void attack (Unit * a, Unit * d){
  Mcrd md = d->mcrd;
  Mcrd ma = a->mcrd;
  Feature * rng = find_feature(a, FEATURE_RNG);
  if(rng){
    if(mdist(ma, md) <= rng->rng.range){
      add_event( mk_event_range(a, d, range_damage(a, d)) );
    }
  }else{
    if(mdist(ma, md) <= 1){
      attack_melee(a, d);
    }
  }
}



/* Check and update visibility of all units in CW */
static void
update_units_visibility (){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id){
      u->visible = true;
    }else{
      if(tile(u->mcrd)->fog > 0 && !is_invis(u))
        u->visible = true;
      else
        u->visible = false;
    }
  }
}



static bool
is_invis (Unit * u){
  int i;
  if(!find_feature(u, FEATURE_INVIS)
  || u->player == cw->id)
    return(false);
  for(i=0; i<6; i++){
    Mcrd nb = neib(u->mcrd, i);
    Unit * u2 = find_unit_at(nb);
    if(u2 && u2->player == cw->id)
      return(false);
  }
  return(true);
}



static void
add_feature (Unit * u, Feature f){
  Feature * new_f = malloc(sizeof(Feature));
  *new_f = f;
  l_push(&u->features, new_f);
}



static void
add_default_features_to_unit (Unit * u){
  if(u->type == &utypes[2]){ /* archer */
    add_feature(u, mk_feature_range(5, 4));
  }
  if(u->type == &utypes[1]) { /* hunter */
    add_feature(u, mk_feature_bool(FEATURE_IGNR    ));
    add_feature(u, mk_feature_bool(FEATURE_INVIS   ));
    add_feature(u, mk_feature_bool(FEATURE_NORETURN));
  }
}



static void
read_config(char * filename){
  FILE * cfg = fopen(filename, "r");
  char s[100]; /* buffer */
  Node * node;

  while( fgets(s, 90, cfg) ){
    if(s[0]=='#' || s[0]=='\n')
      continue;
    if(!strncmp("[MAP-SIZE]", s, 10)){
      sscanf(s, "[MAP-SIZE] %i %i",
          &map_size.x, &map_size.y);
      FOR_EACH_NODE(worlds, node){
        World * w = node->d;
        w->map = calloc(map_size.x*map_size.y, sizeof(Tile));
      }
    }
    if(!strncmp("[UNIT]", s, 6)){
      int plr, x, y, type;
      sscanf(s, "[UNIT] %i %i %i %i",
          &plr, &x, &y, &type);
      FOR_EACH_NODE(worlds, node){
        add_unit(mk_mcrd(x,y), plr, &utypes[type], node->d);
      }
    }
    if(!strncmp("[MAP]", s, 5)){
      int mi = 0; /* Index in 'map' array. */
      while( fgets(s, 90, cfg) && s[0]!='\n'){
        int i;
        for(i=0; s[i]; i++){
          char c = s[i];
          if(c != ' ' && c != '\n'){
            FOR_EACH_NODE(worlds, node){
              World * w = node->d;
              Tile * t = w->map + mi;
              if(c=='.') t->type = 0;
              if(c=='t') t->type = 1;
              if(c=='*') t->type = 2;
              if(c=='h') t->type = 3;
              if(c=='M') t->type = 4;
            }
            mi++;
          }
        }
      }
    }
  }

  fclose(cfg);
}



static void
create_local_world (int id, bool is_ai) {
  World * w = calloc(1, sizeof(World));
  w->id     = id;
  w->is_ai  = is_ai;
  l_push(&worlds, w);
}



static void
create_local_human (int id) {
  create_local_world(id, false);
}



static void
create_local_ai (int id) {
  create_local_world(id, true);
}



static void
local_arguments (int ac, char ** av)
{
  /*0-program_name, 1-"-local", 2-scenario_name*/
  int i;
  for(i=3; i<ac; i++){
    if(!strcmp(av[i], "-ai")){
      int id = str2int(av[i+1]);
      create_local_ai(id);
    }
    if(!strcmp(av[i], "-human")){
      int id = str2int(av[i+1]);
      create_local_human(id);
    }
  }
  read_config(av[2]);
  is_local = true;
}



static void
net_arguments (int ac, char ** av){
  int no_players_left_mark = 0xff;
  char scenarioname[100];
  int i;

  int port = str2int(av[3]);
  init_network(av[2], port);

  /* 0-program_name, 1-"-net", 2-"server", 3-port */
  for(i=4; i<ac; i++){
    if(!strcmp(av[i], "-ai")){
      int id = str2int(av[i+1]);
      create_local_ai(id);
      send_int_as_uint8(id);
    }
    if(!strcmp(av[i], "-human")){
      int id = str2int(av[i+1]);
      create_local_human(id);
      send_int_as_uint8(id);
    }
  }

  send_int_as_uint8(no_players_left_mark);

  get_scenario_name_from_server(scenarioname);
  read_config(scenarioname);
  is_local = false;
}



void
init (int ac, char ** av){
  srand(time(NULL));
  if(!strcmp(av[1], "-local"))
    local_arguments(ac, av);
  if(!strcmp(av[1], "-net"))
    net_arguments(ac, av);
  cw = worlds.h->d;
  updatefog(cw->id);
  update_units_visibility();
}



void
apply_event (Event e){
  switch(e.t){
    case EVENT_MOVE : apply_move(e);  break;
    case EVENT_MELEE: apply_melee(e); break;
    case EVENT_RANGE: apply_range(e); break;
  }
  update_units_visibility();
}



void
add_unit (
    Mcrd crd,
    int player,
    Unit_type * type,
    World * world)
{
  Unit * u      = calloc(1, sizeof(Unit));
  u->player     = player;
  u->mvp        = type->mvp;
  u->health     = type->health;
  u->can_attack = true;
  u->mcrd       = crd;
  u->type       = type;
  if(world->units.count > 0){
    Unit * last_unit = world->units.h->d;
    u->id = last_unit->id + 1;
  }else{
    u->id = 0;
  }
  add_default_features_to_unit(u);
  l_push(&world->units, u);
}



Feature
mk_feature_range (int power, int range){
  Feature f;
  f.rng.t = FEATURE_RNG;
  f.rng.power = power;
  f.rng.range = range;
  return(f);
}



Feature
mk_feature_berserk (int power){
  Feature f;
  f.brsk.t = FEATURE_BRSK;
  f.brsk.power = power;
  return(f);
}



Feature
mk_feature_bool (int type){
  Feature f;
  f.t = type;
  return(f);
}



static bool
is_event_visible (Event e){
  switch(e.t){
    case EVENT_MELEE: return(is_melee_visible(e));
    case EVENT_RANGE: return(is_range_visible(e));
    case EVENT_MOVE : return(is_move_visible (e));
    default: return(true);
  }
}



void
select_next_unit (){
  Node * node;
  Unit * u;
  if(selunit)
    node = unit2node(selunit);
  else
    node = cw->units.h;
  u = node->d;
  do{
    node = node->n ? node->n : cw->units.h;
    u = node->d;
  }while(u->player != cw->id);
  fill_map(selunit = u);
}



static void
add_event_local (Event data){
  Node * nd;
  FOR_EACH_NODE(worlds, nd){
    World * world = nd->d;
    Event * e = calloc(1, sizeof(Event));
    *e = data; /* copy */
    l_addtail(&world->eq, e);
  }
}



void
add_event (Event e){
  add_event_local(e);
  if(!is_local)
    send_event(e);
}

