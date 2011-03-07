

// change tile type
void change_tile(mcrd m){
  if(mp(m)->type ++ == 4)
    mp(m)->type = 0;
  if(selunit) fill_map(selunit);
}



void select_next_unit(){
  do{
    if(selunit && selunit != (unit*)l_last(units))
      selunit = (unit*)l_next(selunit);
    else
      selunit = (unit*)l_first(units);
  }while(selunit->player != player);
  fill_map(selunit);
}



void select_unit(mcrd m){
  selunit = mp(m)->unit;
  fill_map(selunit);
}



void kill_unit(unit * u){
  mp(u->mcrd)->unit = NULL;
  if(u == selunit) selunit = NULL;
  free( l_rem(units, u) );
}



// type: 0-рукопашаня, 1-стрельба, 2-метание, 3-таран
// u1 - атакующий, u2 - защищающийся
void start_attack(unit * u1, unit * u2, int type) {
  mode         = MODE_ATTACK;
  attack_u1    = u1;
  attack_u2    = u2;
  attack_crd   = attack_u2->mcrd;
  attack_index = 0;
  mp(attack_u1->mcrd)->unit = NULL;

  //0-рукопашная
  if(type==0){
    attack_stage      = 0; // наступление
    attack_is_shoot   = false;
    attack_is_counter = false;
  }
  //1-стрельба 
  if(type==1){
    attack_shoot_index = 0;
    attack_is_shoot    = true;
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

  move_unit = u;
  mp(u->mcrd)->unit = NULL;

  get_path(m);
  mode = MODE_MOVE;
  move_tile = (mnode*)l_first(path);
  move_index = 0;
}



void finish_movement(){
  mode = MODE_SELECT;

  move_unit->mcrd = move_tile->crd;
  move_unit->scrd = map2scr(move_tile->crd);
  mp(move_unit->mcrd)->unit = move_unit;

  if(find_feature(move_unit, FEATURE_IGNR))
    move_unit->mvp -= mp(move_unit->mcrd)->cost;
  else
    move_unit->mvp = 0;

  clear_path();
  fill_map(move_unit);

  move_unit = NULL;
}



void ambush(){
  mnode * mnd = (mnode*) l_next(move_tile);
  unit * u = mp(mnd->crd)->unit;
  if( u && u->player != player){
    // this unit will be attacked!
    unit * xxx = move_unit;
    finish_movement();
    start_attack(u, xxx, 0);

    // а можно разрешать окнтратаку, НО
    // но добавить специальный омдификатор "попал в засаду"
    // его действие будет прекращаться после первого боя
    // он будет уменьшать защиту и атаку
    
    // стыдно. исправить. что бы враг не ответил!
    attack_is_counter = 1;
    return;
  }
}



void move_logic(){
  if(move_index == STEPS){
    move_index = 0;
    move_tile = (mnode*)l_next(move_tile);
    if(move_tile == (mnode*)l_last(path))
      finish_movement();
    else
      ambush();
  }
  move_index++;
}



int calc_damage(unit * a, unit * b){
  a = b;
  return(4); // random
}



void on_reach_enemy(){
  int damage = calc_damage(attack_u1, attack_u2);
  attack_u2->health -= damage;
  if(attack_u2->health <= 0) {
    kill_unit(attack_u2);
    attack_u2 = NULL;
    fill_map(attack_u1);
  }
  attack_u1->can_attack = false;
  // теперь возвращаемся на тайл
  attack_stage = 1;
}



void try_retreat(){
  for(int i=0; i<6; i++){
    //mcrd n = neib(attack_u1->mcrd, i);
    mcrd n = neib(attack_u1->mcrd, neib2(i));
    if(!mp(n)->unit){
      attack_crd = n;
      attack_stage = 2;
      return;
    }
  }
}



void start_counterattack(){
  start_attack(attack_u2, attack_u1, 0);
  attack_is_counter=true;
}



void on_return_after_attack(){
  mp(attack_u1->mcrd)->unit = attack_u1;

  if(attack_is_counter || !attack_u2){
    if(attack_u1->health<attack_u1->type->health){
      mp(attack_u1->mcrd)->unit = NULL;
      attack_crd = attack_u1->mcrd;
      try_retreat();
    }else{
      attack_u1 = attack_u2 = NULL;
      mode = MODE_SELECT;
    }
  }else{
    start_counterattack();
  }
}



void on_done_retreat(){
  mp(attack_crd)->unit = attack_u1;
  attack_u1->mcrd = attack_crd;
  attack_u1 = attack_u2 = NULL;
  mode = MODE_SELECT;
}



void shoot_attack(){
  scrd a = attack_u1->scrd;
  scrd b = attack_u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(attack_shoot_index >= steps){
    int dmg = calc_damage(attack_u1, attack_u2);
    attack_u2->health -= dmg;
    if(attack_u2->health <= 0) {
      kill_unit(attack_u2);
      fill_map(attack_u1);
    }
    mp(attack_u1->mcrd)->unit = attack_u1;
    attack_u1->can_attack = false;
    attack_u1 = attack_u2 = NULL;
    mode = MODE_SELECT;
  }

  attack_shoot_index++;
}



void attack_logic() {
  if(attack_is_shoot)
    shoot_attack();
  else{
    if(attack_stage==0 && attack_index==STEPS/2+1)
      on_reach_enemy();
    if(attack_stage==1 && attack_index==0)
      on_return_after_attack();
    if(attack_stage==2 && attack_index==STEPS)
      on_done_retreat();

    if(attack_stage==0) attack_index++;
    if(attack_stage==1) attack_index--;
    if(attack_stage==2) attack_index++;
  }
}



void logic(){
  if(mode==MODE_MOVE)	  move_logic();
  if(mode==MODE_ATTACK)	attack_logic();
}



void updatefog(){
  FOR_EACH_TILE{
    mp(mc)->fog=0;
    FOR_EACH_UNIT{
      if(u->player == player
      && mdist(mc, u->mcrd) <= u->type->see)
        mp(mc)->fog++;
    }
  }
}



