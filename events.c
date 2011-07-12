
bool checkunitsleft(){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id)
      return(true);
  }
  return(false);
}



void onspace(){
  /* change |c|urrent |w|orld */
  Node * nd;
  FOR_EACH_NODE(worlds, nd){
    if(cw == (World*)nd->d){
      if(nd->n)  cw = nd->n->d;  else  cw = worlds->h->d;
      break;
    }
  }

#if 0
  if(!checkunitsleft()){
    puts("WINNER!");
    exit(EXIT_SUCCESS);
  }
#endif

  selunit = NULL;

  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->player == cw->id){
      u->mvp = u->type->mvp;
      u->can_attack = true;
    }
  }

  updatefog(cw->id);
  
  /*select_next_unit(); */
}



void keys(SDL_Event E){
  if(mode!=MODE_SELECT) return;

  switch(E.key.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q:
      done = true;
      break;
    case SDLK_UP:    map_offset.y += 54; break;
    case SDLK_DOWN:  map_offset.y -= 54; break;
    case SDLK_LEFT:  map_offset.x += 72; break;
    case SDLK_RIGHT: map_offset.x -= 72; break;
    case SDLK_SPACE: onspace();          break;
    case SDLK_r:     change_tile(selhex);break;
    /*case SDLK_n:     select_next_unit(); break; */
    default: break;
  }
}



void mv(Unit * moving_unit, Mcrd m){
  get_path(m);

  Node * node;
  for(node=path->h; node->n; node=node->n){
    Mcrd * current = node->d;
    Mcrd * next    = node->n->d;

    Unit * u = find_unit_at( *next );
    if(u && u->player!=moving_unit->player){
      /* AMBUSH */
      Event_melee melee = {EVENT_MELEE, u->id,
          moving_unit->id, *current, 1};
      add_event((Event*)&melee);
      if(!is_local)
        send_melee(melee);
      break;
    }
    
    Event_move mv = {EVENT_MOVE, moving_unit->id, *next};
    add_event((Event*)&mv);
    if(!is_local)
      send_move(mv);
  }
}




/* [a]ttacker, [d]efender */
void support_range(Unit * a, Unit * d){
  int i;
  for(i=0; i<6; i++){
    /* [sup]porter */
    Unit * sup = find_unit_at( neib(d->mcrd, i) );
    if( sup && sup->player == d->player ){
      if(find_feature(sup, FEATURE_RNG)){
        Event_range range = {EVENT_RANGE, sup->id, a->id, a->mcrd, 2};
        add_event((Event*)&range);
        if(!is_local)
          send_range(range);
        if(a->health - range.dmg <= 0)
          return;
      }
    }
  }
}



/* [a]ttacker, [d]efender */
void attack_melee(Unit * a, Unit * d){
  Mcrd md = d->mcrd;
  /*Mcrd ma = a->mcrd; */

  /*огонь поддержки */
  support_range(a, d);

  /* собственно, это и есть заказанная атака */
  Event_melee melee =
      {EVENT_MELEE, a->id, d->id, d->mcrd, melee_attack_damage(a, d)};
  add_event((Event*)&melee);
  if(!is_local)
    send_melee(melee);

  /*проверка на то, что оппонент выживет*/
  if(d->health - melee.dmg <= 0)
    return;

  /* а это уже контратака */
  Event_melee melee2 =
      {EVENT_MELEE, d->id, a->id, a->mcrd, melee_return_damage(d, a)};
  add_event((Event*)&melee2);
  if(!is_local)
    send_melee(melee2);

  /* проверка на необходимость отступления-бегства */
  if(d->health - melee.dmg > d->type->health / 2)
    return;

  /* пытатья убежатьr в противоположном направлении или драться */
  int dir3; /* direction */
  for(dir3=0; dir3<6; dir3++){
    if( ! find_unit_at(neib(md, dir3)) )
      break;
  }
  if(dir3==6){
    attack_melee(d, a); /* паника! */
  }else{
    Event_move mv = {EVENT_MOVE, d->id, neib(md, dir3)};
    add_event((Event*)&mv);
    if(!is_local)
      send_move(mv);
  }
}



/* [a]ttacker, [d]efender */
void attack(Unit * a, Unit * d){
  Mcrd md = d->mcrd;
  Mcrd ma = a->mcrd;
  Feature * rng = find_feature(a, FEATURE_RNG);
  if(rng){
    if(mdist(ma, md) <= rng->data.rng.range){
      Event_range range = 
          {EVENT_RANGE, a->id, d->id, d->mcrd, range_damage(a, d)};
      add_event((Event*)&range);
      if(!is_local)
        send_range(range);
    }
  }else{
    if(mdist(ma, md) <= 1){
      attack_melee(a, d);
    }
  }
}


void mouseclick(SDL_Event E){
  if(mode!=MODE_SELECT) return;

  Mcrd m = scr2map(mk_scrd(E.button.x, E.button.y));
  Unit * u = find_unit_at(m);

  if(u && u->player == cw->id){
    selunit = u;
    fill_map(selunit);
  }else if(selunit){
    if( !u || (u && (is_invis(u)||!mp(m)->fog)) ){
      if(mp(m)->cost <= selunit->mvp)
        mv(selunit, m);
      return;
    }
    if(u && u->player!=cw->id && selunit 
    && selunit->can_attack
    && !is_invis(u) && mp(m)->fog > 0){
      attack(selunit, u);
    }
  }
}



void mousemove(SDL_Event E){
  selhex = scr2map(mk_scrd(E.button.x, E.button.y));
}



void events() {
  SDL_Event E;
  while(SDL_PollEvent(&E)){
    if(E.type==SDL_QUIT)            done = true;
    if(E.type==SDL_KEYUP)           keys(E);
    if(E.type==SDL_MOUSEBUTTONDOWN) mouseclick(E);
    if(E.type==SDL_MOUSEMOTION)     mousemove(E);
    if(E.type==SDL_VIDEORESIZE){
      Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
      screen = SDL_SetVideoMode(E.resize.w,
          E.resize.h, 32, flags);
    }
  }
}


