/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
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

Unit_type utypes[30];
Mcrd    map_size;
List    worlds;
World * cw = NULL; /* current world */
bool    is_local = true;
bool    is_client_active;
Unit *  selected_unit = NULL;

static FILE * logfile = NULL;
static Scenario *current_scenario = NULL;

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
  if(u == selected_unit)
    selected_unit = NULL;
  delete_node(&cw->units, unit2node(u));
  fill_map(selected_unit);
}

static void
update_fog_after_move (Unit * u){
  Mcrd m;
  FOR_EACH_MCRD(m){
    int range = utypes[u->t].range_of_vision;
    if(mdist(m, u->m) <= range)
      tile(m)->fog++;
  }
}

static void
apply_move (Event_move e){
  Unit * u = id2unit(e.u);
  if(find_skill(u, S_IGNR))
    u->mvp -= e.cost;
  else
    u->mvp = 0;
  u->energy -= e.cost;
  u->m = neib(u->m, e.dir);
  fill_map(selected_unit);
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
      to_hit, to_wound, to_as,
      attacks, hits, wounds, final);
#endif
  return(final);
}

static void
apply_melee(Event_melee e){
  Unit * a = id2unit(e.a);
  Unit * d = id2unit(e.d);
  a->count -= e.attackers_killed;
  d->count -= e.defenders_killed;
  a->energy -= 2;
  d->energy -= 2;
  a->can_attack = false;
}

static void
apply_death (Event_death e){
  Unit *u = id2unit(e.u.id);
  kill_unit(u);
}

static void
apply_range (Event_range e){
  Unit * ua = id2unit(e.a);
  Unit * ud = id2unit(e.d);
  int dmg = e.dmg;
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
      int range = utypes[u->t].range_of_vision;
      if(u->player == player
      && mdist(m, u->m) <= range)
        tile(m)->fog++;
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
is_move_visible (Event_move e){
  Unit * u = id2unit(e.u);
  Mcrd m = neib(u->m, e.dir);
  bool fow = tile(m)->fog || tile(u->m)->fog;
  bool hidden = is_invis(u) && u->player!=cw->id;
  return(!hidden && fow);
}

static bool
is_melee_visible (Event_melee e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  return(tile(a->m)->fog || tile(d->m)->fog);
}

static bool
is_range_visible (Event_range e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
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
refresh_units (void){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    u->mvp = utypes[u->t].mvp;
    u->can_attack = true;
    u->energy += utypes[u->t].energy_rg;
    fixnum(0, utypes[u->t].energy, &u->energy);
  }
}

static void
check_win (void){
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
  e.endturn.t          = E_ENDTURN;
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
  e.melee.t                = E_MELEE;
  e.melee.a                = a->id;
  e.melee.d                = d->id;
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
  range = mk_event_range(sup, a, range_damage(sup, a));
  add_event(range);
}

static int
get_wounds (Unit *a, Unit *d){
  Unit_type at = utypes[a->t];
  Unit_type dt = utypes[d->t];
  int tt = tile(d->m)->t; /*tile type*/
  int hits     = 0;
  int wounds   = 0; /*possible wounds(may be blocked by armour)*/
  int final    = 0; /*final wounds(not blocked by armour)*/
  int attacks  = at.attacks * a->count;
  int a_ms     = (at.ms + at.ter_ms[tt]) * a->energy/at.energy;
  int d_ms     = (dt.ms + dt.ter_ms[tt]) * d->energy/dt.energy;
  /*chances to hit, to wound and to ignore armour. percents.*/
  int to_hit   = 5 + (a_ms - d_ms);
  int to_wound = 5 + (at.strength - dt.toughness);
  int to_as    = 10- dt.armor;
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
      to_hit, to_wound, to_as,
      attacks, hits, wounds, final);
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
  int i;
  for(i=0; i<6; i++){
    int dir = opposite_neib_index(i);
    Mcrd m = neib(u->m, dir);
    if(inboard(m) && !unit_at(m)){
      add_event(mk_event_move(u, dir));
      return;
    }
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
update_units_visibility (void){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id){
      u->is_visible = true;
    }else{
      u->is_visible = tile(u->m)->fog>0 && !is_invis(u);
    }
  }
}

static void
create_local_world (int id, bool is_ai) {
  World * w = calloc(1, sizeof(World));
  w->id     = id;
  w->is_ai  = is_ai;
  push_node(&worlds, w);
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
apply_scenario_to_all_worlds (Scenario *s){
  Node *nd;
  FOR_EACH_NODE(worlds, nd){
    World *w = nd->d;
    s->init(w);
  }
}

/*called from add_unit*/
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
  s.t = S_RANGE;
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
    case E_MELEE:   return(is_melee_visible(e.melee));
    case E_RANGE:   return(is_range_visible(e.range));
    case E_MOVE:    return(is_move_visible(e.move));
    case E_DEATH:   return(true);
    case E_ENDTURN: return(true);
    default:
      die("core: is_event_visible(): "
          "unknown event '%i'\n", e.t);
      return(true);
  }
}

static void
apply_endturn(Event_endturn e){
  Node *nd;
  if(e.new_player == 0){
    check_win();
    refresh_units();
  }
  FOR_EACH_NODE(worlds, nd){
    World *w = nd->d;
    if(w->id == e.new_player){
      cw = w;
      is_client_active = true;
      updatefog(cw->id);
      update_units_visibility();
      return;
    }
  }
  is_client_active = false;
}

static void
send_ids_to_server (void){
  int no_players_left_mark = 0xff;
  Node *nd;
  FOR_EACH_NODE(worlds, nd){
    World *w = nd->d;
    send_int_as_uint8(w->id);
  }
  send_int_as_uint8(no_players_left_mark);
}

