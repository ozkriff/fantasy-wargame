/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "bool.h"
#include "list.h"
#include "utype.h"
#include "core.h"
#include "path.h"
#include "misc.h"
#include "core_private.h"
#include "net.h"
#include "scenarios.h"

Unit_type utypes[30];
Mcrd    map_size;
Tile   *map;
bool    is_local = true;
bool    is_client_active;
Unit   *selected_unit;
List    units = {0, 0, 0};
Player *current_player;

static FILE     *logfile;
static Scenario *current_scenario;
static List      dead_units = {0, 0, 0};
static List      players = {0, 0, 0};
static List      eventlist = {0, 0, 0};

static void
kill_unit (Unit *u){
  Node *n = extruct_node(&units, data2node(units, u));
  push_node(&dead_units, n);
  if(u == selected_unit)
    selected_unit = NULL;
  else
    fill_map(selected_unit);
}

static void
update_fog_after_move (Unit *u){
  Mcrd m;
  FOR_EACH_MCRD(m){
    if(mdist(m, u->m) <= utypes[u->t].v)
      tile(m)->visible = true;
  }
}

static void
apply_move (Event_move e){
  Unit *u = id2unit(e.u);
  if(find_skill(u, S_IGNR))
    u->mv -= e.cost;
  else
    u->mv = 0;
  u->stamina -= e.cost;
  u->m = neib(u->m, e.dir);
  fill_map(selected_unit);
  if(u->player == current_player->id)
    update_fog_after_move(u);
}

/* a - shooting unit, b - target */
static int
range_damage (Unit *a, Unit *b){
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
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  a->count -= e.attackers_killed;
  d->count -= e.defenders_killed;
  a->stamina -= 2;
  d->stamina -= 2;
  a->can_attack = false;
}

static void
apply_death (Event_death e){
  Unit *u = id2unit(e.dead_unit_id);
  kill_unit(u);
}

static void
apply_range (Event_range e){
  Unit *ua = id2unit(e.a);
  Unit *ud = id2unit(e.d);
  ud->count -= e.defenders_killed;
  ua->can_attack = false;
  find_skill(ua, S_RANGE)->range.shoots--;
}

static void
updatefog (int player){
  Mcrd m;
  FOR_EACH_MCRD(m){
    Node *node;
    tile(m)->visible = false;
    FOR_EACH_NODE(units, node){
      Unit *u = node->d;
      if(u->player == player
      && mdist(m, u->m) <= utypes[u->t].v)
        tile(m)->visible = true;
    }
  }
}

static bool
is_invis (Unit *u){
  int i;
  if(!find_skill(u, S_INVIS)
  || u->player == current_player->id)
    return(false);
  for(i=0; i<6; i++){
    Mcrd nb = neib(u->m, i);
    Unit *u2 = unit_at(nb);
    if(u2 && u2->player == current_player->id)
      return(false);
  }
  return(true);
}

static bool
is_move_visible (Event_move e){
  Unit *u = id2unit(e.u);
  Mcrd m = neib(u->m, e.dir);
  bool fow = tile(m)->visible || tile(u->m)->visible;
  bool hidden = is_invis(u) && u->player != current_player->id;
  return(!hidden && fow);
}

static bool
is_melee_visible (Event_melee e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  return(tile(a->m)->visible || tile(d->m)->visible);
}

static bool
is_range_visible (Event_range e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  return(tile(a->m)->visible || tile(d->m)->visible);
}

static bool
checkunitsleft(){
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u = node->d;
    if(u->player == current_player->id)
      return(true);
  }
  return(false);
}

static void
refresh_unit (Unit *u){
  Skill *range;
  Unit_type *t = &utypes[u->t];
  u->mv = t->mv;
  u->can_attack = true;
  u->stamina += t->stamina_rg;
  u->morale += t->morale_rg;
  fixnum(0, t->stamina, &u->stamina);
  fixnum(0, t->morale, &u->morale);
  range = find_skill(u, S_RANGE);
  if(range){
    range->range.shoots = range->range.shoots_max;
  }
}

/*Search through friends in vision range and add differense
  between friend's cost and this unit's cost to morale.
  Search through enemies in vision range and add differense
  between enemy's cost and this unit's cost to morale.*/
static void
update_unit_morale (Unit *u){
  Unit_type *t = utypes + u->t;
  int plus = 0;
  int minus = 0;
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u2 = node->d;
    Unit_type *t2 = utypes + u2->t;
    int diff = 1 + (t2->cost - t->cost);
    fixnum(0, t2->cost, &diff);
    if(mdist(u->m, u2->m) <= t->v){
      if(u->player == u2->player)
        plus += diff;
      else
        minus += diff;
    }
  }
  u->morale = u->morale + plus - minus;
  fixnum(0, t->morale, &u->morale);
}

