/* See LICENSE file for copyright and license details. */

extern Mcrd    map_size;
extern Unit *  selected_unit;
extern Unit_type utypes[3];
extern World * cw; /*current world*/
extern bool    is_local;
extern bool    is_client_active;

void  init ();
void  cleanup();

void  move (Unit * moving_unit, Mcrd destination);
void  attack (Unit * a, Unit * d);
void  select_next_unit ();
void  endturn ();

void  update_eq ();
bool  is_eq_empty ();
Event get_next_event ();
void  apply_event (Event e);

void  init_local_worlds (int n, int *ids);
void  init_local_worlds_s (char *s, ...);
void  set_scenario_id (int id);
void  init_net (char *host, int port);

