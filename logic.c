

// change tile type
void change_tile(mcrd m){
  if(mp(m)->type ++ == 4)
    mp(m)->type = 0;
  if(cw->selunit) fill_map(cw->selunit);
}



void select_next_unit(){
  do{
    if(cw->selunit && cw->selunit != (unit*)l_last(cw->units))
      cw->selunit = (unit*)l_next(cw->selunit);
    else
      cw->selunit = (unit*)l_first(cw->units);
  }while(cw->selunit->player != player);
  fill_map(cw->selunit);
}



void select_unit(mcrd m){
  cw->selunit = mp(m)->unit;
  fill_map(cw->selunit);
}



void kill_unit(unit * u){
  mp(u->mcrd)->unit = NULL;
  if(u == cw->selunit) cw->selunit = NULL;
  free( l_rem(cw->units, u) );
}




void finish_movement(){
  //mcrd m = {cw->e[1+cw->mi+1], cw->e[1+cw->mi]};
  unit * u = id2unit(cw->e[2]);
  mp(u->mcrd)->unit = u;
  u->scrd = map2scr(u->mcrd);
  u->mvp -= mp(u->mcrd)->cost;
  if(cw->selunit==u)
    fill_map(u);
  cw->mode = MODE_SELECT;
}



void move_logic(){
  if(cw->index == STEPS){
    cw->index = 0;
    cw->mi++;
    id2unit(cw->e[2])->mcrd
      = neib(id2unit(cw->e[2])->mcrd, cw->e[cw->mi]);
    // TODO вот именно тут и надо обновлять FOW
    if(cw->mi == cw->e[0]-1)
      finish_movement();
  }
  cw->index++;
}



int calc_damage(unit * a, unit * b){
  a = b;
  return(4); // random
}



void on_reach_enemy(){
  //int damage = calc_damage(cw->attack_u1, cw->attack_u2);
  int damage = cw->e[3];

  unit * u1 = id2unit(cw->e[2]);
  unit * u2 = mp( neib(u1->mcrd, cw->e[3]) )->unit;

  //u2->health -= damage;
  if(u2->health <= 0) {
    kill_unit(u2);
    fill_map(u1);
  }
  u1->can_attack = false;
}




void shoot_attack(){
  unit * u1 = id2unit(cw->e[2]);
  unit * u2 = id2unit(cw->e[3]);
  scrd a = u1->scrd;
  scrd b = u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(cw->index >= steps){
    //int dmg = calc_damage(cw->attack_u1, cw->attack_u2);
    int dmg = 1;
    u2->health -= dmg;
    if(u2->health <= 0) {
      kill_unit(u2);
      fill_map(u1);
    }
    u1->can_attack = false;
    cw->mode = MODE_SELECT;
  }
}



void attack_logic() {
  if(cw->e[1]==EVENT_RANGE)
    shoot_attack();
  if(cw->e[1]==EVENT_MELEE){
    if(cw->index==STEPS/2) on_reach_enemy();
    if(cw->index==STEPS)   cw->mode = MODE_SELECT;
  }
  cw->index++;
}



void updatefog(int plr){
  FOR_EACH_TILE{
    mp(mc)->fog=0;
    FOR_EACH_UNIT{
      if(u->player == plr
      && mdist(mc, u->mcrd) <= u->type->see)
        mp(mc)->fog++;
    }
  }
}



void logic(){
  if(cw->mode==MODE_MOVE)   move_logic();
  if(cw->mode==MODE_ATTACK) attack_logic();

  if(cw->mode==MODE_SELECT && cw->event_queue->count>0){
    event * e = (event*)l_deq(cw->event_queue);
    cw->e = calloc(e->data[0], sizeof(int));
    for(int i=0; i<e->data[0]; i++)
      cw->e[i] = e->data[i];

    cw->index = 0;

    if(cw->e[1] == EVENT_MOVE){
      cw->mode = MODE_MOVE;
      mp(id2unit(cw->e[2])->mcrd)->unit = NULL;
      cw->mi = 2;
    }
    if(e->data[1] == EVENT_MELEE) cw->mode = MODE_ATTACK;
    if(e->data[1] == EVENT_RANGE) cw->mode = MODE_ATTACK;
  }
}



