
bool checkunitsleft(){
  FOR_EACH_UNIT {
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

  FOR_EACH_UNIT{
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



// rename
void ambush(){
  mnode * t = (mnode*)l_first(cw->path);
  while(t){
    unit * u = mp(t->crd)->unit ;
    if(u && u->player!=player){
      int dir = mcrd2index(t->crd, ((mnode*)l_prev(t))->crd);
      int data[5] = {5, EVENT_MELEE, u->id, dir, 1};
      add_event(data);
      break;
    }
    t = (mnode*)l_next(t);
  }
}



void mv(mcrd m){
  get_path(m);
  int d[100] = {0, EVENT_MOVE, cw->selunit->id};

  int len=3;
  mnode * t = (mnode*)(l_first(cw->path));
  while(l_next(t)){
    unit * u = mp( ((mnode*)l_next(t))->crd )->unit;
    if(u && u->player!=cw->selunit->player)
      break; // ambush!
    mnode * n = (mnode*)l_next(t);
    d[len] = mcrd2index(t->crd, n->crd);
    t = (mnode*)l_next(t);
    len++;
  }
  d[0] = len;
  //for(int i=0; i<10; i++) printf("%i ", d[i]); puts("");
  add_event(d);

  ambush();
}



void support_range(mcrd m, unit * u){
  for(int i=0; i<6; i++){
    mcrd n = neib(m, i);
    if( mp(n)->unit && mp(n)->unit->player == u->player ){
      feature * rng = find_feature(mp(n)->unit, FEATURE_RNG);
      if(rng){
        int data[5] = {5, EVENT_RANGE,
            mp(n)->unit->id, cw->selunit->id, 2};
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
  int data[5] = {5, EVENT_MELEE, cw->selunit->id, dir, 3};
  add_event(data);

  /*проверка на то, что оппонент выживет*/
  if(u->health - data[4] <= 0)
    return;

  // а это уже контратака
  int dir2 = mcrd2index(u->mcrd, cw->selunit->mcrd);
  int data2[5] = {5, EVENT_MELEE, u->id, dir2, 3};
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
    if( ! mp(neib(m, d))->unit )
      break;
  }
  if(d==6){
    return;
  }else{
    int data3[4] = {4, EVENT_MOVE, u->id, d};
    add_event(data3);
  }
}



void attack(mcrd m, unit * u){
  feature * rng = find_feature(cw->selunit, FEATURE_RNG);
  if(rng){
    if(mdist(cw->selunit->mcrd, m) <= rng->data.rng.range){
      int data[5] = {5, EVENT_RANGE,
          cw->selunit->id, u->id, 2};
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
  unit * u = mp(m)->unit;

  if(u && u->player == player){
    select_unit(m);
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