static void
refresh_units (int player_id){
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u = node->d;
    if(u->player == player_id){
      refresh_unit(u);
      update_unit_morale(u);
    }
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
  e.t = E_ENDTURN;
  e.e.endturn.old_id = old_id;
  e.e.endturn.new_id = new_id;
  return(e);
}

static Event
mk_event_move (Unit *u, int dir){
  Tile_type_id tile_type = tile(neib(u->m, dir))->t;
  Event e;
  e.t = E_MOVE;
  e.e.move.u    = u->id;
  e.e.move.dir  = dir;
  e.e.move.cost = utypes[u->t].ter_mv[tile_type];
  return(e);
}

static Event
mk_event_melee (
    Unit *a,
    Unit *d, 
    int attackers_killed,
    int defenders_killed)
{
  Event e;
  e.t = E_MELEE;
  e.e.melee.a                = a->id;
  e.e.melee.d                = d->id;
  e.e.melee.attackers_killed = attackers_killed;
  e.e.melee.defenders_killed = defenders_killed;
  return(e);
}

static Event
mk_event_range (Unit *a, Unit *d, int dmg){
  Event e;
  e.t = E_RANGE;
  e.e.range.a   = a->id;
  e.e.range.d   = d->id;
  e.e.range.defenders_killed = dmg;
  return(e);
}

static bool
ambush(Mcrd next, Unit *moving_unit){
  Unit *u = unit_at(next);
  if(u && u->player != moving_unit->player){
    add_event( mk_event_melee(u, moving_unit, 1, 3) );
    return(true);
  }else{
    return(false);
  }
}

/* [a]ttacker, [d]efender */
static int
support_range (Unit *a, Unit *d){
  int killed = 0;
  Unit *sup;  /* [sup]porter */
  int i;
  for(i=0; i<6; i++){
    sup = unit_at( neib(d->m, i) );
    if(sup && sup->player == d->player 
    && find_skill(sup, S_RANGE)){
      break;
    }
  }
  if(i==6)
    return(0);
  if(find_skill(sup, S_RANGE)->range.shoots > 0){
    killed = range_damage(sup, a);
    add_event(mk_event_range(sup, a, killed));
  }
  return(killed);
}

static int
get_wounds (Unit *a, Unit *d){
  Unit_type at = utypes[a->t];
  Unit_type dt = utypes[d->t];
  int tt = tile(d->m)->t; /*tile type*/
  int hits     = 0;
  int wounds   = 0; /*possible wounds(may be blocked by armour)*/
  int final    = 0; /*final wounds(not blocked by armour)*/
  int attacks  = at.attacks * a->count   * a->stamina/at.stamina;
  int a_ms     = (at.ms + at.ter_ms[tt]) * a->stamina/at.stamina;
  int d_ms     = (dt.ms + dt.ter_ms[tt]) * d->stamina/dt.stamina;
  /*chances to hit, to wound and to ignore armour. percents.*/
  int to_hit   = 5 + (a_ms - d_ms);
  int a_stren  = at.strength * a->stamina/at.stamina;
  int to_wound = 5 + (a_stren - dt.toughness);
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
  e.e.death.dead_unit_id = u->id;
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
attack_melee (Unit *a, Unit *d){
  Event melee;
  int attackers_killed;
  int defenders_killed;
  int sup_killed = support_range(a, d);
  if(a->count - sup_killed <= 0){
    add_event(mk_event_death(a));
    return;
  }
  defenders_killed = get_wounds(a, d);
  if(!find_skill(a, S_NORETURN))
    attackers_killed = get_wounds(d, a);
  else
    attackers_killed = 0;
  melee = mk_event_melee(a, d,
      attackers_killed, defenders_killed);
  add_event( melee );
  if(a->count - (attackers_killed + sup_killed) <= 0)
    add_event(mk_event_death(a));
  if(d->count - defenders_killed <= 0){
    add_event(mk_event_death(d));
  }else{
    /*TODO check unit's morale*/
#if 0
    if(d->morale < utypes[d->t].morale/4)
#endif
      flee(d);
  }
  puts("");
}

/* Check and update visibility of all units in CW */
static void
update_units_visibility (void){
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u = node->d;
    if(u->player == current_player->id){
      u->is_visible = true;
    }else{
      u->is_visible = tile(u->m)->visible && !is_invis(u);
    }
  }
}

static void
create_local_human (int id) {
  Player *p = calloc(1, sizeof(Player));
  p->id     = id;
  p->is_ai  = false;
  p->last_event_id = -1;
  push_node(&players, p);
}

static void
create_local_ai (int id) {
  Player *p = calloc(1, sizeof(Player));
  p->id     = id;
  p->is_ai  = true;
  p->last_event_id = -1;
  push_node(&players, p);
}

