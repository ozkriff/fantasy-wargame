

// change tile type
void change_tile(Mcrd m){
  if(mp(m)->type ++ == 4)
    mp(m)->type = 0;
  if(selunit) fill_map(selunit);
}



/*
void select_next_unit(){
  do{
    if(selunit && selunit != l_last(cw->units->t->d)
      selunit = (Unit*)l_next(selunit);
    else
      selunit = (Unit*)l_first(units);
  }while(selunit->player != player);
  fill_map(selunit);
}
*/


void kill_unit(Unit * u){
  if(u == selunit) selunit = NULL;

  Node * nd;
  FOR_EACH_NODE(cw->units, nd){
    Unit * iter = nd->d;
    if(iter==u){
      // удаляем список свойств
      Node * feature_node;
      FOR_EACH_NODE(u->features, feature_node)
        l_delete_node(u->features, feature_node);

      l_delete_node(cw->units, nd);
      return;
    }
  }
}



// вызывается после перемещеня юнита
void upd_fog_2 (Unit * u){
  Mcrd mc;
  FOR_EACH_MCRD(mc){
    if(mdist(mc, u->mcrd) <= u->type->see)
      mp(mc)->fog++;
  }
}



void finish_movement(){
  Unit * u = id2unit(e.move.u);
  //u->mvp -= mp(u->mcrd)->cost;
  u->mvp -= u->type->ter_mvp[mp(u->mcrd)->type];
  u->mcrd = e.move.dest;
  if(selunit==u) fill_map(u);
  u->scrd = map2scr(u->mcrd);
  if(u->player==player)
    upd_fog_2(u);
  mode = MODE_SELECT;
}

void move_logic(){
  if(eindex == STEPS-1)
    finish_movement();
  eindex++;
}



// dmg = E(attack_skill) / E(defence_skill) * HPi/HPo * K
// основная разница в коеффициентах и бонусах поверхности:
// при рукопашной бонусы берутся с клетки защищающегося
// при стрельбе каждый юнит перед со своей клетки

// a - attacking unit, b - defender
int melee_attack_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[mp(b->mcrd)->type];
  int terrain_defence = b->type->ter_def[mp(b->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;
  
  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(4 * attack/defence);
}



// a - defender, a - attacking unit
int melee_return_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[mp(a->mcrd)->type];
  int terrain_defence = b->type->ter_def[mp(a->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;
  
  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(3 * attack/defence);
}


// a - shooting unit, a - target
int range_damage (Unit * a, Unit * b){
  int terrain_attack  = a->type->ter_atk[mp(a->mcrd)->type];
  int terrain_defence = b->type->ter_def[mp(b->mcrd)->type];

  float hp_a = (float)a->health / a->type->health;
  float hp_b = (float)a->health / a->type->health;
  
  int attack  = a->type->attack  + terrain_attack  * hp_a;
  int defence = b->type->defence + terrain_defence * hp_b;

  return(3 * attack/defence);
}





void on_reach_enemy(){
  int damage = e.melee.dmg;

  Unit * u1 = id2unit(e.melee.a);
  Unit * u2 = id2unit(e.melee.d);

  u2->health -= damage;
  if(u2->health <= 0) {
    kill_unit(u2);
    fill_map(u1);
  }
  u1->can_attack = false;
}



void on_arrow_hit(){
  Unit * u1 = id2unit(e.range.a);
  Unit * u2 = id2unit(e.range.d);
  int dmg = e.range.dmg;
  u2->health -= dmg;
  if(u2->health <= 0) {
    kill_unit(u2);
    fill_map(u1);
  }
  u1->can_attack = false;
}




void shoot_attack(){
  Unit * u1 = id2unit(e.range.a);
  Unit * u2 = id2unit(e.range.d);
  Scrd a = u1->scrd;
  Scrd b = u2->scrd;
  int steps = sdist(a,b) / 6;

  //стрела долетела
  if(eindex >= steps){
    on_arrow_hit();
    mode = MODE_SELECT;
  }
}



void attack_logic() {
  if(e.t==EVENT_RANGE)
    shoot_attack();
  if(e.t==EVENT_MELEE){
    if(eindex==STEPS/2) on_reach_enemy();
    if(eindex==STEPS)   mode = MODE_SELECT;
  }
  eindex++;
}



void updatefog(int plr){
  Mcrd mc;
  FOR_EACH_MCRD(mc){
    mp(mc)->fog=0;
    Node * node;
    FOR_EACH_NODE(cw->units, node){
      Unit * u = node->d;
      if(u->player == plr
      && mdist(mc, u->mcrd) <= u->type->see)
        mp(mc)->fog++;
    }
  }
}


bool is_move_vsbl (Event e){
  Unit * u = id2unit(e.move.u);
  // Не мешает ли нам туман войны?
  bool fow = mp(e.move.dest)->fog || mp(u->mcrd)->fog;
  // Юнит спрятался?
  bool hidden = is_invis(u) && u->player!=player;
  if(!hidden&&fow)
    return(true);
  else
    return(false);
}


bool is_melee_visible (Event e){
  Unit * a = id2unit( e.melee.a );

  if( mp(a->mcrd)->fog )    return(true);
  if( mp(e.melee.md)->fog ) return(true);

  return(false);
}



bool is_range_visible (Event e){
  Unit * a = id2unit( e.range.a );

  if( mp(a->mcrd)->fog )    return(true);
  if( mp(e.range.md)->fog ) return(true);

  return(false);
}



void logic(){
  if(mode==MODE_MOVE)   move_logic();
  if(mode==MODE_ATTACK) attack_logic();

  if(mode==MODE_SELECT && cw->eq->count>0){
    Event * tmp = l_dequeue(cw->eq);
    e = *tmp;
    free(tmp);

    eindex = 0;

    if(e.t == EVENT_MOVE) {
      if( is_move_vsbl(e) ){
        mode = MODE_MOVE;
      }else{
        finish_movement(); logic();
      }
    }
    if(e.t == EVENT_MELEE){
      if( is_melee_visible(e) )
        mode = MODE_ATTACK;
      else{
        on_reach_enemy(); logic();
      }
    }
    if(e.t == EVENT_RANGE){
      if( is_range_visible(e) )
        mode = MODE_ATTACK;
      else{
        on_arrow_hit(); logic();
      }
    }
  }
}



