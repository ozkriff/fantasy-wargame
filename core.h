
extern Mcrd    map_size;
extern Unit *  selunit; /* selected unit */
extern Unit_type utypes[3];
extern World * cw; /*current world*/
extern bool    is_local;

void  init (int ac, char ** av);
void  move (Unit * moving_unit, Mcrd destination);
void  attack (Unit * a, Unit * d);
void  endturn ();

void  update_eq ();
Event get_next_event ();
void  apply_event (Event e);

void  select_next_unit ();


/* REMOVE THIS! */
void  add_event (Event e);

