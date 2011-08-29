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



Scenario scenario_02 = {
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
  w->map = str2map(scenario_02.map);
  /*player0 units*/{
    add_unit(mk_mcrd(1,2), 0, UNIT_TYPE_DEFENDER, w);
    add_unit(mk_mcrd(1,3), 0, UNIT_TYPE_DEFENDER, w);
    add_unit(mk_mcrd(1,4), 0, UNIT_TYPE_DEFENDER, w);
    add_unit(mk_mcrd(1,5), 0, UNIT_TYPE_HUNTER,   w);
  }
  /*player1 units*/{
    add_unit(mk_mcrd(3,4), 1, UNIT_TYPE_HUNTER,   w);
    add_unit(mk_mcrd(3,5), 1, UNIT_TYPE_DEFENDER, w);
    add_unit(mk_mcrd(3,3), 1, UNIT_TYPE_ARCHER,   w);
  }
}



static void
scenario_logic (){
  /* TODO */
}
