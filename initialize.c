

SDL_Surface * loadimg(char * str){
  SDL_Surface * surf = IMG_Load(str);
  if(!surf){ puts("img load error!"); exit(1); }
  SDL_Surface * optsurf = SDL_DisplayFormatAlpha(surf);
  free(surf);
  return(optsurf);
}



void load_sprites(){
  terrsrf[0] = loadimg("img/grass.xpm");
  terrsrf[1] = loadimg("img/tree.xpm");
  terrsrf[2] = loadimg("img/water.xpm");
  terrsrf[3] = loadimg("img/hills.xpm");
  terrsrf[4] = loadimg("img/mounteen.xpm");

  arrow = loadimg("img/w/arrow.xpm");

  sel = loadimg("img/sel.xpm");
  hl1 = loadimg("img/hl1.png");
  hl3 = loadimg("img/hl3.png");
  hl5 = loadimg("img/hl5.png");
  hl6 = loadimg("img/hl6.png");

  sldr_wa = loadimg("img/w/b.png");
  sldr_wd = loadimg("img/w/d.png");
  sldr_wb = loadimg("img/w/b.png");
  sldr_wh = loadimg("img/w/h.png");
}



void initmapcost(){
  char * ini = 
    "   . . * . . . *  "
    "  * . . t . M .   "
    "   . . . . . . .  "
    "  . . h . M . .   "
    "   . . . . . . .  "
    "  . . . t M . .   "
    "   . . . t . * .  "
    "  t t . . . . .   "
    "   . . . . . . *  "
    "  . * . * * . *   "
    "   . * . . . * .  "
    "  . . * * * . .   "
    "   . . . . . . .  "
    "  . . . . . . .   "
    "\0";
    
  int mi = 0; // индекс в массиве 'map'
  for(int i=0; ini[i]!='\0'; i++){
    if(ini[i]!=' '){
      tile * t = worlds[0].map + mi;
      if(ini[i]=='.') t->type = 0;
      if(ini[i]=='t') t->type = 1;
      if(ini[i]=='*') t->type = 2;
      if(ini[i]=='h') t->type = 3;
      if(ini[i]=='M') t->type = 4;
      mi++;
    }
  }
}


void add_feature(unit * u, int type, feature_data * data){
  feature * f = malloc(sizeof(feature));
  f->type = type;
  f->data = *data;
  l_push(u->features, f);
}



void add_unit(mcrd crd, int player, unit_type * type) {
  unit * u  = malloc(sizeof(unit));
  u->player = player;
  u->mvp    = type->mvp;
  u->can_attack = true;
  u->health = type->health;
  u->mcrd   = crd;
  u->scrd   = map2scr(crd);
  u->type   = type;
  // на всякий инициализировать список
  *(u->features) = (l_list){0, 0, 0};
  mp(crd)->unit = u;

  if(worlds[0].units->count>0)
    u->id = ((unit*)l_last(worlds[0].units))->id;
  else
    u->id = 0;
  
  // инициализировать нужные особенности юнитов
  if(type == &utypes[2]){ // archer
    feature_range fd = {5,2,999};
    add_feature(u, FEATURE_RNG, (feature_data*)&fd );
  }
  if(type == &utypes[1]) { // hunter
    int btrue = 1;
    add_feature(u, FEATURE_IGNR,  (feature_data*)&btrue);
    add_feature(u, FEATURE_INVIS, (feature_data*)&btrue);
  }

  // add new unit to list "units"
  l_push(worlds[0].units, u);
}



void init_worlds() {
  for(int i=0; i<8; i++){
    worlds[0].map   = calloc(MAP_W*MAP_H, sizeof(tile));
    worlds[0].st    = calloc(1, sizeof(l_list));
    worlds[0].path  = calloc(1, sizeof(l_list));
    worlds[0].units = calloc(1, sizeof(l_list));
    worlds[0].selhex = (mcrd){-1,-1};
    worlds[0].mode = MODE_SELECT;
    worlds[0].selunit = NULL;
  }
}



void add_units(){
  add_unit( (mcrd){1,2}, 0, &utypes[0] );
  add_unit( (mcrd){1,3}, 0, &utypes[0] );
  add_unit( (mcrd){1,4}, 0, &utypes[0] );
  add_unit( (mcrd){1,5}, 0, &utypes[1] );
  add_unit( (mcrd){2,5}, 0, &utypes[1] );
  add_unit( (mcrd){2,6}, 0, &utypes[1] );
            
  add_unit( (mcrd){3,5}, 1, &utypes[0] );
  add_unit( (mcrd){3,1}, 1, &utypes[2] );
  add_unit( (mcrd){3,2}, 1, &utypes[2] );
  add_unit( (mcrd){3,3}, 1, &utypes[2] );

  add_unit( (mcrd){6,6}, 2, &utypes[1] );
}



void init(){
  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  atexit(SDL_Quit);

  Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
  //screen = SDL_SetVideoMode(320, 240, 32, flags);
  screen = SDL_SetVideoMode(640, 480, 32, flags);

  char * f = "LiberationMono-Regular.ttf";
  font = TTF_OpenFont(f, 12);

  player = 0;

  init_worlds();
  load_sprites();
  initmapcost();
  add_units();
}

