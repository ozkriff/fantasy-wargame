/* See LICENSE file for copyright and license details. */

typedef enum {
  /*humans*/
  U_PEASANT,
  U_MILITIAMAN,
  U_SPEARMAN,
  U_SWORDSMAN,
  U_FOOT_KNIGHT,
  U_KNIGHT,
  U_POOR_ARCHER,
  U_ARCHER,
  U_SCOUT,
  /*goblins*/
  U_WEAK_GOBLIN,
  U_GOBLIN,
  U_GOBLIN_SLINGER,
  U_GOBLIN_SCOUT,
  /*orcs*/
  U_YOUNG_ORC,
  U_ORC,
  U_ARMORED_ORC,
  U_CRAZY_ORC,
  /*trolls*/
  U_YOUNG_TROLL,
  U_TROLL
} Unit_type_id;

void init_unit_types (void);

