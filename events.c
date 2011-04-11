
bool checkunitsleft(){
  unit * u;
  FOR_EACH_UNIT(u){
    if(u->player == player)
      return(true);
  }
  return(false);
}



void onspace(){
  player++; if(player==3) player=0;

  {
    cw++;
    if(cw == &worlds[players_count])
      cw = &worlds[0];
  }

#if 0
  if(!checkunitsleft()){
    puts("WINNER!");
    exit(EXIT_SUCCESS);
  }
#endif

  cw->selunit = NULL;

  unit * u;
  FOR_EACH_UNIT(u){
    if(u->player == player){
      u->mvp = u->type->mvp;
      u->can_attack = true;
    }
  }
  
  //select_next_unit();
}



void keys(SDL_Event E){
  if(cw->mode!=MODE_SELECT) return;

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
    case SDLK_r:     change_tile(cw->selhex);break;
    case SDLK_n:     select_next_unit(); break;
    default: break;
  }
}



void mv(mcrd m){
  get_path(m);
  int d[5] = {5, EVENT_MOVE, cw->selunit->id};

  mnode * current = (mnode*)(l_first(cw->path));
  while(l_next(current)){
    mnode * next = (mnode*)(l_next(current));

    unit * u = find_unit_at( next->crd );
    if(u && u->player!=cw->selunit->player){
      // AMBUSH
      int dir = mcrd2index(next->crd, current->crd);
      int data[5] = {5, EVENT_MELEE, u->id, dir, 1};
      add_event(data);
      break;
    }
    
    d[3] = next->crd.x;
    d[4] = next->crd.y;
    add_event(d);

    current = next;
  }
}




void support_range(mcrd m, unit * u){
  for(int i=0; i<6; i++){
    unit * mu = find_unit_at( neib(m,i) );
    if( mu && mu->player == u->player ){
      feature * rng = find_feature(mu, FEATURE_RNG);
      if(rng){
        int data[5] = {5, EVENT_RANGE,
            mu->id, cw->selunit->id, 2};
        add_event(data);
        if(cw->selunit->health - data[4] <= 0)
          return;
      }
    }
  }
}



void attack_melee(mcrd m, unit * u){
  //огонь поддержки
  support_range(m, u);

  // собственно, это и есть заказанная атака
  int dir = mcrd2index(cw->selunit->mcrd, u->mcrd);
  int data[5] = {5, EVENT_MELEE, cw->selunit->id,
      dir, melee_attack_damage(cw->selunit, u)};
  add_event(data);

  /*проверка на то, что оппонент выживет*/
  if(u->health - data[4] <= 0)
    return;

  // а это уже контратака
  int dir2 = mcrd2index(u->mcrd, cw->selunit->mcrd);
  int data2[5] = {5, EVENT_MELEE, u->id, dir2,
      melee_return_damage(u, cw->selunit)};
  add_event(data2);

  //добавить проверку на необходимость отступления-бегства
  //if(отстпать?)
  if(0)
    return;

  // направление нужно выбирать в цикле, а не так как сейчас
  // криво-криво. переделать! должен пытатья убежатьr
  // в противоположном направлении
  int d; // direction
  for(d=0; d<6; d++){
    if( ! find_unit_at(neib(m, d)) )
      break;
  }
  if(d==6){
    mcrd dest = neib(m, rand()%6);
    attack_melee(dest, u);
  }else{
    mcrd dest = neib(m, d);
    int data3[5] = {4, EVENT_MOVE, u->id, dest.x, dest.y};
    add_event(data3);
  }
}



void attack(mcrd m, unit * u){
  feature * rng = find_feature(cw->selunit, FEATURE_RNG);
  if(rng){
    if(mdist(cw->selunit->mcrd, m) <= rng->data.rng.range){
      int data[5] = {5, EVENT_RANGE,
          cw->selunit->id, u->id,
          range_damage(cw->selunit, u)};
      add_event(data);
    }
  }else{
    if(mdist(cw->selunit->mcrd, m) <= 1){
      attack_melee(m, u);
    }
  }
}


void mouseclick(SDL_Event E){
  if(cw->mode!=MODE_SELECT) return;

  mcrd m = scr2map((scrd){E.button.x, E.button.y});
  unit * u = find_unit_at(m);

  if(u && u->player == player){
    cw->selunit = u;
    fill_map(cw->selunit);
  }else if(cw->selunit){
    if( !u || (u && (is_invis(u)||!mp(m)->fog)) ){
      if(mp(m)->cost <= cw->selunit->mvp)
        mv(m);
      return;
    }
    if(u && u->player!=player && cw->selunit 
    && cw->selunit->can_attack
    && !is_invis(u) && mp(m)->fog > 0){
      attack(m, u);
    }
  }
}



void mousemove(SDL_Event E){
  cw->selhex = scr2map((scrd){E.button.x, E.button.y});
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


