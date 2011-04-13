

// просто рисует пиксель
void pxl32(int x, int y, Uint32 pixel) {
  if( x<0 || y<0 || x >= screen->w || y >= screen->h )
    return;
  Uint32 * pixels = (Uint32 *)screen->pixels;
  pixels[ (y * screen->w) + x ] = pixel;
}


// рисует "большой"(size)  пиксель
void bigpxl(Uint32 colr, int size, int x, int y){
  SDL_Rect rect = {x, y, size, size};
  SDL_FillRect(screen, &rect, colr);
}



#define bzpxl(x,y,clr) \
  if(!steep) pxl32(x,y,clr); else pxl32(y,x,clr);
#define swap(a,b) { int tmp;  tmp=a; a=b; b=tmp; }

void bzline (scrd a, scrd b, Uint32 clr){
  bool steep = abs(b.y-a.y) > abs(b.x-a.x);
  if(steep)   { swap(a.x,a.y); swap(b.x,b.y); }
  if(a.x>b.x) { swap(a.x,b.x); swap(a.y,b.y); }
  int deltax = b.x-a.x;
  int deltay = abs(b.y-a.y);
  int error = deltax >> 1;
  int y = a.y, ystep = (a.y<b.y) ? 1 : -1;
  for (int x = a.x; x <= b.x; x++) {
    bzpxl(x,y,clr);
    error -= deltay;
    if(error<0){ y+=ystep; error+=deltax; }
  }
}



void blit(SDL_Surface *src, int x, int y) {
  SDL_Rect rect = {x, y, 0, 0};
  SDL_BlitSurface(src, NULL, screen, &rect);
}



void mblit(SDL_Surface *src, mcrd crd) {
  blit(src, crd.x, crd.y);
}



void draw_map() {
  mcrd mc;
  FOR_EACH_MCRD(mc){
    mblit(terrsrf[ mp(mc)->type ], map2scr(mc));
    if(mp(mc)->fog<=0)  mblit(hl3, map2scr(mc));
  }
}
  


// TODO rename: mconnect? m2mline?
// draw line between 2 tiles
void mline(mcrd a, mcrd b){
  scrd sa = map2scr(a), sb = map2scr(b);
  sa.x+=36; sa.y+=54; sb.x+=36; sb.y+=54;
  //unit * u = cw->selunit ? cw->selunit : cw->move_unit;
  //if(!u)
    //return;
  //if(mp(a)->cost <= u->mvp)
    //bzline(sa, sb, BLUE);
  //else
    bzline(sa, sb, RED);
}



// draw current path
void draw_path() {
  if(cw->path->count>0){
    l_node * node;
    for(node=cw->path->h; node->n; node=node->n){
      mcrd * current = node->d;
      mcrd * next    = node->n->d;
      mline(*current, *next);
    }
  }
}



// draw path to some point
void draw_path_2_mcrd(mcrd a){
  if(mp(a)->cost==30000) return;
  mcrd tmp = a;
  while( ! mcrdeq(tmp,cw->selunit->mcrd) ){
    mline(tmp, mp(tmp)->parent);
    tmp = mp(tmp)->parent;
  }
}



void draw_bg(Uint32 clr){ SDL_FillRect(screen,NULL,clr); }



