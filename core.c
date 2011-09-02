/* See LICENSE file for copyright and license details. */

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
#include "utype.h"
#include "scenarios.h"




/* GLOBAL VARIABLES */

Unit_type utypes[3];
Mcrd    map_size;
List    worlds;
World * cw = NULL; /* current world */
bool    is_local;
bool    is_active; /* TODO rename! */
Unit *  selunit = NULL; /* selected unit */

static FILE * logfile;
static int scenario_players_count;


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
  l_delete_node(&cw->units, unit2node(u));
  fill_map(selunit);
}



static void
update_fog_after_move (Unit * u){
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(mdist(m, u->m) <= utypes[u->t].range_of_vision)
      tile(m)->fog++;
  }
}



static void
apply_move (Event e){
  Unit * u = id2unit(e.move.u);
  if(find_skill(u, S_IGNR))
    u->mvp -= e.move.cost;
  else
    u->mvp = 0;
  u->m = neib(u->m, e.move.dir);
  fill_map(selunit);
  if(u->player==cw->id)
    update_fog_after_move(u);
}



/* a - shooting unit, b - target */
static int
range_damage (Unit * a, Unit * b){
  Skill_range s   = find_skill(a, S_RANGE)->range;
  int hits        = 0;
  int wounds      = 0; /*possible wounds(may be blocked by armour)*/
  int final       = 0; /*final wounds(not blocked by armour)*/
  int attacks     = a->count;
  /*chances to hit, to wound and to ignore armour. percents.*/
  int to_hit      = 2 + s.skill;
  int to_wound    = 5 + (s.strength - utypes[b->t].toughness);
  int to_as       = 10- utypes[b->t].armor;
#if 1
  int r = rnd(0, 2);
  to_hit   += rnd(-r, r);
  to_wound += rnd(-r, r);
  to_as    += rnd(-r, r);
#endif
  fixnum(0, 9, &to_hit);
  fixnum(0, 9, &to_wound);
  hits   = attacks * to_hit   / 10;
  wounds = hits    * to_wound / 10;
  final  = wounds  * to_as    / 10;
#if 1
  printf("%i %i %i -> %i %i %i [%i]\n",
      to_hit, to_wound, to_as, attacks, hits, wounds, final);
#endif
  return(final);
}



static void
apply_melee(Event e){
  Unit * a = id2unit(e.melee.a);
  Unit * d = id2unit(e.melee.d);
  a->count -= e.melee.attackers_killed;
  d->count -= e.melee.defenders_killed;
  a->can_attack = false;
}



static void
apply_death (Event e){
  Unit *u = id2unit(e.death.u.id);
  kill_unit(u);
}



static void
apply_range (Event e){
  Unit * ua = id2unit(e.range.a);
  Unit * ud = id2unit(e.range.d);
  int dmg = e.range.dmg;
  ud->count -= dmg;
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
      && mdist(m, u->m) <= utypes[u->t].range_of_vision){
        tile(m)->fog++;
      }
    }
  }
}



static bool
is_invis (Unit * u){
  int i;
  if(!find_skill(u, S_INVIS)
  || u->player == cw->id)
    return(false);
  for(i=0; i<6; i++){
    Mcrd nb = neib(u->m, i);
    Unit * u2 = unit_at(nb);
    if(u2 && u2->player == cw->id)
      return(false);
  }
  return(true);
}



static bool
is_move_visible (Event e){
  Unit * u = id2unit(e.move.u);
  bool fow = tile(neib(u->m, e.move.dir))->fog || tile(u->m)->fog;
  bool hidden = is_invis(u) && u->player!=cw->id;
  return(!hidden && fow);
}



static bool
is_melee_visible (Event e){
  Unit * a = id2unit( e.melee.a );
  Unit * d = id2unit( e.melee.d );
  return(tile(a->m)->fog || tile(d->m)->fog);
}



