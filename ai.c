
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include "list.h"
#include "structs.h"
#include "core.h"
#include "path.h"
#include "misc.h"
#include "ai.h"


/* if there is enemy at adjustment cell then attack it */
static void
ai_attack(Unit * u){
  int i;
  for(i=0; i<6; i++){
    Unit * enm = find_unit_at( neib(u->m, i) );
    if(!enm || enm->player==cw->id)
      continue;
    attack(u, enm);
  }
}



/* random movement */
static void
ai_movement(Unit * u){
  int i;
  fill_map(u);
  for(i=0; i<6; i++){
    Mcrd m = neib(u->m, i);
    Unit * u2 = find_unit_at(m);
    /*if( is_invis(u2) || !mp(m)->fog ){
      mv(u, m);
    }*/
    if(!u2){
      move(u, m);
      break;
    }
  }
}



void
ai (){
  Node * n;
  while(cw->eq.count > 0){
    apply_event(get_next_event());
  }
  FOR_EACH_NODE(cw->units, n){
    Unit * u = n->d;
    if(u->player != cw->id)
      continue;
    ai_attack(u);
    ai_movement(u);
    while(cw->eq.count > 0){
      apply_event(get_next_event());
    }
  }
  endturn();
}
