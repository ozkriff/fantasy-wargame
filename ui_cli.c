
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdlib.h>
#include "list.h"
#include "structs.h"
#include "core.h"
#include "misc.h"


char b[200]; /* [b]uffer */
int ui_done;


void ui_list_units(){
  Node * n;
  FOR_EACH_NODE(cw->units, n){
    Unit * u = n->d;
    char * s = 
        "id=%i mcrd={%i,%i} plr=%i\n";
    printf(s, u->id, u->mcrd.x, u->mcrd.y, u->player);
  }
  printf("\n");
}



void ui_help(){
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



void ui_move(){
  Mcrd f, t; /* from, to */
  int n = sscanf(b, "m %i %i %i %i\n", 
      &f.x, &f.y, &t.x, &t.y);
  if(n!=4){
    puts("error.");
  }else{
    printf("move unit from %i.%i to %i.%i\n",
        f.x, f.y, t.x, t.y);
    move(find_unit_at(f), t);
  }
}



void ui_attack(){
  Mcrd f, t; /* from, to */
  int n = sscanf(b, "a %i %i %i %i\n", 
      &f.x, &f.y, &t.x, &t.y);
  if(n!=4){
    puts("error.");
  }else{
    printf("attack from %i.%i to %i.%i\n",
        f.x, f.y, t.x, t.y);
    attack(find_unit_at(f), find_unit_at(t));
  }
}



void ui_print_cw_id(){
  printf("cw->id: %i\n", cw->id);
}



void ui_print_map(){
  Mcrd m;
  for(m.y=0; m.y<map_size.y; m.y++){
    if(m.y%2 == 0)
      putchar(' ');
    for(m.x=0; m.x<map_size.x; m.x++){
      /*printf("%i ", tile(m)->type);*/
      printf("%i ", tile(m)->fog);
    }
    putchar('\n');
  }    
}



void ui_unit_info(){
  Mcrd m;
  int n = sscanf(b, "i %i %i\n", &m.x, &m.y);
  if(n!=2){
    puts("error.");
  }else{
    Unit * u = find_unit_at(m);
    char * s = 
        "id: %i\n"
        "health: %i\n"
        "player: %i\n";
    printf(s, u->id, u->count, u->player);
  }
}



void ui_end_turn(){
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
    case 'p': ui_print_cw_id(); break;
  }
}



int main(int ac, char **av){
#if 0
  char *s[7] = {
    "./ui-cli",
    "-local",
    "scenario2",
    "-human",
    "0",
    "-human",
    "1" };
  ac = 7;
  av = s;
#endif
  init(ac, av);
  while(!ui_done){
    printf(">> ");
    fgets(b, 200, stdin);
    ui_cmd(b);
  }
  return(0);
}

