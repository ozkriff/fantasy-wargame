/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdlib.h>
#include "list.h"
#include "core.h"
#include "misc.h"

char b[200]; /* [b]uffer */
int ui_done;

void ui_list_units (void){
  Node * n;
  FOR_EACH_NODE(units, n){
    Unit * u = n->d;
    char * s = 
        "id=%i mcrd={%i,%i} plr=%i\n";
    printf(s, u->id, u->m.x, u->m.y, u->player);
  }
  printf("\n");
}

void ui_help (void){
  char * helpstr = 
    "u - units\n"
    "i - unit info\n"
    "s - show map\n"
    "m - move\n"
    "a - attack\n"
    "t - end turn\n"
    "h - help\n"
    "q - quit\n"
    "p - show player id\n";
  puts(helpstr);
}

void ui_move (void){
  Mcrd f, t; /* from, to */
  int n = sscanf(b, "m %i %i %i %i\n", 
      &f.x, &f.y, &t.x, &t.y);
  if(n!=4){
    puts("error.");
  }else{
    printf("move unit from %i.%i to %i.%i\n",
        f.x, f.y, t.x, t.y);
    move(unit_at(f), t);
  }
}

void ui_attack (void){
  Mcrd f, t; /* from, to */
  int n = sscanf(b, "a %i %i %i %i\n", 
      &f.x, &f.y, &t.x, &t.y);
  if(n!=4){
    puts("error.");
  }else{
    printf("attack from %i.%i to %i.%i\n",
        f.x, f.y, t.x, t.y);
    attack(unit_at(f), unit_at(t));
  }
}

void ui_print_player_id (void){
  printf("player->id: %i\n", current_player->id);
}

void ui_print_map (void){
  Mcrd m;
  for(m.y=0; m.y<map_size.y; m.y++){
    if(m.y%2 == 0)
      putchar(' ');
    for(m.x=0; m.x<map_size.x; m.x++){
      /*printf("%i ", tile(m)->type);*/
      printf("%i ", tile(m)->visible);
    }
    putchar('\n');
  }    
}

void ui_unit_info (void){
  Mcrd m;
  int n = sscanf(b, "i %i %i\n", &m.x, &m.y);
  if(n!=2){
    puts("error.");
  }else{
    Unit * u = unit_at(m);
    char * s = 
        "id: %i\n"
        "health: %i\n"
        "player: %i\n";
    printf(s, u->id, u->count, u->player);
  }
}

void ui_end_turn (void){
  endturn();
}

void ui_cmd(char * b){
  switch(b[0]){
    case 'h': ui_help();        break; 
    case 'u': ui_list_units();  break;   
    case 'i': ui_unit_info();   break;
    case 's': ui_print_map();   break;
    case 'm': ui_move();        break;
    case 'a': ui_attack();      break;
    case 't': ui_end_turn();    break;
    case 'q': ui_done = 1;      break;
    case 'p': ui_print_player_id(); break;
  }
}

int main(void){
  init();
  init_local_players_s("hh", 0, 1);
  set_scenario_id(1);
  while(!ui_done){
    printf(">> ");
    fgets(b, 200, stdin);
    ui_cmd(b);
  }
  return(0);
}