/*called from add_unit*/
static int
new_unit_id (){
  if(units.count > 0){
    Unit *u = units.h->d;
    return(u->id + 1);
  }else{
    return(0);
  }
}

static bool
is_event_visible (Event e){
  switch(e.t){
    case E_MELEE:   return(is_melee_visible(e.e.melee));
    case E_RANGE:   return(is_range_visible(e.e.range));
    case E_MOVE:    return(is_move_visible(e.e.move));
    case E_DEATH:   return(true);
    case E_ENDTURN: return(true);
    default:
      die("core: is_event_visible(): "
          "unknown event '%i'\n", e.t);
      return(true);
  }
}

/*undo all events that this player have not seen yet*/
static void
undo_unshown_events (){
  Node *node = eventlist.t;
  while(node){
    Event *e = node->d;
    if(e->id == current_player->last_event_id)
      break;
    undo_event(*e);
    node = node->p;
  }
}

static void
apply_endturn(Event_endturn e){
  Node *nd;
  FOR_EACH_NODE(players, nd){
    Player *p = nd->d;
    if(p->id == e.new_id){
      current_player = p;
      undo_unshown_events();
      is_client_active = true;
      check_win();
      refresh_units(current_player->id);
      updatefog(current_player->id);
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
  FOR_EACH_NODE(players, nd){
    Player *p = nd->d;
    send_int_as_uint8(p->id);
  }
  send_int_as_uint8(no_players_left_mark);
}

static void
undo_move (Event_move e){
  Unit *u = id2unit(e.u);
  int dir = e.dir + 3;
  if(dir >= 6)
    dir -= 6;
  if(find_skill(u, S_IGNR))
    u->mv += e.cost;
  else
    u->mv = utypes[u->t].mv;
  u->stamina += e.cost;
  u->m = neib(u->m, dir);
  fill_map(selected_unit);
  if(u->player == current_player->id)
    updatefog(current_player->id);
}

static void
undo_melee (Event_melee e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  a->count += e.attackers_killed;
  d->count += e.defenders_killed;
  a->stamina += 2;
  d->stamina += 2;
  a->can_attack = true;;
}

static void
undo_range (Event_range e){
  Unit *a = id2unit(e.a);
  Unit *d = id2unit(e.d);
  d->count += e.defenders_killed;
  a->can_attack = true;;
}

static void
undo_death (Event_death e){
  Node *n = deq_node(&dead_units);
  Unit *u = n->d;
  if(u->id != e.dead_unit_id)
    die("NOOO");
  push_node(&units, n->d);
  free(n);
  fill_map(selected_unit);
}

/*------------------NON-STATIC FUNCTIONS-----------------*/

void
undo_event (Event e){
  switch(e.t){
    case E_MOVE:  undo_move(e.e.move);   break;
    case E_MELEE: undo_melee(e.e.melee); break;
    case E_DEATH: undo_death(e.e.death); break;
    case E_RANGE: undo_range(e.e.range); break;
    default: break; 
  }
  update_units_visibility();
}

void
select_next_unit (void){
  Node *node;
  Unit *u;
  if(selected_unit)
    node = data2node(units, selected_unit);
  else
    node = units.h;
  u = node->d;
  do{
    node = node->n ? node->n : units.h;
    u = node->d;
  }while(u->player != current_player->id);
  fill_map(selected_unit = u);
}

void
add_event_local (Event data){
  Event *e = calloc(1, sizeof(Event));
  *e = data;
  add_node_to_tail(&eventlist, e);
}

void
event2log (Event e){
  switch(e.t){
    case E_MOVE:
      fprintf(logfile, "MOVE  u=%i, dir=%i, cost=%i\n",
          e.e.move.u, e.e.move.dir, e.e.move.cost );
      break;
    case E_MELEE:
      fprintf(logfile, "MELEE a=%i, d=%i, ak=%i, dk=%i\n",
          e.e.melee.a,
          e.e.melee.d,
          e.e.melee.attackers_killed,
          e.e.melee.defenders_killed );
       break;
    case E_RANGE:
      fprintf(logfile, "RANGE a=%i, d=%i, killed=%i\n",
          e.e.range.a, e.e.range.d, e.e.range.defenders_killed );
      break;
    case E_ENDTURN:
      fprintf(logfile, "TURN %i --> %i\n",
          e.e.endturn.old_id,
          e.e.endturn.new_id);
      break;
    case E_DEATH:
      fprintf(logfile, "KILLED %i\n", e.e.death.dead_unit_id);
      break;
    default:
      die("core: event2log(): "
          "unknown event '%i'\n", e.t);
      break;
   }
}

void
add_event (Event e){
  if(eventlist.count > 0){
    Event *lastevent = eventlist.t->d;
    e.id = lastevent->id + 1;
  }else{
    e.id = 0;
  }
  add_event_local(e);
  event2log(e);
  if(!is_local)
    send_event(e);
}

void
cleanup (void){
  fclose(logfile);
  logfile = NULL;
  while(players.count > 0)
    delete_node(&players, players.h);
  while(eventlist.count > 0)
    delete_node(&eventlist, eventlist.h);
  while(units.count > 0)
    delete_node(&units, units.h);
  while(dead_units.count > 0)
    delete_node(&dead_units, dead_units.h);
}

static void
apply_invisible_events (void){
  Event *e;
  Node *node;
  if(eventlist.count == 0)
    return;
  /*find last seen event*/
  FOR_EACH_NODE(eventlist, node){ 
    e = node->d;
    if(e->id == current_player->last_event_id)
      break;
  }
  if(!node)
    return;
  node = node->n;
  while(node){
    e = node->d;
    if(!is_event_visible(*e))
      apply_event(*e);
    else
      break;
    node = node->n;
  }
}

/* always called after apply_invisible_events */
Event *
get_next_event (void){
  Node *node;
  Event *e = NULL;
  if(eventlist.count == 0)
    return(NULL);
  if(current_player->last_event_id == -1){
    e = eventlist.h->d;
    return(e);
  }
  FOR_EACH_NODE(eventlist, node){ 
    e = node->d;
    if(e->id == current_player->last_event_id){
      e = node->n->d;
      return(e);
    }
  }
  return(NULL);
}

void
endturn (void){
  int id = current_player->id + 1;
  if(id == current_scenario->players_count)
    id = 0;
  selected_unit = NULL;
  add_event(mk_event_endturn(current_player->id, id));
}

void
move (Unit *moving_unit, Mcrd destination){
  List path;
  Node *node;
  if(tile(destination)->cost > moving_unit->mv)
    return;
  path = get_path(destination);
  for(node=path.h; node->n; node=node->n){
    Mcrd *next    = node->n->d;
    Mcrd *current = node->d;
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
attack (Unit *a, Unit *d){
  int dist = mdist(a->m, d->m);
  Skill *range = find_skill(a, S_RANGE);
  if(range){
    if(dist <= range->range.range
    && range->range.shoots > 0){
      int targets_killed = range_damage(a, d);
      add_event( mk_event_range(a, d, targets_killed) );
      if(d->count - targets_killed <= 0)
        add_event(mk_event_death(d));
      puts("");
    }
  }else{
    if(a->can_attack && dist == 1){
      attack_melee(a, d);
    }
  }
}

void
apply_event (Event e){
  current_player->last_event_id = e.id;
  switch(e.t){
    case E_MOVE:    apply_move(e.e.move);       break;
    case E_MELEE:   apply_melee(e.e.melee);     break;
    case E_RANGE:   apply_range(e.e.range);     break;
    case E_ENDTURN: apply_endturn(e.e.endturn); break;
    case E_DEATH:   apply_death(e.e.death);     break;
    default:
      die("core: apply_event(): "
          "unknown event '%i'\n", e.t);
      break;
  }
  update_units_visibility();
}

void
add_unit (Mcrd crd, int plr, int type) {
  Unit *u       = calloc(1, sizeof(Unit));
  u->player     = plr;
  u->mv         = utypes[type].mv;
  u->count      = utypes[type].count;
  u->can_attack = true;
  u->m          = crd;
  u->t          = type;
  u->id         = new_unit_id();
  u->skills_n   = utypes[type].skills_n;
  u->stamina    = utypes[type].stamina;
  u->morale     = utypes[type].morale;
  memcpy(u->skills, utypes[type].skills,
      u->skills_n*sizeof(Skill));
  push_node(&units, u);
}

/* called before get_next_event */
bool
unshown_events_left (void){
  apply_invisible_events();
  if(eventlist.count == 0){
    return(false);
  }else{
    Event *e = eventlist.t->d;
    return(e->id != current_player->last_event_id);
  }
}

void
init_local_players (int n, int *ids){
  int i;
  for(i=0; i<n; i++)
    create_local_human(ids[i]);
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
  FOR_EACH_NODE(players, nd){
    if(i==id){
      Player *p = nd->d;
      p->is_ai = true;
      return;
    }
    i++;
  }
}

/*example: init_local_players_s("hh", 0, 1);*/
void
init_local_players_s (char *s, ...){
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
      die("core: init_players_s(): "
          "%c, %i\n", *c, *c);
    }
    c++;
  }
  va_end(ap);
  current_player = players.t->d;
}

void
set_scenario_id (int id){
  current_player = players.t->d;
  current_scenario = scenarios + id;
  map_size = current_scenario->map_size;
  current_scenario->init();
  updatefog(current_player->id);
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
  selected_unit = NULL;
  current_player = NULL;
  logfile = NULL;
  current_scenario = NULL;
}

