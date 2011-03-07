
// это что угодно, только не модуль UNITS. думать.

#define FOR_EACH_UNIT \
  for(unit * u=(unit*)l_first(worlds[0].units); u; u=(unit*)l_next(u))

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


