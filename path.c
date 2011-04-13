

void push(Mcrd tile, Mcrd parent, int newcost) {
  mp(tile)->cost   = newcost;
  mp(tile)->parent = parent;
  
  Mcrd * m = malloc(sizeof(Mcrd));
  m->x = tile.x;
  m->y = tile.y;
  l_push(cw->st, m);
}



Mcrd pop(){
  Mcrd m = *(Mcrd*)l_pop(cw->st);
  return( m );
}




// returns corrected newcost
// сделать так, что б ход заканчивался на этой клетке
// т.е. дополнить стоимость до ближайшего кратного 
// в этом случае он и так закончится тут
int zoc(Mcrd a, Unit * u, int cost){
  if(find_feature(u, FEATURE_IGNR))
    return(cost);

  int mvp = u->type->mvp;
  for(int i=0; i<6; i++){
    Mcrd n = neib(a, i);
    Unit * u2 = find_unit_at(n);
    if(inboard(n) && cost%mvp!=0
    && u2 && u2->player!=u->player
    && mp(n)->fog>0 && !is_invis(u2) )
      return(cost + mvp - (cost % mvp));
  }
  return(cost);
}



// process neiborhood
void process_nbh (Unit * u, Mcrd t, Mcrd nb){
  if( ! inboard(nb) )
    return;

  // что бы не проходить через видимых врагов
  Unit * u2 = find_unit_at(nb);
  if(u2 // && u2->player!=u->player
  && mp(nb)->fog>0
  && !is_invis(u2) )
    return;
  
  int n       = u->type->ter_mvp[mp(nb)->type];
  int newcost = zoc(nb, u, mp(t)->cost + n);

  if(mp(nb)->cost>newcost && newcost<=u->type->mvp)
    push(nb, t, newcost);
}



void fill_map(Unit * u) {
  Mcrd m;
  FOR_EACH_MCRD(m){
    mp(m)->cost   = 30000;
    mp(m)->parent = mk_mcrd(0,0);
  }  
  push(u->mcrd, u->mcrd, 0); // push start point
  while(cw->st->count>0){
    Mcrd t = pop();
    for(int i=0; i<6; i++)
      process_nbh(u, t, neib(t, i));
  }
}



void clear_path(){
  while(cw->path->count)
    free(l_pop(cw->path));
}


void addwaypoint(Mcrd wp){
  Mcrd * m = malloc(sizeof(Mcrd));
  m->x = wp.x;
  m->y = wp.y;
  l_push(cw->path, m);
}



void get_path(Mcrd a){
  clear_path();
  while(mp(a)->cost!=0){
    addwaypoint(a);
    a = mp(a)->parent;
  }
  // добавляем отправную точку(где стоит юнит)
  addwaypoint(a);
}


