

// просто рисует пиксель
void pxl32(int x, int y, Uint32 pixel) {
  if( x<0 || y<0 || x >= screen->w || y >= screen->h )
    return;
  Uint32 * pixels = (Uint32 *)screen->pixels;
  pixels[ (y * screen->w) + x ] = pixel;
}


// рисует "большой"(size)  пиксель
void pxl(Uint32 colr, int size, int x, int y){
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
  FOR_EACH_TILE{
    mblit(terrsrf[ mp(mc)->type ], map2scr(mc));
    if(mp(mc)->fog<=0)  mblit(hl3, map2scr(mc));
  }
}
  


// TODO rename: mconnect? m2mline?
// draw line between 2 tiles
void mline(mcrd a, mcrd b){
  scrd sa = map2scr(a), sb = map2scr(b);
  sa.x+=36; sa.y+=54; sb.x+=36; sb.y+=54;
  if(mp(a)->cost <= worlds[0].selunit->mvp)
    bzline(sa, sb, BLUE);
  else
    bzline(sa, sb, RED);
}



// draw current path
void draw_path() {
  if(worlds[0].path->count>0){
    mnode * tile = (mnode*)l_first(worlds[0].path);
    while(l_next(tile)){
      if(l_next(tile)){
        mcrd a = tile->crd;
        mcrd b = ((mnode*)l_next(tile))->crd;
        mline(a,b);
      }
      tile = (mnode*)l_next(tile);
    }
  }
}



// draw path to some point
void draw_path_2_mcrd(mcrd a){
  if(mp(a)->cost==30000) return;
  mcrd tmp = a;
  while( ! mcrdeq(tmp,worlds[0].selunit->mcrd) ){
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

  if(0){
    char str[100];
    sprintf(str, "[%i.%i.%i.%i]",
        u->health, u->mvp>0, u->can_attack,
        (bool)find_feature(u, FEATURE_RNG));
    text(str, (scrd){s.x+10, s.y+60}, false);
  }
}



void draw_units(){
  FOR_EACH_UNIT{
    if(worlds[0].move_unit!=u && worlds[0].attack_u1!=u
    && mp(u->mcrd)->fog>0 
    && !(u->player!=player && is_invis(u)) )
      draw_unit(u);
  }
}



void draw_moving_unit(){
  mcrd a = worlds[0].move_tile->crd;
  mcrd b = ((mnode*)l_next(worlds[0].move_tile))->crd;
  scrd crd = mbetween(a, b, worlds[0].move_index);
  mblit(type2srf(worlds[0].move_unit->type), crd);
}



void draw_attacking_unit(){
  mcrd a = worlds[0].attack_u1->mcrd;
  mcrd b = worlds[0].attack_crd; //attack_u2->crd;
  scrd crd = mbetween(a, b, worlds[0].attack_index);
  mblit(type2srf(worlds[0].attack_u1->type), crd);
}



void draw_possible_tiles(){
  FOR_EACH_TILE{
    mcrd p = mp(mc)->parent;
    if(!(p.x==0 && p.y==0)
    && mp(mc)->cost <= worlds[0].selunit->mvp) {
      mblit(hl1, map2scr(mc));
      mline(mc, p);
    }
  }
}



void maptext(){
  FOR_EACH_TILE{
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
  scrd a = map2scr(worlds[0].attack_u1->mcrd);
  scrd b = map2scr(worlds[0].attack_u2->mcrd);
  int steps = sdist(a,b)/6;
  float dx  = (float)(b.x-a.x)/steps;
  float dy  = (float)(b.y-a.y)/steps;

  a.x += dx * worlds[0].attack_shoot_index;
  a.y += dy * worlds[0].attack_shoot_index;

  // пройденное снарядом расстояние. от 0.0 до 1.0
  float xxx = (float)worlds[0].attack_shoot_index/steps;

  // вертикальная поправка
  int dh = 36 * sinf(xxx*3.14);

  blit(arrow, a.x, a.y-dh);
}




void draw(){
  draw_bg(BLACK);

  // TODO нужно вызывать не отсюда. смотри NOTES
  updatefog(); 

  draw_map();
  if(worlds[0].mode==MODE_SELECT && worlds[0].selunit)
    draw_possible_tiles();
  mblit(sel, map2scr(worlds[0].selhex));
  if(worlds[0].selunit) mblit(sel, map2scr(worlds[0].selunit->mcrd));
  draw_units();
  if(worlds[0].mode==MODE_MOVE){ draw_moving_unit(); draw_path(); }
  if(worlds[0].mode==MODE_ATTACK){
    draw_attacking_unit();
    if(worlds[0].attack_is_shoot) draw_shoot_attack();
  }
  //maptext();
  text( (player==0)?"[pl:0]":"[pl:1]", (scrd){0,0}, false);
  
  SDL_Flip(screen);
}