void text(char * str, scrd crd, bool iscentred){
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



SDL_Surface * type2srf(unit_type * t){
       if(t==&utypes[0]) return(sldr_wd);
  else if(t==&utypes[1]) return(sldr_wh);
  else if(t==&utypes[2]) return(sldr_wa);
  else exit(1);
}



void draw_unit(unit *u){
  scrd s = map2scr(u->mcrd);
  if(u->player==0) mblit(hl5, s);
  if(u->player==1) mblit(hl6, s);
  mblit(type2srf(u->type), s);

  if(1){
    char str[100];
    sprintf(str, "%i", u->health);
    text(str, (scrd){s.x+10, s.y+60}, false);
  }
}



void draw_units(){
  l_node * node;
  FOR_EACH_NODE(cw->units, node){
    unit * u = node->d;

    if(cw->mode==MODE_ATTACK
    && cw->e[1]==EVENT_MELEE
    && cw->e[2]==u->id)
      continue;

    if(cw->mode==MODE_MOVE
    && cw->e[1]==EVENT_MOVE
    && cw->e[2]==u->id)
      continue;

    if( u->player!=player
    && (mp(u->mcrd)->fog==0 || is_invis(u)) )
      continue;

    draw_unit(u);
  }
}



void draw_moving_unit(){
  unit * u = id2unit(cw->e[2]);
  mcrd b = {cw->e[3], cw->e[4]};
  scrd crd = mbetween(u->mcrd, b, cw->index);
  mblit(type2srf(u->type), crd);
}



void draw_attacking_unit(){
  mcrd a = id2unit(cw->e[2])->mcrd;
  mcrd b = neib(a, cw->e[3]);
  int i = (cw->index<STEPS/2) ? (cw->index) : (STEPS-cw->index);
  scrd crd = mbetween(a, b, i);
  mblit(type2srf(id2unit(cw->e[2])->type), crd);
}


void draw_possible_tiles(){
  mcrd mc;
  FOR_EACH_MCRD(mc){
    mcrd p = mp(mc)->parent;
    if(!(p.x==0 && p.y==0)
    && mp(mc)->cost <= cw->selunit->mvp) {
      mblit(hl1, map2scr(mc));
      mline(mc, p);
    }
  }
}



void maptext(){
  mcrd mc;
  FOR_EACH_MCRD(mc){
    if(mp(mc)->cost!=30000 && mp(mc)->cost!=0){
      scrd s = map2scr(mc);
      s.x+=36; s.y+=54;
      char str[100];
      sprintf(str, "%i", mp(mc)->cost);
      text(str, s, true);
    }
  }
}



void draw_shoot_attack(){
  unit * u1 = id2unit(cw->e[2]);
  unit * u2 = id2unit(cw->e[3]);
  scrd a = u1->scrd;
  scrd b = u2->scrd;
  int steps = sdist(a,b)/6;
  float dx  = (float)(b.x-a.x)/steps;
  float dy  = (float)(b.y-a.y)/steps;

  a.x += dx * cw->index;
  a.y += dy * cw->index;

  // вертикальная поправка
  int dh = 36 * sinf((float)cw->index/steps*3.14);
  blit(arrow, a.x, a.y-dh);

  // рисует "хвост" стрелы. через задницу!
  for(int i=1; i<cw->index; i++){
    // вертикальная поправка
    int d0 = 36 * sinf((float)(i  )/steps*3.14);
    int d1 = 36 * sinf((float)(i-1)/steps*3.14);

    scrd n0 = u1->scrd;
    scrd n1 = u1->scrd;
    n0.x += 36+ dx*(i  );  n0.y += 36+ dy*(i  )-d0;
    n1.x += 36+ dx*(i-1);  n1.y += 36+ dy*(i-1)-d1;

    bzline(n0, n1, RED);
  }
}



void draw(){
  draw_bg(BLACK);

  // TODO нужно вызывать не отсюда. смотри NOTES
  updatefog(player); 

  draw_map();
  if(cw->mode==MODE_SELECT && cw->selunit)
    draw_possible_tiles();
  mblit(sel, map2scr(cw->selhex));
  if(cw->selunit) mblit(sel, map2scr(cw->selunit->mcrd));
  draw_units();
  if(cw->mode==MODE_MOVE){ draw_moving_unit(); /*draw_path();*/ }
  if(cw->mode==MODE_ATTACK){
    if(cw->e[1]==EVENT_MELEE) draw_attacking_unit();
    if(cw->e[1]==EVENT_RANGE) draw_shoot_attack();
  }
  //maptext();
  text( (player==0)?"[pl:0]":"[pl:1]", (scrd){0,0}, false);
  
  SDL_Flip(screen);
}