/*------------------NON-STATIC FUNCTIONS-----------------*/

void
select_next_unit (void){
  Node * node;
  Unit * u;
  if(selected_unit)
    node = unit2node(selected_unit);
  else
    node = cw->units.h;
  u = node->d;
  do{
    node = node->n ? node->n : cw->units.h;
    u = node->d;
  }while(u->player != cw->id);
  fill_map(selected_unit = u);
}

void
add_event_local (Event data){
  Node * nd;
  FOR_EACH_NODE(worlds, nd){
    World * world = nd->d;
    Event * e = calloc(1, sizeof(Event));
    *e = data; /* copy */
    add_node_to_tail(&world->eq, e);
  }
}

void
event2log (Event e){
  switch(e.t){
    case E_MOVE:
      fprintf(logfile, "MOVE  u=%i, dir=%i, cost=%i\n",
          e.move.u, e.move.dir, e.move.cost );
      break;
    case E_MELEE:
      fprintf(logfile, "MELEE a=%i, d=%i, ak=%i, dk=%i\n",
          e.melee.a,
          e.melee.d,
          e.melee.attackers_killed,
          e.melee.defenders_killed );
       break;
    case E_RANGE:
      fprintf(logfile, "RANGE a=%i, d=%i, dmg=%i\n",
          e.range.a, e.range.d, e.range.dmg );
      break;
    case E_ENDTURN:
      fprintf(logfile, "TURN %i --> %i\n",
          e.endturn.old_player, e.endturn.new_player );
      break;
    case E_DEATH:
      fprintf(logfile, "KILLED %i\n", e.death.u.id);
      break;
    default:
      die("core: event2log(): "
          "unknown event '%i'\n", e.t);
      break;
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
cleanup (void){
  fclose(logfile);
  logfile = NULL;
  while(worlds.count > 0){
    World * w = worlds.h->d;
    while(w->eq.count > 0)
      delete_node(&w->eq, w->eq.h);
    while(w->units.count > 0)
      delete_node(&w->units, w->units.h);
    free(w->map);
    delete_node(&worlds, worlds.h);
  }
}

/* called before get_next_event */
void
update_eq (void){
  Event * e;
  while(cw->eq.count > 0){
    e = cw->eq.h->d;
    if(!is_event_visible(*e)){
      e = deq_node(&cw->eq);
      apply_event(*e);
      free(e);
    }else{
      return;
    }
  }
}

/* always called after update_eq */
Event
get_next_event (void){
  Event * tmp = deq_node(&cw->eq);
  Event e = *tmp;
  free(tmp);
  return(e);
}

void
endturn (void){
  int id = cw->id + 1;
  if(id == current_scenario->players_count)
    id = 0;
  add_event(mk_event_endturn(cw->id, id));
}

void
move (Unit * moving_unit, Mcrd destination){
  List path;
  Node * node;
  if(tile(destination)->cost > moving_unit->mvp)
    return;
  path = get_path(destination);
  for(node=path.h; node->n; node=node->n){
    Mcrd * next    = node->n->d;
    Mcrd * current = node->d;
    if(ambush(*next, moving_unit))
      break;
    add_event( mk_event_move(moving_unit,
        mcrd2index(*current, *next)) );
  }
  while(path.count > 0)
    delete_node(&path, path.h);
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
apply_event (Event e){
  switch(e.t){
    case E_MOVE:    apply_move(e.move);       break;
    case E_MELEE:   apply_melee(e.melee);     break;
    case E_RANGE:   apply_range(e.range);     break;
    case E_ENDTURN: apply_endturn(e.endturn); break;
    case E_DEATH:   apply_death(e.death);     break;
    default:
      die("core: apply_event(): "
          "unknown event '%i'\n", e.t);
      break;
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
  u->energy     = utypes[type].energy;
  memcpy(u->skills, utypes[type].skills,
      u->skills_n*sizeof(Skill));
  push_node(&world->units, u);
}

bool
is_eq_empty (void){
  update_eq();
  return(cw->eq.count == 0);
}

void
init_local_worlds (int n, int *ids){
  int i;
  printf("new_worlds: %i\n", n);
  for(i=0; i<n; i++)
    create_local_human(ids[i]);
  cw = worlds.h->d;
}

void
init_net (char *host, int port){
  init_network(host, port);
  send_ids_to_server();
  set_scenario_id(get_scenario_from_server());
  is_local = false;
  add_event_local(mk_event_endturn(0, 0));
}

void
mark_ai (int id){
  int i = 0;
  Node *nd;
  FOR_EACH_NODE(worlds, nd){
    if(i==id){
      World *w = nd->d;
      w->is_ai = true;
      return;
    }
    i++;
  }
}

/*example: init_local_worlds_s("hh", 0, 1);*/
void
init_local_worlds_s (char *s, ...){
  va_list ap;
  char *c = s;
  va_start(ap, s);
  while(c && *c){
    int id = va_arg(ap, int);
    if(*c=='h'){
      create_local_human(id);
    }else if(*c=='a'){
      create_local_ai(id);
    }else{
      die("core: init_worlds_s(): "
          "%c, %i\n", *c, *c);
    }
    c++;
  }
  va_end(ap);
  cw = worlds.t->d;
}

void
set_scenario_id (int id){
  current_scenario = scenarios + id;
  map_size = current_scenario->map_size;
  apply_scenario_to_all_worlds(current_scenario);
  updatefog(cw->id);
  update_units_visibility();
  if(logfile)
    die("core: set_scenario_id(): logfile != NULL.");
  logfile = fopen("log", "w");
}

void
init (void){
  srand( (unsigned int)time(NULL) );
  init_unit_types();
  init_scenarios();
  is_client_active = true;
}

