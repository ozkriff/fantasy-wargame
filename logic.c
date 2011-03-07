

// change tile type
void change_tile(mcrd m){
  if(mp(m)->type ++ == 4)
    mp(m)->type = 0;
  if(worlds[0].selunit) fill_map(worlds[0].selunit);
}



void select_next_unit(){
  do{
    if(worlds[0].selunit && worlds[0].selunit != (unit*)l_last(worlds[0].units))
      worlds[0].selunit = (unit*)l_next(worlds[0].selunit);
    else
      worlds[0].selunit = (unit*)l_first(worlds[0].units);
  }while(worlds[0].selunit->player != player);
  fill_map(worlds[0].selunit);
}



void select_unit(mcrd m){
  worlds[0].selunit = mp(m)->unit;
  fill_map(worlds[0].selunit);
}



void kill_unit(unit * u){
  mp(u->mcrd)->unit = NULL;
  if(u == worlds[0].selunit) worlds[0].selunit = NULL;
  free( l_rem(worlds[0].units, u) );
}



// type: 0-рукопашаня, 1-стрельба, 2-метание, 3-таран
// u1 - атакующий, u2 - защищающийся
void start_attack(unit * u1, unit * u2, int type) {
  worlds[0].mode         = MODE_ATTACK;
  worlds[0].attack_u1    = u1;
  worlds[0].attack_u2    = u2;
  worlds[0].attack_crd   = worlds[0].attack_u2->mcrd;
  worlds[0].attack_index = 0;
  mp(worlds[0].attack_u1->mcrd)->unit = NULL;

  //0-рукопашная
  if(type==0){
    worlds[0].attack_stage      = 0; // наступление
    worlds[0].attack_is_shoot   = false;
    worlds[0].attack_is_counter = false;
  }
  //1-стрельба 
  if(type==1){
    worlds[0].attack_shoot_index = 0;
    worlds[0].attack_is_shoot    = true;
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

  worlds[0].move_unit = u;
  mp(u->mcrd)->unit = NULL;

  get_path(m);
  worlds[0].mode = MODE_MOVE;
  worlds[0].move_tile = (mnode*)l_first(worlds[0].path);
  worlds[0].move_index = 0;
}



void finish_movement(){
  worlds[0].mode = MODE_SELECT;

  worlds[0].move_unit->mcrd = worlds[0].move_tile->crd;
  worlds[0].move_unit->scrd = map2scr(worlds[0].move_tile->crd);
  mp(worlds[0].move_unit->mcrd)->unit = worlds[0].move_unit;

  if(find_feature(worlds[0].move_unit, FEATURE_IGNR))
    worlds[0].move_unit->mvp -= mp(worlds[0].move_unit->mcrd)->cost;
  else
    worlds[0].move_unit->mvp = 0;

  clear_path();
  fill_map(worlds[0].move_unit);

  worlds[0].move_unit = NULL;
}



void ambush(){
  mnode * mnd = (mnode*) l_next(worlds[0].move_tile);
  unit * u = mp(mnd->crd)->unit;
  if( u && u->player != player){
    // this unit will be attacked!
    unit * xxx = worlds[0].move_unit;
    finish_movement();
    start_attack(u, xxx, 0);

    // а можно разрешать окнтратаку, НО
    // но добавить специальный омдификатор "попал в засаду"
    // его действие будет прекращаться после первого боя
    // он будет уменьшать защиту и атаку
    
    // стыдно. исправить. что бы враг не ответил!
    worlds[0].attack_is_counter = 1;
    return;
  }
}



void move_logic(){
  if(worlds[0].move_index == STEPS){
    worlds[0].move_index = 0;
    worlds[0].move_tile = (mnode*)l_next(worlds[0].move_tile);
    if(worlds[0].move_tile == (mnode*)l_last(worlds[0].path))
      finish_movement();
    else
      ambush();
  }
  worlds[0].move_index++;
}



int calc_damage(unit * a, unit * b){
  a = b;
  return(4); // random
}



void on_reach_enemy(){
  int damage = calc_damage(worlds[0].attack_u1, worlds[0].attack_u2);
  worlds[0].attack_u2->health -= damage;
  if(worlds[0].attack_u2->health <= 0) {
    kill_unit(worlds[0].attack_u2);
    worlds[0].attack_u2 = NULL;
    fill_map(worlds[0].attack_u1);
  }
  worlds[0].attack_u1->can_attack = false;
  // теперь возвращаемся на тайл
  worlds[0].attack_stage = 1;
}



void try_retreat(){
  for(int i=0; i<6; i++){
    //mcrd n = neib(attack_u1->mcrd, i);
    mcrd n = neib(worlds[0].attack_u1->mcrd, neib2(i));
    if(!mp(n)->unit){
      worlds[0].attack_crd = n;
      worlds[0].attack_stage = 2;
      return;
    }
  }
}



void start_counterattack(){
  start_attack(worlds[0].attack_u2, worlds[0].attack_u1, 0);
   worlds[0].attack_is_counter = true;
}



void on_return_after_attack(){
  mp(worlds[0].attack_u1->mcrd)->unit = worlds[0].attack_u1;

  if(worlds[0].attack_is_counter || !worlds[0].attack_u2){
    if(worlds[0].attack_u1->health < worlds[0].attack_u1->type->health){
      mp(worlds[0].attack_u1->mcrd)->unit = NULL;
      worlds[0].attack_crd = worlds[0].attack_u1->mcrd;
      try_retreat();
    }else{
      worlds[0].attack_u1 = worlds[0].attack_u2 = NULL;
      worlds[0].mode = MODE_SELECT;
    }
  }else{
    start_counterattack();
  }
}



void on_done_retreat(){
  mp(worlds[0].attack_crd)->unit = worlds[0].attack_u1;
  worlds[0].attack_u1->mcrd = worlds[0].attack_crd;
  worlds[0].attack_u1 = worlds[0].attack_u2 = NULL;
  worlds[0].mode = MODE_SELECT;
}



void shoot_attack(){
  scrd a = worlds[0].attack_u1->scrd;
  scrd b = worlds[0].attack_u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(worlds[0].attack_shoot_index >= steps){
    int dmg = calc_damage(worlds[0].attack_u1, worlds[0].attack_u2);
    worlds[0].attack_u2->health -= dmg;
    if(worlds[0].attack_u2->health <= 0) {
      kill_unit(worlds[0].attack_u2);
      fill_map(worlds[0].attack_u1);
    }
    mp(worlds[0].attack_u1->mcrd)->unit = worlds[0].attack_u1;
    worlds[0].attack_u1->can_attack = false;
    worlds[0].attack_u1 = worlds[0].attack_u2 = NULL;
    worlds[0].mode = MODE_SELECT;
  }

  worlds[0].attack_shoot_index++;
}



void attack_logic() {
  if(worlds[0].attack_is_shoot)
    shoot_attack();
  else{
    if(worlds[0].attack_stage==0 && worlds[0].attack_index==STEPS/2+1)
      on_reach_enemy();
    if(worlds[0].attack_stage==1 && worlds[0].attack_index==0)
      on_return_after_attack();
    if(worlds[0].attack_stage==2 && worlds[0].attack_index==STEPS)
      on_done_retreat();

    if(worlds[0].attack_stage==0) worlds[0].attack_index++;
    if(worlds[0].attack_stage==1) worlds[0].attack_index--;
    if(worlds[0].attack_stage==2) worlds[0].attack_index++;
  }
}



void logic(){
  if(worlds[0].mode==MODE_MOVE)	  move_logic();
  if(worlds[0].mode==MODE_ATTACK) attack_logic();
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



