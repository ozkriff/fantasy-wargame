

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



void add_feature(Unit * u, int type, Feature_data * data){
  Feature * f = malloc(sizeof(Feature));
  f->type = type;
  f->data = *data;
  l_push(u->features, f);
}



void add_unit(Mcrd crd, int plr, Unit_type * type, World * wrld) {
  List * units = wrld->units;

  Unit * u  = malloc(sizeof(Unit));
  u->player = plr;
  u->mvp    = type->mvp;
  u->can_attack = true;
  u->health = type->health;
  u->mcrd   = crd;
  u->scrd   = map2scr(crd);
  u->type   = type;
  // на всякий инициализировать список
  u->features = calloc(1, sizeof(List));;
  
  //mp(crd)->unit = u;

  u->id = units->count>0 ? ((Unit*)units->h->d)->id+1 : 0;
  
  // инициализировать нужные особенности юнитов
  if(type == &utypes[2]){ // archer
    Feature_range fd = {5,4,999};
    add_feature(u, FEATURE_RNG, (Feature_data*)&fd );
  }
  if(type == &utypes[1]) { // hunter
    int btrue = 1;
    add_feature(u, FEATURE_IGNR,     (Feature_data*)&btrue);
    add_feature(u, FEATURE_INVIS,    (Feature_data*)&btrue);
    add_feature(u, FEATURE_NORETURN, (Feature_data*)&btrue);
  }

  l_push(units, u);
}



void read_config(){
  FILE * cfg = fopen("scenario2", "r");
  char s[100]; // buffer

  while( fgets(s, 90, cfg) ){
    if(s[0]=='#' || s[0]=='\n')
      continue;
    if(!strncmp("[NUM-OF-PLAYERS]", s, 15)){
      sscanf(s, "[NUM-OF-PLAYERS] %i",
          &players_count);

      int i;
      for(i=0; i<players_count; i++){
        World * w = calloc(1, sizeof(World));
        w->units = calloc(1, sizeof(List));
        w->eq    = calloc(1, sizeof(List));
        l_addtail(worlds, w);
      }
    }
    if(!strncmp("[MAP-SIZE]", s, 10)){
      sscanf(s, "[MAP-SIZE] %i %i",
          &MAP_W, &MAP_H);

      Node * nd;
      FOR_EACH_NODE(worlds, nd){
        World * w = nd->d;
        w->map = calloc(MAP_W*MAP_H, sizeof(Tile));
      }
    }
    if(!strncmp("[UNIT]", s, 6)){
      int plr, x, y, type;
      sscanf(s, "[UNIT] %i %i %i %i",
          &plr, &x, &y, &type);

      Node * nd;
      FOR_EACH_NODE(worlds, nd){
        add_unit(mk_mcrd(x,y), plr, &utypes[type], nd->d);
      }
    }
    if(!strncmp("[MAP]", s, 5)){
      int mi = 0; // индекс в массиве 'map'
      while( fgets(s, 90, cfg) && s[0]!='\n'){
        for(int i=0; s[i]; i++){
          char c = s[i];
          if(c != ' ' && c != '\n'){
            Node * nd;
            FOR_EACH_NODE(worlds, nd){
              World * w = nd->d;
              Tile * t = w->map + mi;
              if(c=='.') t->type = 0;
              if(c=='t') t->type = 1;
              if(c=='*') t->type = 2;
              if(c=='h') t->type = 3;
              if(c=='M') t->type = 4;
            }
            mi++;
          }
        }
      }
    }
  }
  
  fclose(cfg);
}



void init(){
  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  atexit(SDL_Quit);

  Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
  screen = SDL_SetVideoMode(640, 480, 32, flags);

  //char * f = "LiberationMono-Regular.ttf";
  font = TTF_OpenFont("font.ttf", 12);

  worlds = calloc(1, sizeof(List));

  st    = calloc(1, sizeof(List));
  path  = calloc(1, sizeof(List));
  selhex = mk_mcrd(-1,-1);
  mode = MODE_SELECT;
  selunit = NULL;

  read_config();

  player = 0;
  cw = worlds->h->d;

  load_sprites();

  updatefog(player);
}

