

/* просто рисует пиксель */
void pxl32(int x, int y, Uint32 pixel) {
  if( x<0 || y<0 || x >= screen->w || y >= screen->h )
    return;
  Uint32 * pixels = (Uint32 *)screen->pixels;
  pixels[ (y * screen->w) + x ] = pixel;
}


/* рисует "большой"(size)  пиксель */
void bigpxl(Uint32 colr, int size, int x, int y){
  SDL_Rect rect = {x, y, size, size};
  SDL_FillRect(screen, &rect, colr);
}



#define bzpxl(x,y,clr) \
  if(!steep) pxl32(x,y,clr); else pxl32(y,x,clr);
#define swap(a,b) { int tmp;  tmp=a; a=b; b=tmp; }

void bzline (Scrd a, Scrd b, Uint32 clr){
  bool steep = abs(b.y-a.y) > abs(b.x-a.x);
  if(steep)   { swap(a.x,a.y); swap(b.x,b.y); }
  if(a.x>b.x) { swap(a.x,b.x); swap(a.y,b.y); }
  int deltax = b.x-a.x;
  int deltay = abs(b.y-a.y);
  int error = deltax >> 1;
  int y = a.y, ystep = (a.y<b.y) ? 1 : -1;
  int x;
  for (x = a.x; x <= b.x; x++) {
    bzpxl(x,y,clr);
    error -= deltay;
    if(error<0){ y+=ystep; error+=deltax; }
  }
}



void blit(SDL_Surface *src, int x, int y) {
  SDL_Rect rect = {x, y, 0, 0};
  SDL_BlitSurface(src, NULL, screen, &rect);
}



void mblit(SDL_Surface *src, Mcrd crd) {
  blit(src, crd.x, crd.y);
}



void draw_map() {
  Mcrd mc;
  FOR_EACH_MCRD(mc){
    mblit(terrsrf[ mp(mc)->type ], map2scr(mc));
    if(mp(mc)->fog<=0)  mblit(hl3, map2scr(mc));
  }
}
  


/* TODO rename: mconnect? m2mline? */
/* draw line between 2 tiles */
void mline(Mcrd a, Mcrd b){
  Scrd sa = map2scr(a), sb = map2scr(b);
  sa.x+=36; sa.y+=54; sb.x+=36; sb.y+=54;
  /*unit * u = cw->selunit ? cw->selunit : cw->move_unit; */
  /*if(!u) */
    /*return; */
  /*if(mp(a)->cost <= u->mvp) */
    /*bzline(sa, sb, BLUE); */
  /*else */
    bzline(sa, sb, RED);
}



/* draw path to some point */
void draw_path_2_mcrd(Mcrd a){
  if(mp(a)->cost==30000) return;
  Mcrd tmp = a;
  while( ! mcrdeq(tmp,selunit->mcrd) ){
    mline(tmp, mp(tmp)->parent);
    tmp = mp(tmp)->parent;
  }
}



void draw_bg(Uint32 clr){ SDL_FillRect(screen,NULL,clr); }



void text(char * str, Scrd crd, bool iscentred){
  SDL_Color col = {0xFF, 0xFF, 0xFF, 0xFF};
  SDL_Surface * s = TTF_RenderText_Blended(font, str, col);
  
  SDL_Rect rect;
  if(iscentred)
    rect = (SDL_Rect){crd.x-s->w/2, crd.y-s->h/2, 0,0};
  else
    rect = (SDL_Rect){crd.x, crd.y, 0,0};
    
  SDL_BlitSurface(s, NULL, screen, &rect);
  SDL_FreeSurface(s);
}



SDL_Surface * type2srf(Unit_type * t){
       if(t==&utypes[0]) return(sldr_wd);
  else if(t==&utypes[1]) return(sldr_wh);
  else if(t==&utypes[2]) return(sldr_wa);
  else exit(1);
}



