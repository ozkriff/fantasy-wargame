
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



void mouseclick(SDL_Event E){
  if(cw->mode!=MODE_SELECT) return;

  mcrd m = scr2map((scrd){E.button.x, E.button.y});
  unit * u = mp(m)->unit;

  if(u && u->player == player){
    select_unit(m);
  }else if(cw->selunit){
    if(!u || (u && (is_invis(u)||!mp(m)->fog))){
      //start_moving(cw->selunit, m);

      event_move e = {cw->selunit->id, m};
      add_event(EVENT_MOVE, (event_data*)&e );
      //print_event_queue(cw->event_queue);
      
      return;
    }

    if(u && u->player!=player && cw->selunit 
    && cw->selunit->can_attack
    && !is_invis(u) && mp(m)->fog > 0){
      feature * rng = find_feature(cw->selunit, FEATURE_RNG);
      if(rng){
        if(mdist(cw->selunit->mcrd, m) <= rng->data.rng.range){
          //start_attack(cw->selunit, mp(m)->unit, 1);
          event_attack e = {1, cw->selunit->id, mp(m)->unit->id};
          add_event(EVENT_ATTACK, (event_data*)&e );
        }
      }else{
        if(mdist(cw->selunit->mcrd, m) <= 1){
          //start_attack(cw->selunit, mp(m)->unit, 0);
          event_attack e = {0, cw->selunit->id, mp(m)->unit->id};
          add_event(EVENT_ATTACK, (event_data*)&e );
        }
      }
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


