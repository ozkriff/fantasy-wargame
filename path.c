/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include "list.h"
#include "structs.h"
#include "misc.h"
#include "core.h"
#include "path.h"



/* stack for filling map */
static Stack stack;



/* Push this coordinates to stack,
update cost and parent of this tile */
static void
push (Mcrd crd, Mcrd parent, int newcost) {
  Mcrd * m = malloc(sizeof(Mcrd));
  *m = crd;
  l_push(&stack, m);
  tile(crd)->cost   = newcost;
  tile(crd)->parent = parent;
}



static Mcrd
pop (){
  Mcrd * tmp = l_pop(&stack);
  Mcrd m = *tmp;
  free(tmp);
  return( m );
}



/* zone of control */
static int
zoc (Mcrd a, Unit * u, int cost){
  int i;
  int mvp = utypes[u->t].mvp;
  for(i=0; i<6; i++){
    Mcrd n = neib(a, i);
    Unit * u2 = find_unit_at(n);
    if(inboard(n) && cost%mvp != 0
    && u2 && u2->player != u->player
    && tile(n)->fog > 0
    && u2->visible ){
      return(cost + mvp - (cost % mvp));
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
  u2 = find_unit_at(nb);
  if(u2 && u2->player!=u->player
  && tile(nb)->fog>0
  && u2->visible)
    return;
  n = utypes[u->t].ter_mvp[tile(nb)->t];
  if(find_skill(u, SKILL_IGNR))
    newcost = tile(t)->cost + n;
  else
    newcost = zoc(nb, u, tile(t)->cost + n);
  if(tile(nb)->cost>newcost && newcost<=utypes[u->t].mvp)
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
    l_delete_node(&stack, stack.h);
}



static void
addwaypoint (List * path, Mcrd m){
  Mcrd * tmp = malloc(sizeof(Mcrd));
  *tmp = m;
  l_push(path, tmp);
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
