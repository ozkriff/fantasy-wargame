/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "list.h"
#include "structs.h"
#include "misc.h"
#include "core.h"
#include "core_private.h"
#include "scenarios.h"



static void init_scenario(World *w);
static void scenario_logic();



Scenario scenario_01 = {
  2, /*players_count*/
  {7, 14}, /*map_size*/
  " . . * . . . *"
  ". . . t . M . "
  " . . . . . . ."
  ". . . . . . . "
  " . . . . . . ."
  ". . . . . . . "
  " . . . . . * ."
  ". . * * * . . "
  " . * h h * . ."
  ". * h M h * . "
  " . * h h * . ."
  ". . * * * . . "
  " . . . . . . ."
  ". . . . . . . ",
  init_scenario, 
  scenario_logic
};



static void
init_scenario (World *w){
  w->map = str2map(scenario_01.map);
  /*player0 units*/{
    add_unit(mk_mcrd(1,2), 0, U_DEFENDER, w);
  }
  /*player1 units*/{
    add_unit(mk_mcrd(3,5), 1, U_DEFENDER, w);
  }
}



static void
scenario_logic (){
  /* TODO */
}

