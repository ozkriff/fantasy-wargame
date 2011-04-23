
// вспомогательные/служебные функции

Mcrd mk_mcrd(int x, int y){
  Mcrd m = {x, y};
  return(m);
}

Scrd mk_scrd(int x, int y){
  Scrd s = {x, y};
  return(s);
}

void PrintFError (char * format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsprintf(buffer,format, args);
  perror(buffer);
  va_end(args);
}



#define FOR_EACH_MCRD(mc) \
  for(mc.y=0; mc.y<MAP_H; mc.y++) \
    for(mc.x=0; mc.x<MAP_W; mc.x++)    
    
Feature * find_feature(Unit * u, int type){
  Node * node;
  FOR_EACH_NODE(u->features, node){
    Feature * f = node->d;
    if(f->type == type)
      return(f);
  }
  return(NULL);
}





/*
void print_event_queue(List * l){
  for(int i=0; i<players_count; i++){
    Node * node;
    FOR_EACH_NODE(l, node){
      event * e = node->d;
      for(int j=0; j<e.data[0]; j++)
        printf("%i ", e.data[i]);
    }
    puts("");
  }
}
*/



void add_event (Event * data){
  Node * nd;
  FOR_EACH_NODE(worlds, nd){
    World * world = nd->d;
    Event * e = calloc(1, sizeof(Event));
    *e = *data; // copy
    l_addtail(world->eq, e);
  }
}




// равны ли mcrd?
bool mcrdeq(Mcrd a, Mcrd b){ return(a.x==b.x&&a.y==b.y); }



// доступ к ячейке по mcrd
Tile * mp(Mcrd c){ return(cw->map + MAP_W*c.y + c.x); }



// дистанция между клетками
int mdist(Mcrd a, Mcrd b) {
  a.x += a.y/2;
  b.x += b.y/2;
  
  int dx = b.x-a.x;
  int dy = b.y-a.y;
  
  return( (abs(dx) + abs(dy) + abs(dx-dy)) / 2 );
}



// int i - neib index
// получить соседа клетки
Mcrd neib(Mcrd a, int i) {
  int d[2][6][2] = {
    { {1,-1}, {1,0}, {1,1}, { 0,1}, {-1,0}, { 0,-1}, },
    { {0,-1}, {1,0}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, } };
  int dx = d[a.y%2][i][0];
  int dy = d[a.y%2][i][1];
  return( (Mcrd){a.x+dx, a.y+dy} );
}



// TODO rename
// возвращает противоположный индекс соседа
int neib2(int i){
  //i+=3; if(i>=6) i-=6;
  int d[] = {3, 4,2, 5,1, 0};
  return(i+d[i]);
}



int mcrd2index(Mcrd a, Mcrd b){
  for(int i=0; i<6; i++){
    if(mcrdeq(neib(a,i), b))
      return(i);
  }
  exit(1); // ERROR
}



// в пределах карты?
bool inboard(Mcrd t){
  return( t.x>=0 && t.x<MAP_W && t.y>=0 && t.y<MAP_H );
}



// расстояние между двумя экранными точками
int sdist(Scrd a, Scrd b) {
  int dx = abs(b.x - a.x);
  int dy = abs(b.y - a.y);

  return( sqrt(pow(dx, 2)+pow(dy, 2)) );
}



Scrd map2scr(Mcrd map) {
  Scrd scr;

  // space bwetween tiles
  int space = 0;
  
  scr.y = map_offset.y  + map.y*(29+space);
  scr.x = map_offset.x  + map.x*(72+space);
  //if(map.y%2) scr.x += 36;
  if(map.y%2) scr.x -= 36;
  
  return(scr);
}



// find tile with nearest coords
// деление надва нужно потому,
// что клетки `сплющены` по вертикали
Mcrd scr2map(Scrd m) {
  Mcrd min;            
  int min_dist = 9000;

  m.x/=2;

  Mcrd mc;
  FOR_EACH_MCRD(mc){
    Scrd wp = map2scr(mc);
    wp.x += 36; wp.y += 54; wp.x/=2;
    if(sdist(m, wp) < min_dist){
      min_dist = sdist(m, wp);
      min = mc;
    }
  }
  return(min);
}



// назвать понятнее
// возвращает промежуточные координаты между клетками
Scrd between(Scrd a, Scrd b, int i) {
  float dx = (float)(b.x-a.x)/STEPS;
  float dy = (float)(b.y-a.y)/STEPS;
  return( mk_scrd(a.x+dx*i, a.y+dy*i) );
}



Scrd mbetween(Mcrd a, Mcrd b, int i){
  return(  between(map2scr(a), map2scr(b), i)  );
}



Unit * id2unit(int id){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(u->id == id)
      return(u);
  }
  return(NULL);
}



Unit * find_unit_at(Mcrd crd){
  Node * node;
  FOR_EACH_NODE(cw->units, node){
    Unit * u = node->d;
    if(mcrdeq(u->mcrd, crd))
      return(u);
  }
  return(NULL);
}



bool is_invis (Unit * u){
  if(!find_feature(u, FEATURE_INVIS))
    return(false);

  for(int i=0; i<6; i++){
    Mcrd nb = neib(u->mcrd, i);
    Unit * u2 = find_unit_at(nb);
    if(u2 && u2->player != u->player)
      return(false);
  }
  return(true);
}

