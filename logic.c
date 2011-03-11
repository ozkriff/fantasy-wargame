

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



// type: 0-рукопашаня, 1-стрельба, 2-метание, 3-таран
// u1 - атакующий, u2 - защищающийся
void start_attack(unit * u1, unit * u2, int type) {
  cw->mode         = MODE_ATTACK;
  cw->attack_u1    = u1;
  cw->attack_u2    = u2;
  cw->attack_crd   = cw->attack_u2->mcrd;
  cw->attack_index = 0;
  mp(cw->attack_u1->mcrd)->unit = NULL;

  //0-рукопашная
  if(type==0){
    cw->attack_stage      = 0; // наступление
    cw->attack_is_shoot   = false;
    cw->attack_is_counter = false;
  }
  //1-стрельба 
  if(type==1){
    cw->attack_shoot_index = 0;
    cw->attack_is_shoot    = true;
  }
  //2-метание
  if(type==2){
    /*todo*/;
  }
  //3-таран
  if(type==3){
    /*todo*/;
  }
}



void start_moving(unit * u, mcrd m) {
  if(mp(m)->cost > u->type->mvp || u->mvp == 0)
    return;

  cw->move_unit = u;
  mp(u->mcrd)->unit = NULL;

  get_path(m);
  cw->mode = MODE_MOVE;
  cw->move_tile = (mnode*)l_first(cw->path);
  cw->move_index = 0;
}



void finish_movement(){
  cw->mode = MODE_SELECT;

  cw->move_unit->mcrd = cw->move_tile->crd;
  cw->move_unit->scrd = map2scr(cw->move_tile->crd);
  mp(cw->move_unit->mcrd)->unit = cw->move_unit;

  if(find_feature(cw->move_unit, FEATURE_IGNR))
    cw->move_unit->mvp -= mp(cw->move_unit->mcrd)->cost;
  else
    cw->move_unit->mvp = 0;

  clear_path();
  fill_map(cw->move_unit);

  cw->move_unit = NULL;
}



// а можно разрешать окнтратаку, НО
// но добавить специальный омдификатор "попал в засаду"
// его действие будет прекращаться после первого боя
// он будет уменьшать защиту и атаку
void ambush(){
  mnode * mnd = (mnode*) l_next(cw->move_tile);
  unit * u = mp(mnd->crd)->unit;
  if( u && u->player != cw->move_unit->player){
    // this unit will be attacked!
    unit * xxx = cw->move_unit;
    finish_movement();
    start_attack(u, xxx, 0);
    
    // стыдно. исправить. что бы враг не ответил!
    cw->attack_is_counter = 1;
    return;
  }
}



void move_logic(){
  if(cw->move_index == STEPS){
    cw->move_index = 0;
    cw->move_tile = (mnode*)l_next(cw->move_tile);
    if(cw->move_tile == (mnode*)l_last(cw->path))
      finish_movement();
    else
      ambush();
  }
  cw->move_index++;
}



int calc_damage(unit * a, unit * b){
  a = b;
  return(4); // random
}



void on_reach_enemy(){
  int damage = calc_damage(cw->attack_u1, cw->attack_u2);
  cw->attack_u2->health -= damage;
  if(cw->attack_u2->health <= 0) {
    kill_unit(cw->attack_u2);
    cw->attack_u2 = NULL;
    fill_map(cw->attack_u1);
  }
  cw->attack_u1->can_attack = false;
  // теперь возвращаемся на тайл
  cw->attack_stage = 1;
}



void try_retreat(){
  for(int i=0; i<6; i++){
    //mcrd n = neib(attack_u1->mcrd, i);
    mcrd n = neib(cw->attack_u1->mcrd, neib2(i));
    if(!mp(n)->unit){
      cw->attack_crd = n;
      cw->attack_stage = 2;
      return;
    }
  }
}



void start_counterattack(){
  start_attack(cw->attack_u2, cw->attack_u1, 0);
   cw->attack_is_counter = true;
}



void on_return_after_attack(){
  mp(cw->attack_u1->mcrd)->unit = cw->attack_u1;

  if(cw->attack_is_counter
  || !cw->attack_u2 
  || find_feature(cw->attack_u1, FEATURE_NORETURN)){
    if(cw->attack_u1->health < cw->attack_u1->type->health){
      mp(cw->attack_u1->mcrd)->unit = NULL;
      cw->attack_crd = cw->attack_u1->mcrd;
      try_retreat();
    }else{
      cw->attack_u1 = cw->attack_u2 = NULL;
      cw->mode = MODE_SELECT;
    }
  }else{
    start_counterattack();
  }
}



void on_done_retreat(){
  mp(cw->attack_crd)->unit = cw->attack_u1;
  cw->attack_u1->mcrd = cw->attack_crd;
  cw->attack_u1 = cw->attack_u2 = NULL;
  cw->mode = MODE_SELECT;
}



void shoot_attack(){
  scrd a = cw->attack_u1->scrd;
  scrd b = cw->attack_u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(cw->attack_shoot_index >= steps){
    int dmg = calc_damage(cw->attack_u1, cw->attack_u2);
    cw->attack_u2->health -= dmg;
    if(cw->attack_u2->health <= 0) {
      kill_unit(cw->attack_u2);
      fill_map(cw->attack_u1);
    }
    mp(cw->attack_u1->mcrd)->unit = cw->attack_u1;
    cw->attack_u1->can_attack = false;
    cw->attack_u1 = cw->attack_u2 = NULL;
    cw->mode = MODE_SELECT;
  }

  cw->attack_shoot_index++;
}



void attack_logic() {
  if(cw->attack_is_shoot)
    shoot_attack();
  else{
    if(cw->attack_stage==0 && cw->attack_index==STEPS/2+1)
      on_reach_enemy();
    if(cw->attack_stage==1 && cw->attack_index==0)
      on_return_after_attack();
    if(cw->attack_stage==2 && cw->attack_index==STEPS)
      on_done_retreat();

    if(cw->attack_stage==0) cw->attack_index++;
    if(cw->attack_stage==1) cw->attack_index--;
    if(cw->attack_stage==2) cw->attack_index++;
  }
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
    if(e->type == EVENT_MOVE){
      unit * u = id2unit(e->data.mv.id);
      updatefog(u->player); 
      fill_map(u);
      updatefog(player); 
      start_moving(u, e->data.mv.dest);
    }
    if(e->type == EVENT_ATTACK){
      unit * u0 = id2unit(e->data.at.id0);
      unit * u1 = id2unit(e->data.at.id1);
      int type = e->data.at.type;
      start_attack(u0, u1, type);
    }
  }
}



