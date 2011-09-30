/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "list.h"
#include "structs.h"
#include "utype.h"
#include "misc.h"
#include "core.h"
#include "core_private.h"
#include "scenarios.h"

static void init_scenario(World *w);
static void scenario_logic(void);

Scenario scenario_02 = {
  2, /*players_count*/
  {12, 12}, /*map_size*/
  " . . * . . h * . . . . ."
  ". . . t . M . . . . . . "
  " . . . . . . . t . . . ."
  ". . . . . . h M . . . . "
  " . . . . . . h . . . . ."
  ". . . . . t . . . . . . "
  " . . . . . t . . . . . ."
  ". . . . h . . . . . . . "
  " . . . . M . . . t . . ."
  ". . * . h . . . t t . . "
  " . * . . . . . t . . . ."
  ". . * . . * . . . . . . ",
  init_scenario, 
  scenario_logic
};

static void
player_0_units (World *w){
  add_unit(mk_mcrd( 0,  3), 0, U_PEASANT, w);
  add_unit(mk_mcrd( 0,  4), 0, U_SCOUT, w);
  add_unit(mk_mcrd( 0,  5), 0, U_PEASANT, w);
  add_unit(mk_mcrd( 0,  6), 0, U_PEASANT, w);
  add_unit(mk_mcrd( 1,  4), 0, U_SWORDSMAN, w);
  add_unit(mk_mcrd( 1,  5), 0, U_POOR_ARCHER, w);
  add_unit(mk_mcrd( 1,  6), 0, U_FOOT_KNIGHT, w);
  add_unit(mk_mcrd( 1,  7), 0, U_SWORDSMAN, w);
}

static void
player_1_units (World *w){
  add_unit(mk_mcrd( 8,  4), 1, U_GOBLIN, w);
  add_unit(mk_mcrd( 8,  5), 1, U_GOBLIN, w);
  add_unit(mk_mcrd( 8,  6), 1, U_ORC, w);
  add_unit(mk_mcrd( 8,  7), 1, U_GOBLIN, w);
  add_unit(mk_mcrd( 9,  3), 1, U_GOBLIN_SCOUT, w);
  add_unit(mk_mcrd( 9,  7), 1, U_GOBLIN_SLINGER, w);
  add_unit(mk_mcrd( 9,  4), 1, U_ARMORED_ORC, w);
  add_unit(mk_mcrd(10,  4), 1, U_TROLL, w);
  add_unit(mk_mcrd(10,  5), 1, U_ORC, w);
}



static void
init_scenario (World *w){
  w->map = str2map(scenario_02.map);
  player_0_units(w);
  player_1_units(w);
}

static void
scenario_logic (void){
  /* TODO */
}

