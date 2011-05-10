
/* if there is enemy at adjustment cell then attack it */

void ai_attack(Unit * u){
  int i;
  for(i=0; i<6; i++){
    Unit * enm = find_unit_at( neib(u->mcrd, i) );
    if(!enm || enm->player==player)
      continue;
    attack(u, enm);
  }
}



/* random movement */

void ai_movement(Unit * u){
  fill_map(u);
  int i;
  for(i=0; i<6; i++){
    Mcrd m = neib(u->mcrd, i);
    Unit * u2 = find_unit_at(m);

    /*if( is_invis(u2) || !mp(m)->fog ){
      mv(u, m);
    }*/

    if(!u2){
      mv(u, m);
      break;
    }
  }
}



void ai(){
  Node * n;
  FOR_EACH_NODE(cw->units, n){
    Unit * u = n->d;
    if(u->player != player)
      continue;

    ai_attack(u);
    ai_movement(u);
    apply_events_to_world();
  }

  /* end turn */
  onspace();
}

