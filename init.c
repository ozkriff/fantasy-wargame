
int str2int (char * str) {
  int n;
  if(sscanf(str, "%i", &n) != 1){
    puts("str2int error.");
    exit(1);
  }
  return(n);
}



void add_feature(Unit * u, int type, Feature_data * data){
  Feature * f = malloc(sizeof(Feature));
  f->type = type;
  f->data = *data;
  l_push(u->features, f);
}



void add_default_features_to_unit (Unit * u){
  if(u->type == &utypes[2]){ /* archer */
    Feature_range fd = {5,4,999};
    add_feature(u, FEATURE_RNG, (Feature_data*)&fd );
  }
  if(u->type == &utypes[1]) { /* hunter */
    int btrue = 1;
    add_feature(u, FEATURE_IGNR,     (Feature_data*)&btrue);
    add_feature(u, FEATURE_INVIS,    (Feature_data*)&btrue);
    add_feature(u, FEATURE_NORETURN, (Feature_data*)&btrue);
  }
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
  u->features = calloc(1, sizeof(List));;
  
  u->id = units->count>0 ? ((Unit*)units->h->d)->id+1 : 0;
  
  add_default_features_to_unit(u);

  l_push(units, u);
}



void read_config(char * filename){
  FILE * cfg = fopen(filename, "r");
  char s[100]; /* buffer */

  while( fgets(s, 90, cfg) ){
    if(s[0]=='#' || s[0]=='\n')
      continue;
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
      int mi = 0; /* индекс в массиве 'map' */
      while( fgets(s, 90, cfg) && s[0]!='\n'){
        int i;
        for(i=0; s[i]; i++){
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



void create_local_world (int id, bool is_ai) {
  World * w = calloc(1, sizeof(World));
  w->units  = calloc(1, sizeof(List));
  w->eq     = calloc(1, sizeof(List));
  w->id     = id;
  w->is_ai  = is_ai;
  l_push(worlds, w);
}



void create_local_human (int id) {
  create_local_world(id, false);
}



void create_local_ai (int id) {
  create_local_world(id, true);
}



void local_arguments (int ac, char ** av)
{
  /*is_local = true;*/

  /*av[0]-program name, av[1]-"-local", av[2]-scenario*/
  int i;
  for(i=3; i<ac; i++){
    if(!strcmp(av[i], "-ai")){
      int id = str2int(av[i+1]);
      create_local_ai(id);
    }
    if(!strcmp(av[i], "-human")){
      int id = str2int(av[i+1]);
      create_local_human(id);
    }
  }
  read_config(av[2]);
}



void send_int_as_uint8 (int n){
  uint8_t x = n;
  SDLNet_TCP_Send(socket, &x, 1);
}



void net_arguments (int ac, char ** av){
  is_local = false;

  int port = str2int(av[3]);
  init_network(av[2], port);

  /* 0-имя программы, 1-"-net", 2-"server", 3-port */
  int i;
  for(i=4; i<ac; i++){
    if(!strcmp(av[i], "-ai")){
      int id = str2int(av[i+1]);
      create_local_ai(id);
      send_int_as_uint8(id);
    }
    if(!strcmp(av[i], "-human")){
      int id = str2int(av[i+1]);
      create_local_human(id);
      send_int_as_uint8(id);
    }
  }

  /*сказать серверу, что больше игроков у клинта нет*/
  /*0xff-специальная метка. игрока с таким id не бывает.*/
  send_int_as_uint8(0xff);
  
  /*тут мы получаем имя сценария от сервера!*/
  while(1){
    if(SDLNet_CheckSockets(sockets, 100)==0)
      continue;
    
    if(SDLNet_SocketReady(socket)){
      /*incoming data size in bytes*/
      uint8_t size;
      char scenarioname[255];
      SDLNet_TCP_Recv(socket, &size, 1);
      SDLNet_TCP_Recv(socket, scenarioname, size);
      printf("scenario: '%s'\n", scenarioname); /* <-- */
      read_config(scenarioname);
      return;
    }
  }
}




void init(int ac, char ** av){
  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);
  TTF_Init();
  atexit(SDL_Quit);

  Uint32 flags = SDL_SWSURFACE | SDL_RESIZABLE;
  screen = SDL_SetVideoMode(640, 480, 32, flags);

  /*char * f = "LiberationMono-Regular.ttf"; */
  font = TTF_OpenFont("font.ttf", 12);

  worlds = calloc(1, sizeof(List));

  st    = calloc(1, sizeof(List));
  path  = calloc(1, sizeof(List));
  selhex = mk_mcrd(-1,-1);
  mode = MODE_SELECT;
  selunit = NULL;

  if(!strcmp(av[1], "-local"))
    local_arguments(ac, av);
  if(!strcmp(av[1], "-net")){
    net_arguments(ac, av);
  }

  cw = worlds->h->d;

  load_sprites();

  updatefog(cw->id);
}