void draw_unit(Unit *u){
  Scrd s = map2scr(u->mcrd);
  if(u->player==0) mblit(hl5, s);
  if(u->player==1) mblit(hl6, s);
  mblit(type2srf(u->type), s);

  if(1){
    char str[100];
    sprintf(str, "%i", u->health);
    text(str, mk_scrd(s.x+10, s.y+60), false);
  }
}



void draw_units(){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;

    if(mode==MODE_ATTACK
    && e.t==EVENT_MELEE
    && e.melee.a==u->id)
      continue;

    if(mode==MODE_MOVE
    && e.t==EVENT_MOVE
    && e.melee.a==u->id)
      continue;

    if( u->player!=player
    && (mp(u->mcrd)->fog==0 || is_invis(u)) )
      continue;

    draw_unit(u);
  }
}



void draw_moving_unit(){
  Unit * u = id2unit(e.move.u);
  Mcrd b = e.move.dest;
  Scrd crd = mbetween(u->mcrd, b, eindex);
  mblit(type2srf(u->type), crd);
}



void draw_attacking_unit(){
  Mcrd a = id2unit(e.melee.a)->mcrd;
  Mcrd b = e.melee.md;
  int i = (eindex<STEPS/2) ? (eindex) : (STEPS-eindex);
  Scrd crd = mbetween(a, b, i);
  mblit(type2srf(id2unit(e.melee.a)->type), crd);
}


void draw_possible_tiles(){
  Mcrd mc;
  FOR_EACH_MCRD(mc){
    Mcrd p = mp(mc)->parent;
    if(!(p.x==0 && p.y==0)
    && mp(mc)->cost <= selunit->mvp) {
      mblit(hl1, map2scr(mc));
      mline(mc, p);
    }
  }
}



void maptext(){
  Mcrd mc;
  FOR_EACH_MCRD(mc){
    if(mp(mc)->cost!=30000 && mp(mc)->cost!=0){
      Scrd s = map2scr(mc);
      s.x+=36; s.y+=54;
      char str[100];
      sprintf(str, "%i", mp(mc)->cost);
      text(str, s, true);
    }
  }
}



void draw_shoot_attack(){
  Unit * u1 = id2unit(e.range.a);
  Unit * u2 = id2unit(e.range.d);
  Scrd a = u1->scrd;
  Scrd b = u2->scrd;
  int steps = sdist(a,b)/6;
  float dx  = (float)(b.x-a.x)/steps;
  float dy  = (float)(b.y-a.y)/steps;

  a.x += dx * eindex;
  a.y += dy * eindex;

  /* вертикальная поправка */
  int dh = 36 * sinf((float)eindex/steps*3.14);
  blit(arrow, a.x, a.y-dh);

  /* рисует "хвост" стрелы. через задницу! */
  int i;
  for(i=1; i<eindex; i++){
    /* вертикальная поправка */
    int d0 = 36 * sinf((float)(i  )/steps*3.14);
    int d1 = 36 * sinf((float)(i-1)/steps*3.14);

    Scrd n0 = u1->scrd;
    Scrd n1 = u1->scrd;
    n0.x += 36+ dx*(i  );  n0.y += 36+ dy*(i  )-d0;
    n1.x += 36+ dx*(i-1);  n1.y += 36+ dy*(i-1)-d1;

    bzline(n0, n1, RED);
  }
}



void draw(){
  draw_bg(BLACK);

  draw_map();
  if(mode==MODE_SELECT && selunit)
    draw_possible_tiles();
  mblit(sel, map2scr(selhex));
  if(selunit) mblit(sel, map2scr(selunit->mcrd));
  draw_units();
  if(mode==MODE_MOVE) draw_moving_unit();
  if(mode==MODE_ATTACK){
    if(e.t==EVENT_MELEE) draw_attacking_unit();
    if(e.t==EVENT_RANGE) draw_shoot_attack();
  }
  /*maptext(); */
  text( (player==0)?"[pl:0]":"[pl:1]", mk_scrd(0,0), false);
  
  SDL_Flip(screen);
}


