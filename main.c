
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "list.c"
#include "structs.c"
#include "globals.c"
#include "units.c"
#include "accessory.c"
#include "path.c"
#include "logic.c"
#include "events.c"
#include "draw.c"
#include "init.c"


void mainloop(){
  while(!done){
    events();
    logic();
    draw();
    SDL_Delay(1.0*33);
  }
}


void args(int ac, char **av){
  if(ac!=1 || av==NULL)
    exit(EXIT_FAILURE);
}



#undef main
int main(int ac, char **av){
  args(ac,av);
  init();
  mainloop();
  return(EXIT_SUCCESS);
}

