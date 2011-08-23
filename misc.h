
#define FOR_EACH_MCRD(mc) \
  for(mc.y=0; mc.y<map_size.y; mc.y++) \
    for(mc.x=0; mc.x<map_size.x; mc.x++)

int    str2int (char * str);
Mcrd   mk_mcrd (int x, int y);
bool   mcrdeq (Mcrd a, Mcrd b);
int    mdist (Mcrd a, Mcrd b);
Mcrd   neib (Mcrd a, int i);
int    opposite_neib_index (int i);
int    mcrd2index (Mcrd a, Mcrd b);
void   fixnum (int min, int max, int *n);
int    rnd (int min, int max);
bool   strcmp_sp (char *s1, char *s2);

Feature * find_feature (Unit * u, int type);

/* ----------------------------------------------------- */

Tile * tile (Mcrd c);
bool   inboard (Mcrd t);
Unit * id2unit (int id);
Unit * find_unit_at (Mcrd crd);

