
bool checkunitsleft(){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
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

  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
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
    //case SDLK_n:     select_next_unit(); break;
    default: break;
  }
}



void mv(Mcrd m){
  get_path(m);
  int d[5] = {5, EVENT_MOVE, cw->selunit->id};

  Node * node;
  for(node=cw->path->h; node->n; node=node->n){
    Mcrd * current = node->d;
    Mcrd * next    = node->n->d;

    Unit * u = find_unit_at( *next );
    if(u && u->player!=cw->selunit->player){
      // AMBUSH
      int dir = mcrd2index(*next, *current);
      int data[5] = {5, EVENT_MELEE, u->id, dir, 1};
      add_event(data);
      break;
    }
    
    d[3] = next->x;
    d[4] = next->y;
    add_event(d);
  }
}




// [a]ttacker, [d]efender
void support_range(Unit * a, Unit * d){
  for(int i=0; i<6; i++){
    // [sup]porter
    Unit * sup = find_unit_at( neib(d->mcrd, i) );
    if( sup && sup->player == d->player ){
      if(find_feature(sup, FEATURE_RNG)){
        int data[5] = {5, EVENT_RANGE, sup->id, a->id, 2};
        add_event(data);
        if(a->health - data[4] <= 0)
          return;
      }
    }
  }
}



// [a]ttacker, [d]efender
void attack_melee(Unit * a, Unit * d){
  Mcrd md = d->mcrd;
  Mcrd ma = a->mcrd;

  //огонь поддержки
  support_range(a, d);

  // собственно, это и есть заказанная атака
  int dir = mcrd2index(ma, md);
  int data[5] = {5, EVENT_MELEE, a->id,
      dir, melee_attack_damage(a, d)};
  add_event(data);

  /*проверка на то, что оппонент выживет*/
  if(d->health - data[4] <= 0)
    return;

  // а это уже контратака
  int dir2 = mcrd2index(md, ma);
  int data2[5] = {5, EVENT_MELEE, d->id, dir2,
      melee_return_damage(d, a)};
  add_event(data2);

  // проверка на необходимость отступления-бегства
  if(d->health - data[4] > d->type->health / 2)
    return;

  // пытатья убежатьr в противоположном направлении или драться
  int dir3; // direction
  for(dir3=0; dir3<6; dir3++){
    if( ! find_unit_at(neib(md, dir3)) )
      break;
  }
  if(dir3==6){
    attack_melee(d, a); // паника!
  }else{
    Mcrd dest = neib(md, dir3);
    int data3[5] = {5, EVENT_MOVE, d->id, dest.x, dest.y};
    add_event(data3);
  }
}



// [a]ttacker, [d]efender
void attack(Unit * a, Unit * d){
  Mcrd md = d->mcrd;
  Mcrd ma = a->mcrd;
  Feature * rng = find_feature(a, FEATURE_RNG);
  if(rng){
    if(mdist(ma, md) <= rng->data.rng.range){
      int data[5] = {5, EVENT_RANGE,
          a->id, d->id,
          range_damage(a, d)};
      add_event(data);
    }
  }else{
    if(mdist(ma, md) <= 1){
      attack_melee(a, d);
    }
  }
}


void mouseclick(SDL_Event E){
  if(cw->mode!=MODE_SELECT) return;

  Mcrd m = scr2map((Scrd){E.button.x, E.button.y});
  Unit * u = find_unit_at(m);

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
      attack(cw->selunit, u);
    }
  }
}



void mousemove(SDL_Event E){
  cw->selhex = scr2map((Scrd){E.button.x, E.button.y});
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


