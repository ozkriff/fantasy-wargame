

void push(mcrd tile, mcrd parent, int newcost) {
  mp(tile)->cost   = newcost;
  mp(tile)->parent = parent;
  
  mnode * new = malloc(sizeof(mnode));
  new->crd = tile;
  l_push(cw->st, new);
}



mcrd pop(){
  mnode * tmp = (mnode*)l_pop(cw->st);
  mcrd crd = tmp->crd;
  free(tmp);
  return(crd);
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



// returns corrected newcost
// сделать так, что б ход заканчивался на этой клетке
// т.е. дополнить стоимость до ближайшего кратного 
// в этом случае он и так закончится тут
int zoc(mcrd a, unit * u, int cost){
  if(find_feature(u, FEATURE_IGNR))
    return(cost);

  int mvp = u->type->mvp;
  for(int i=0; i<6; i++){
    mcrd n = neib(a, i);
    unit * u2 = mp(n)->unit;
    if(inboard(n) && cost%mvp!=0
    && u2 && u2->player!=u->player
    && mp(n)->fog>0 && !is_invis(u2) )
      return(cost + mvp - (cost % mvp));
  }
  return(cost);
}



// process neiborhood
void process_nbh (unit * u, mcrd t, mcrd nb){
  if( ! inboard(nb) )
    return;

  // что бы не проходить через видимых врагов
  unit * u2 = mp(nb)->unit;
  if(u2 && u2->player!=u->player
  && mp(nb)->fog>0
  && !is_invis(u2) )
    return;
  
  int n       = u->type->ter_mvp[mp(nb)->type];
  int newcost = zoc(nb, u, mp(t)->cost + n);

  if(mp(nb)->cost>newcost && newcost<=u->type->mvp)
    push(nb, t, newcost);
}



void fill_map(unit * u) {
  FOR_EACH_TILE{
    mp(mc)->cost   = 30000;
    mp(mc)->parent = nmcrd;
  }  
  push(u->mcrd, u->mcrd, 0); // push start point
  while(cw->st->count>0){
    mcrd t = pop();
    for(int i=0; i<6; i++)
      process_nbh(u, t, neib(t, i));
  }
}



void clear_path(){
  while(cw->path->count)
    free(l_pop(cw->path));
}


void addwaypoint(mcrd wp){
  mnode * new = malloc(sizeof(mnode));
  new->crd = wp;
  l_push(cw->path, new);
}



void get_path(mcrd a){
  clear_path();
  while(mp(a)->cost!=0){
    addwaypoint(a);
    a = mp(a)->parent;
  }
  // добавляем отправную точку(где стоит юнит)
  addwaypoint(a);
}


