/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include "list.h"
#include "structs.h"
#include "scenarios.h"

Scenario scenarios[20];

void
init_scenarios(){
  scenarios[0] = scenario_01;
  scenarios[1] = scenario_02;
}

