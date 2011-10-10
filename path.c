/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include "list.h"
#include "utype.h"
#include "core.h"
#include "misc.h"
#include "path.h"

/* stack for filling map */
static Stack stack = {NULL, NULL, 0};

/* Push this coordinates to stack,
update cost and parent of this tile */
static void
push (Mcrd crd, Mcrd parent, int newcost) {
  Mcrd * m = malloc(sizeof(Mcrd));
  *m = crd;
  push_node(&stack, m);
  tile(crd)->cost   = newcost;
  tile(crd)->parent = parent;
}

static Mcrd
pop (void){
  Mcrd * tmp = pop_node(&stack);
  Mcrd m = *tmp;
  free(tmp);
  return( m );
}

/* zone of control */
static int
zoc (Mcrd a, Unit * u, int cost){
  int i;
  int mv = utypes[u->t].mv;
  for(i=0; i<6; i++){
    Mcrd n = neib(a, i);
    Unit * u2 = unit_at(n);
    if(inboard(n) && cost%mv != 0
    && u2 && u2->player != u->player
    && tile(n)->visible
    && u2->is_visible ){
      return(cost + mv - (cost % mv));
    }
  }
  return(cost);
}

/* process neiborhood */
/* TODO describe */
static void
process_nbh (Unit * u, Mcrd t, Mcrd nb){
  Unit * u2;
  int n, newcost;
  /* For not to pass through visible enemy. */
  u2 = unit_at(nb);
  if(u2 && u2->player!=u->player
  && tile(nb)->visible
  && u2->is_visible)
    return;
  n = utypes[u->t].ter_mv[tile(nb)->t];
  if(find_skill(u, S_IGNR))
    newcost = tile(t)->cost + n;
  else
    newcost = zoc(nb, u, tile(t)->cost + n);
  if(tile(nb)->cost>newcost && newcost<=utypes[u->t].mv)
    push(nb, t, newcost);
}

void
fill_map (Unit * u) {
  Mcrd m;
  if(!u)
    return;
  FOR_EACH_MCRD(m){
    tile(m)->cost   = 30000;
    tile(m)->parent = mk_mcrd(0,0);
  }  
  push(u->m, u->m, 0); /* push start point */
  while(stack.count>0){
    Mcrd t = pop();
    int i;
    for(i=0; i<6; i++){
      Mcrd nb = neib(t, i);
      if(inboard(nb))
        process_nbh(u, t, nb);
    }
  }
  while(stack.count > 0)
    delete_node(&stack, stack.h);
}

static void
addwaypoint (List * path, Mcrd m){
  Mcrd * tmp = malloc(sizeof(Mcrd));
  *tmp = m;
  push_node(path, tmp);
}

List
get_path (Mcrd destination){
  List path = {0, 0, 0};
  while(tile(destination)->cost != 0){
    addwaypoint(&path, destination);
    destination = tile(destination)->parent;
  }
  /* Adding starting position - here unit stays. */
  addwaypoint(&path, destination);
  return(path);
}
