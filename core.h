
extern Mcrd    map_size;
extern Unit *  selunit; /* selected unit */
extern Unit_type utypes[3];
extern World * cw; /*current world*/
extern bool    is_local;
extern bool    is_active; /* TODO: describe */

void  init (int ac, char ** av);
void  cleanup();

void  move (Unit * moving_unit, Mcrd destination);
void  attack (Unit * a, Unit * d);
void  select_next_unit ();
void  endturn ();

void  update_eq ();
Event get_next_event ();
void  apply_event (Event e);


