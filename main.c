
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
#include "accessory.c"
#include "path.c"
#include "logic.c"
#include "events.c"
#include "draw.c"
#include "init.c"
#include "ai.c"


void mainloop(){
  while(!done){
    if(cw->is_ai){
      while(cw->eq->count>0)
        apply_events_to_world();
      ai();
    }

    events();
    logic();
    draw();

    SDL_Delay(1.0*33);
  }
}



#undef main
int main(int ac, char **av){
  init(ac, av);
  mainloop();
  return(EXIT_SUCCESS);
}

