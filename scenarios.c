/* See LICENSE file for copyright and license details. */

#include "bool.h"
#include "list.h"
#include "utype.h"
#include "core.h"
#include "scenarios.h"

Scenario scenarios[20];

void
init_scenarios (void){
  scenarios[0] = scenario_01;
  scenarios[1] = scenario_02;
}