static bool
is_range_visible (Event e){
  Unit * a = id2unit( e.range.a );
  Unit * d = id2unit( e.range.d );
  return(tile(a->m)->fog || tile(d->m)->fog);
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



static void
refresh_units (){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id){
      u->mvp = utypes[u->t].mvp;
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



/*old player's id, new player's id*/
static Event
mk_event_endturn (int old_id, int new_id){
  Event e;
  e.endturn.t = E_ENDTURN;
  e.endturn.old_player = old_id;
  e.endturn.new_player = new_id;
  return(e);
}



static Event
mk_event_move (Unit * u, int dir){
  int tile_type = tile(neib(u->m, dir))->t;
  Event e;
  e.move.t    = E_MOVE;
  e.move.u    = u->id;
  e.move.dir  = dir;
  e.move.cost = utypes[u->t].ter_mvp[tile_type];
  return(e);
}



static Event
mk_event_melee (
    Unit * a,
    Unit * d, 
    int attackers_killed,
    int defenders_killed)
{
  Event e;
  e.melee.t   = E_MELEE;
  e.melee.a   = a->id;
  e.melee.d   = d->id;
  e.melee.attackers_killed = attackers_killed;
  e.melee.defenders_killed = defenders_killed;
  return(e);
}



static Event
mk_event_range (Unit * a, Unit * d, int dmg){
  Event e;
  e.range.t   = E_RANGE;
  e.range.a   = a->id;
  e.range.d   = d->id;
  e.range.dmg = dmg;
  return(e);
}



static bool
ambush(Mcrd next, Unit * moving_unit){
  Unit * u = unit_at(next);
  if(u && u->player != moving_unit->player){
    add_event( mk_event_melee(u, moving_unit, 1, 3) );
    return(true);
  }else{
    return(false);
  }
}



/* [a]ttacker, [d]efender */
static void
support_range (Unit * a, Unit * d){
  Event range;
  Unit * sup;  /* [sup]porter */
  int i;
  for(i=0; i<6; i++){
    sup = unit_at( neib(d->m, i) );
    if(sup && sup->player == d->player 
    && find_skill(sup, S_RANGE)){
      break;
    }
  }
  if(i==6)
    return;
  range = mk_event_range(sup, a, 2);
  add_event(range);
}



static int
get_wounds (Unit *a, Unit *d){
  int hits     = 0;
  int wounds   = 0; /*possible wounds(may be blocked by armour)*/
  int final    = 0; /*final wounds(not blocked by armour)*/
  int attacks  = utypes[a->t].attacks * a->count;
  int a_ms     = utypes[a->t].ms + utypes[a->t].ter_ms[tile(d->m)->t];
  int d_ms     = utypes[a->t].ms + utypes[a->t].ter_ms[tile(d->m)->t];
  /*chances to hit, to wound and to ignore armour. percents.*/
  int to_hit   = 5 + (a_ms - d_ms);
  int to_wound = 5 + (utypes[a->t].strength - utypes[d->t].toughness);
  int to_as    = 10- utypes[d->t].armor;
#if 1
  int r = 1;
  to_hit   += rnd(-r, r);
  to_wound += rnd(-r, r);
  to_as    += rnd(-r, r);
#endif
  fixnum(0, 9, &to_hit);
  fixnum(0, 9, &to_wound);
  hits   = attacks * to_hit   / 10;
  wounds = hits    * to_as    / 10;
  final  = wounds  * to_wound / 10;
#if 1
  printf("%i %i %i -> %i %i %i [%i]\n",
      to_hit, to_wound, to_as, attacks, hits, wounds, final);
#endif
  return(final);
}
 


static Event
mk_event_death (Unit *u){
  Event e;
  e.t = E_DEATH;
  e.death.u = *u;
  return(e);
}



static void
flee (Unit *u){
  int dir = 0;
  while(unit_at((neib(u->m, dir))) && dir < 6)
    dir++;
  if(dir==6){
#if 0
    /*TODO if no place to run away,
      then attack random unit.*/
    attack(d, a);
#endif
  }else{
    add_event(mk_event_move(u, dir));
  }
}



/* [a]ttacker, [d]efender */
static void
attack_melee (Unit * a, Unit * d){
  Event melee;
  int attackers_killed;
  int defenders_killed;
  support_range(a, d);
  defenders_killed = get_wounds(a, d);
  attackers_killed = get_wounds(d, a);
  melee = mk_event_melee(a, d,
      attackers_killed, defenders_killed);
  add_event( melee );
  if(a->count - attackers_killed <= 0)
    add_event(mk_event_death(a));
  if(d->count - defenders_killed <= 0){
    add_event(mk_event_death(d));
  }else{
    /*TODO check unit's morale*/
    if(rand()%2)
      flee(d);
  }
  puts("");
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
      u->visible = tile(u->m)->fog>0 && !is_invis(u);
    }
  }
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
#if 0
  scenario = parse_scenario_file(av[2]);
  apply_scenario_to_all_worlds(scenario);
#else
  {
    Node *nd;
    scenario_players_count = scenario_02.players_count;
    map_size = scenario_02.map_size;
    FOR_EACH_NODE(worlds, nd){
      World *w = nd->d;
      scenario_02.init(w);
    }
  }
#endif
  is_local = true;
}



static void
net_arguments (int ac, char ** av){
  int no_players_left_mark = 0xff;
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

#if 0
  get_scenario_name_from_server(scenarioname);
  scenario = parse_scenario_file(scenarioname);
  apply_scenario_to_all_worlds(scenario);
#else
  /*TODO load hardcoded scenario*/
#endif
  is_local = false;
  add_event_local(mk_event_endturn(0, 0));
}



/*called fron add_unit*/
static int
new_unit_id (World *w){
  if(w->units.count > 0){
    Unit *u = w->units.h->d;
    return(u->id + 1);
  }else{
    return(0);
  }
}



Skill
mk_skill_range (int skill, int strength, int range){
  Skill s;
  s.range.t = S_RANGE;
  s.range.skill = skill;
  s.range.strength = strength;
  s.range.range = range;
  return(s);
}



#if 0
Skill
mk_skill_berserk (int power){
  Skill s;
  s.brsk.t = BRSK;
  s.brsk.power = power;
  return(s);
}
#endif



