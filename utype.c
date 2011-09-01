/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "structs.h"
#include "core_private.h"
#include "core.h"
#include "utype.h"



static Unit_type
init_defender (){
  Unit_type u;
  u.range_of_vision         =  1;
  u.morale                  =  5;
  u.count                   = 10;
  u.ms                      =  3;
  u.strength                =  3;
  u.toughness               =  4;
  u.attacks                 =  1;
  u.armor                   =  3;
  u.mvp                     =  3;
  u.ter_mvp[TILE_GRASS    ] =  1;
  u.ter_mvp[TILE_FOREST   ] =  4;
  u.ter_mvp[TILE_WATER    ] =  8;
  u.ter_mvp[TILE_HILLS    ] =  4;
  u.ter_mvp[TILE_MOUNTEENS] =  9;
  u.ter_ms [TILE_GRASS    ] =  0;
  u.ter_ms [TILE_FOREST   ] =  1;
  u.ter_ms [TILE_WATER    ] = -2;
  u.ter_ms [TILE_HILLS    ] =  0;
  u.ter_ms [TILE_MOUNTEENS] =  0;
  u.skills_n = 0;
  return(u);
}



static Unit_type
init_hunter (){
  Unit_type u;
  u.range_of_vision         =  4;
  u.morale                  =  5;
  u.count                   = 10;
  u.ms                      =  3;
  u.strength                =  3;
  u.toughness               =  3;
  u.attacks                 =  2;
  u.armor                   =  1;
  u.mvp                     =  4;
  u.ter_mvp[TILE_GRASS    ] =  1;
  u.ter_mvp[TILE_FOREST   ] =  2;
  u.ter_mvp[TILE_WATER    ] =  4;
  u.ter_mvp[TILE_HILLS    ] =  3;
  u.ter_mvp[TILE_MOUNTEENS] =  6;
  u.ter_ms [TILE_GRASS    ] =  0;
  u.ter_ms [TILE_FOREST   ] =  2;
  u.ter_ms [TILE_WATER    ] = -2;
  u.ter_ms [TILE_HILLS    ] =  1;
  u.ter_ms [TILE_MOUNTEENS] =  1;
  u.skills_n = 3;
  u.skills[0] = mk_skill_bool(SKILL_IGNR    );
  u.skills[1] = mk_skill_bool(SKILL_INVIS   );
  u.skills[2] = mk_skill_bool(SKILL_NORETURN);
  return(u);
}



static Unit_type
init_archer (){
  Unit_type u;
  u.range_of_vision         =  3;
  u.morale                  =  5;
  u.count                   = 10;
  u.ms                      =  2;
  u.strength                =  3;
  u.toughness               =  2;
  u.attacks                 =  1;
  u.armor                   =  1;
  u.mvp                     =  3;
  u.ter_mvp[TILE_GRASS    ] =  1;
  u.ter_mvp[TILE_FOREST   ] =  3;
  u.ter_mvp[TILE_WATER    ] =  8;
  u.ter_mvp[TILE_HILLS    ] =  4;
  u.ter_mvp[TILE_MOUNTEENS] =  9;
  u.ter_ms [TILE_GRASS    ] =  0;
  u.ter_ms [TILE_FOREST   ] =  1;
  u.ter_ms [TILE_WATER    ] = -2;
  u.ter_ms [TILE_HILLS    ] =  0;
  u.ter_ms [TILE_MOUNTEENS] =  0;
  u.skills_n = 1;
  u.skills[0] = mk_skill_range(3, 5, 4);
  return(u);
}



void
init_unit_types (){
  utypes[UNIT_TYPE_DEFENDER] = init_defender();
  utypes[UNIT_TYPE_HUNTER  ] = init_hunter();
  utypes[UNIT_TYPE_ARCHER  ] = init_archer();
}
