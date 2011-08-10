
#include <stdbool.h>
#include <stdlib.h>
#include "list.h"
#include "structs.h"
#include "misc.h"
#include "core.h"
#include "path.h"



/* stack for filling map */
static Stack stack;



static void
push (Mcrd crd, Mcrd parent, int newcost) {
  Mcrd * m;
  tile(crd)->cost   = newcost;
  tile(crd)->parent = parent;
  
  m = malloc(sizeof(Mcrd));
  *m = crd;
  l_push(&stack, m);
}



static Mcrd
pop (){
  Mcrd * tmp = (Mcrd*)l_pop(&stack);
  Mcrd m = *tmp;
  free(tmp);
  return( m );
}



/* zone of control */
static int
zoc (Mcrd a, Unit * u, int cost){
  int mvp;
  int i;
  if(find_feature(u, FEATURE_IGNR))
    return(cost);

  mvp = u->type->mvp;
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
  
  if( ! inboard(nb) )
    return;

  /* For not to pass through visible enemy. */
  u2 = find_unit_at(nb);
  if(u2 && u2->player!=u->player
  && tile(nb)->fog>0
  && u2->visible)
    return;

  n       = u->type->ter_mvp[tile(nb)->type];
  newcost = zoc(nb, u, tile(t)->cost + n);

  if(tile(nb)->cost>newcost && newcost<=u->type->mvp)
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
  push(u->mcrd, u->mcrd, 0); /* push start point */
  while(stack.count>0){
    Mcrd t = pop();
    int i;
    for(i=0; i<6; i++)
      process_nbh(u, t, neib(t, i));
  }
	while(stack.count > 0)
		l_delete_node(&stack, stack.h);
}



static void
addwaypoint (List * path, Mcrd wp){
  Mcrd * m = malloc(sizeof(Mcrd));
  *m = wp;
  l_push(path, m);
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
