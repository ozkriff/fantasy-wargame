
// вспомогательные/служебные функции

void PrintFError (char * format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsprintf(buffer,format, args);
  perror(buffer);
  va_end(args);
}



#define FOR_EACH_UNIT \
  for(unit * u=(unit*)l_first(cw->units); \
  u; u=(unit*)l_next(u))

#define FOR_EACH_TILE \
  for(mcrd mc={0,0}; mc.y<MAP_H; mc.y++) \
    for(mc.x=0; mc.x<MAP_W; mc.x++)    
    
feature * find_feature(unit * u, int type){
  feature * f = (feature*)l_first(u->features);
  while(f){
    if(f->type == type)
      return(f);
    f = (feature*)l_next(f);
  }
  return(NULL);
}





void print_event_queue(l_list * l){
  event * e;
  for(int i=0; i<players_count; i++){
    for(e = (event*)l_first(l); e; e = (event*)l_next(e)){
      for(int j=0; j<e->data[0]; j++)
        printf("%i ", e->data[i]);
    }
    puts("");
  }
}


void add_event(int * data){
  for(int i=0; i<players_count; i++){
    event * e = calloc(1, sizeof(event));
    e->data   = calloc(data[0], sizeof(int));
    for(int j=0; j<data[0]; j++)
      e->data[j] = data[j];
    l_addt(worlds[i].event_queue, e);
  }
}



// равны ли mcrd?
bool mcrdeq(mcrd a, mcrd b){ return(a.x==b.x&&a.y==b.y); }



// доступ к ячейке по mcrd
tile * mp(mcrd c){ return(cw->map + MAP_W*c.y + c.x); }



// дистанция между клетками
int mdist(mcrd a, mcrd b) {
  a.x += a.y/2;
  b.x += b.y/2;
  
  int dx = b.x-a.x;
  int dy = b.y-a.y;
  
  return( (abs(dx) + abs(dy) + abs(dx-dy)) / 2 );
}



// int i - neib index
// получить соседа клетки
mcrd neib(mcrd a, int i) {
  int d[2][6][2] = {
    { {1,-1}, {1,0}, {1,1}, { 0,1}, {-1,0}, { 0,-1}, },
    { {0,-1}, {1,0}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, } };
  int dx = d[a.y%2][i][0];
  int dy = d[a.y%2][i][1];
  return( (mcrd){a.x+dx, a.y+dy} );
}



// TODO rename
// возвращает противоположный индекс соседа
int neib2(int i){
  //i+=3; if(i>=6) i-=6;
  int d[] = {3, 4,2, 5,1, 0};
  return(i+d[i]);
}



int mcrd2index(mcrd a, mcrd b){
  for(int i=0; i<6; i++){
    if(mcrdeq(neib(a,i), b))
      return(i);
  }
  exit(1); // ERROR
}



// в пределах карты?
bool inboard(mcrd t){
  return( t.x>=0 && t.x<MAP_W && t.y>=0 && t.y<MAP_H );
}



// расстояние между двумя экранными точками
int sdist(scrd a, scrd b) {
  int dx = abs(b.x - a.x);
  int dy = abs(b.y - a.y);

  return( sqrt(pow(dx, 2)+pow(dy, 2)) );
}



scrd map2scr(mcrd map) {
  scrd scr;

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
mcrd scr2map(scrd m) {
  mcrd min;            
  int min_dist = 9000;

  m.x/=2;

  FOR_EACH_TILE{
    scrd wp = map2scr(mc);
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
scrd between(scrd a, scrd b, int i) {
  float dx = (float)(b.x-a.x)/STEPS;
  float dy = (float)(b.y-a.y)/STEPS;
  return( (scrd){a.x+dx*i, a.y+dy*i} );
}



scrd mbetween(mcrd a, mcrd b, int i){
  return(  between(map2scr(a), map2scr(b), i)  );
}



unit * id2unit(int id){
  FOR_EACH_UNIT{
    if(u->id == id)
      return(u);
  }
  return(NULL);
}



bool is_invis (unit * u){
  if(!find_feature(u, FEATURE_INVIS))
    return(false);

  for(int i=0; i<6; i++){
    mcrd nb = neib(u->mcrd, i);
    unit * u2 = mp(nb)->unit;
    if(u2 && u2->player != u->player)
      return(false);
  }
  return(true);
}