Skill
mk_skill_bool (int type){
  Skill s;
  s.t = type;
  return(s);
}



static bool
is_event_visible (Event e){
  switch(e.t){
    case E_MELEE: return(is_melee_visible(e));
    case E_RANGE: return(is_range_visible(e));
    case E_MOVE : return(is_move_visible (e));
    default: return(true);
  }
}



static void
apply_endturn(Event e){
  Node *nd;
  FOR_EACH_NODE(worlds, nd){
    World *w = nd->d;
    if(w->id == e.endturn.new_player){
      cw = w;
      is_active = true;
      check_win();
      refresh_units();
      updatefog(cw->id);
      update_units_visibility();
      return;
    }
  }
  is_active = false;
}




/*------------------NON-STATIC FUNCTION------------------*/



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



void
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
event2log (Event e){
  if(e.t == E_MOVE){
    fprintf(logfile,
        "MOVE  u=%i, dir=%i, cost=%i\n",
        e.move.u,
        e.move.dir,
        e.move.cost );
  }
  if(e.t == E_MELEE) {
    fprintf(logfile,
        "MELEE a=%i, d=%i, ak=%i, dk=%i\n",
        e.melee.a,
        e.melee.d,
        e.melee.attackers_killed,
        e.melee.defenders_killed );
  }
  if(e.t == E_RANGE) {
    fprintf(logfile,
        "RANGE a=%i, d=%i, dmg=%i\n",
        e.range.a,
        e.range.d,
        e.range.dmg );
  }
  if(e.t == E_ENDTURN) {
    fprintf(logfile,
        "TURN %i --> %i\n",
        e.endturn.old_player,
        e.endturn.new_player );
  }
  if(e.t == E_DEATH) {
    fprintf(logfile, "KILLED %i\n", e.death.u.id);
  }
}



void
add_event (Event e){
  add_event_local(e);
  event2log(e);
  if(!is_local)
    send_event(e);
}



void
cleanup(){
  fclose(logfile);
  while(worlds.count > 0){
    World * w = worlds.h->d;
    while(w->eq.count > 0)
      l_delete_node(&w->eq, w->eq.h);
    while(w->units.count > 0)
      l_delete_node(&w->units, w->units.h);
    free(w->map);
    l_delete_node(&worlds, worlds.h);
  }
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



void
endturn (){
  int id = cw->id + 1;
  if(id == scenario_players_count)
    id = 0;
  add_event(mk_event_endturn(cw->id, id));
}



void
move (Unit * moving_unit, Mcrd destination){
  List path = get_path(destination);
  Node * node;
  for(node=path.h; node->n; node=node->n){
    Mcrd * next    = node->n->d;
    Mcrd * current = node->d;
    if(ambush(*next, moving_unit))
      break;
    add_event( mk_event_move(moving_unit, mcrd2index(*current, *next)) );
  }
  while(path.count > 0)
    l_delete_node(&path, path.h);
}



/* [a]ttacker, [d]efender */
void
attack (Unit * a, Unit * d){
  Mcrd md = d->m;
  Mcrd ma = a->m;
  Skill * range = find_skill(a, S_RANGE);
  if(range){
    if(mdist(ma, md) <= range->range.range){
      int targets_killed = range_damage(a, d);
      add_event( mk_event_range(a, d, targets_killed) );
      if(d->count - targets_killed <= 0)
        add_event(mk_event_death(d));
      puts("");
    }
  }else{
    if(mdist(ma, md) <= 1){
      attack_melee(a, d);
    }
  }
}


void
init (int ac, char ** av){
  srand( (unsigned int)time(NULL) );
  init_unit_types();
  if(!strcmp(av[1], "-local"))
    local_arguments(ac, av);
  if(!strcmp(av[1], "-net"))
    net_arguments(ac, av);
  cw = worlds.h->d;
  updatefog(cw->id);
  update_units_visibility();
  logfile = fopen("log", "w");
  is_active = true;
}



void
apply_event (Event e){
  switch(e.t){
    case E_MOVE:    apply_move(e);    break;
    case E_MELEE:   apply_melee(e);   break;
    case E_RANGE:   apply_range(e);   break;
    case E_ENDTURN: apply_endturn(e); break;
    case E_DEATH:   apply_death(e);   break;
  }
  update_units_visibility();
}



void
add_unit (
    Mcrd crd,
    int player,
    int type,
    World * world)
{
  Unit * u      = calloc(1, sizeof(Unit));
  u->player     = player;
  u->mvp        = utypes[type].mvp;
  u->count      = utypes[type].count;
  u->can_attack = true;
  u->m          = crd;
  u->t          = type;
  u->id         = new_unit_id(world);
  u->skills_n   = utypes[type].skills_n;
  {
    int i;
    for(i=0; i<u->skills_n; i++)
      u->skills[i] = utypes[type].skills[i];
  }
  l_push(&world->units, u);
}



bool
is_eq_empty (){
  update_eq();
  return(cw->eq.count == 0);
}


