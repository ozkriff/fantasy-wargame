/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "structs.h"
#include "core_private.h"
#include "core.h"
#include "utype.h"

static Unit_type
init_peasant (void){
  Unit_type u;
  u.range_of_vision      =   3;
  u.morale               =   3;
  u.count                =   6;
  u.ms                   =   3;
  u.strength             =   2;
  u.toughness            =   2;
  u.attacks              =   3;
  u.armor                =   1;
  u.mvp                  =   4;
  u.energy               =  10;
  u.energy_rg            =   3;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   1;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_swordsman (void){
  Unit_type u;
  u.range_of_vision      =   3;
  u.morale               =   5;
  u.count                =   6;
  u.ms                   =   5;
  u.strength             =   4;
  u.toughness            =   4;
  u.attacks              =   4;
  u.armor                =   3;
  u.mvp                  =   3;
  u.energy               =  20;
  u.energy_rg            =   4;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_foot_knight (void){
  Unit_type u;
  u.range_of_vision      =   2;
  u.morale               =   7;
  u.count                =   6;
  u.ms                   =   6;
  u.strength             =   5;
  u.toughness            =   5;
  u.attacks              =   2;
  u.armor                =   7;
  u.mvp                  =   2;
  u.energy               =  20;
  u.energy_rg            =   4;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_poor_archer (void){
  Unit_type u;
  u.range_of_vision      =   4;
  u.morale               =   5;
  u.count                =   6;
  u.ms                   =   3;
  u.strength             =   2;
  u.toughness            =   3;
  u.attacks              =   3;
  u.armor                =   2;
  u.mvp                  =   4;
  u.energy               =  15;
  u.energy_rg            =   4;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   1;
  u.skills[0] = mk_skill_range(5, 5, 3);
  return(u);
}

static Unit_type
init_scout (void){
  Unit_type u;
  u.range_of_vision      =   5;
  u.morale               =   5;
  u.count                =   6;
  u.ms                   =   3;
  u.strength             =   2;
  u.toughness            =   3;
  u.attacks              =   3;
  u.armor                =   2;
  u.mvp                  =   4;
  u.energy               =  15;
  u.energy_rg            =   4;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   3;
  u.skills[0] = mk_skill_bool(S_IGNR);
  u.skills[1] = mk_skill_bool(S_INVIS);
  u.skills[2] = mk_skill_range(5, 4, 2);
  return(u);
}

static Unit_type
init_goblin (void){
  Unit_type u;
  u.range_of_vision      =   3;
  u.morale               =   2;
  u.count                =   9;
  u.ms                   =   2;
  u.strength             =   2;
  u.toughness            =   2;
  u.attacks              =   2;
  u.armor                =   0;
  u.mvp                  =   3;
  u.energy               =   8;
  u.energy_rg            =   2;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   2;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_goblin_slinger (void){
  Unit_type u;
  u.range_of_vision      =   4;
  u.morale               =   3;
  u.count                =   9;
  u.ms                   =   2;
  u.strength             =   2;
  u.toughness            =   2;
  u.attacks              =   2;
  u.armor                =   1;
  u.mvp                  =   3;
  u.energy               =   8;
  u.energy_rg            =   2;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   1;
  u.skills[0] = mk_skill_range(5, 4, 2);
  return(u);
}

static Unit_type
init_goblin_scout (void){
  Unit_type u;
  u.range_of_vision      =   6;
  u.morale               =   4;
  u.count                =   6;
  u.ms                   =   4;
  u.strength             =   3;
  u.toughness            =   2;
  u.attacks              =   6;
  u.armor                =   1;
  u.mvp                  =   4;
  u.energy               =  15;
  u.energy_rg            =   4;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   2;
  u.skills[0] = mk_skill_bool(S_NORETURN);
  u.skills[0] = mk_skill_bool(S_IGNR);
  u.skills[1] = mk_skill_bool(S_INVIS);
  return(u);
}

static Unit_type
init_orc (void){
  Unit_type u;
  u.range_of_vision      =   3;
  u.morale               =   5;
  u.count                =   6;
  u.ms                   =   3;
  u.strength             =   5;
  u.toughness            =   4;
  u.attacks              =   5;
  u.armor                =   2;
  u.mvp                  =   3;
  u.energy               =  20;
  u.energy_rg            =   5;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_armored_orc (void){
  Unit_type u;
  u.range_of_vision      =   3;
  u.morale               =   7;
  u.count                =   6;
  u.ms                   =   3;
  u.strength             =   5;
  u.toughness            =   4;
  u.attacks              =   3;
  u.armor                =   5;
  u.mvp                  =   2;
  u.energy               =  20;
  u.energy_rg            =   6;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

static Unit_type
init_troll (void){
  Unit_type u;
  u.range_of_vision      =   1;
  u.morale               =   5;
  u.count                =   6;
  u.ms                   =   6;
  u.strength             =   8;
  u.toughness            =   6;
  u.attacks              =   2;
  u.armor                =   2;
  u.mvp                  =   2;
  u.energy               =  60;
  u.energy_rg            =  10;
  u.ter_mvp[T_GRASS    ] =   1;
  u.ter_mvp[T_FOREST   ] =   3;
  u.ter_mvp[T_WATER    ] =   8;
  u.ter_mvp[T_HILLS    ] =   4;
  u.ter_mvp[T_MOUNTEENS] =   9;
  u.ter_ms [T_GRASS    ] =   0;
  u.ter_ms [T_FOREST   ] =   1;
  u.ter_ms [T_WATER    ] =  -2;
  u.ter_ms [T_HILLS    ] =   0;
  u.ter_ms [T_MOUNTEENS] =   0;
  u.skills_n             =   0;
  return(u);
}

void
init_unit_types (){
  utypes[U_PEASANT       ] = init_peasant();
  utypes[U_SWORDSMAN     ] = init_swordsman();
  utypes[U_FOOT_KNIGHT   ] = init_foot_knight();
  utypes[U_POOR_ARCHER   ] = init_poor_archer();
  utypes[U_SCOUT         ] = init_scout();
  utypes[U_GOBLIN        ] = init_goblin();
  utypes[U_GOBLIN_SLINGER] = init_goblin_slinger();
  utypes[U_GOBLIN_SCOUT  ] = init_goblin_scout();
  utypes[U_ORC           ] = init_orc();
  utypes[U_ARMORED_ORC   ] = init_armored_orc();
  utypes[U_TROLL         ] = init_troll();
}

