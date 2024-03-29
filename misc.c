/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "bool.h"
#include "list.h"
#include "utype.h"
#include "core.h"
#include "misc.h"

void
die(const char *errstr, ...){
  va_list ap;
  va_start(ap, errstr);
  vfprintf(stderr, errstr, ap);
  va_end(ap);
  exit(EXIT_FAILURE);
}

int
str2int (char *str) {
  int n;
  if(sscanf(str, "%i", &n) != 1)
    die("str2int error: \'%s\'\n", str);
  return(n);
}

Mcrd
mk_mcrd (int x, int y){
  Mcrd m;
  m.x = x;
  m.y = y;
  return(m);
}

Skill *
find_skill(Unit *u, Skill_id type){
  int i;
  for(i=0; i<u->skills_n; i++){
    if(u->skills[i].t == type)
      return(&u->skills[i]);
  }
  return(NULL);
}

/* is mcrd equal */
bool
mcrdeq(Mcrd a, Mcrd b){
  return(a.x==b.x && a.y==b.y); 
}

/* Get distance between tiles. */
int
mdist (Mcrd a, Mcrd b) {
  int dx, dy;
  a.x += a.y/2; 
  b.x += b.y/2;
  dx = b.x-a.x;
  dy = b.y-a.y;
  return( (abs(dx) + abs(dy) + abs(dx-dy)) / 2 );
}

/* Get tile's neiborhood by it's index. */
Mcrd
neib (Mcrd a, int neib_index) {
  int d[2][6][2] = {
    { {1,-1}, {1,0}, {1,1}, { 0,1}, {-1,0}, { 0,-1}, },
    { {0,-1}, {1,0}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, } };
  int dx = d[a.y%2][neib_index][0];
  int dy = d[a.y%2][neib_index][1];
  return( mk_mcrd(a.x+dx, a.y+dy) );
}

/* Get tile's opposite neiborhood. */
int
opposite_neib_index (int i){
  int d[] = {3, 4,2, 5,1, 0};
  return(i + d[i]);
}

int
mcrd2index(Mcrd a, Mcrd b){
  int i;
  for(i=0; i<6; i++){
    if(mcrdeq(neib(a,i), b))
      return(i);
  }
  die("mcrd2tile error.");
  return(0);
}

/*------------------------------------------------------*/

/* mcrd2tile */
Tile *
tile (Mcrd c){
  return(map + map_size.x*c.y + c.x);
}

/* is tile inboard */
bool
inboard (Mcrd t){
  bool is_x_ok = t.x >= 0 && t.x < map_size.x;
  bool is_y_ok = t.y >= 0 && t.y < map_size.y;
  return(is_x_ok && is_y_ok);
}

Unit *
id2unit (int id){
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u = node->d;
    if(u->id == id)
      return(u);
  }
  return(NULL);
}

Unit *
unit_at (Mcrd crd){
  Node *node;
  FOR_EACH_NODE(units, node){
    Unit *u = node->d;
    if(mcrdeq(u->m, crd))
      return(u);
  }
  return(NULL);
}

void
fixnum (int min, int max, int *n){
  if(*n < min)
    *n = min;
  if(*n > max)
    *n = max;
}

int
rnd (int min, int max){
  if(max != min)
    return(rand()%(max-min)+min);
  else
    return(max);
}

/*Compare strings till first space or tab.
  strcmp_sp("lo 2 3", "lo %i %i") -> true
  strcmp_sp("no 2 3", "lo %i %i") -> false */
bool
strcmp_sp (char *s1, char *s2){
  while(s2 && s1){
    if(*s1 != *s2)
      return(false);
    if(*s1==' ' || *s1=='\t')
      return(true);
    s1++, s2++;
  }
  return(true);
}

/*get tile type corresponding to character*/
int
char2tiletype (char c){
  if(c=='.') return(T_GRASS);
  if(c=='t') return(T_FOREST);
  if(c=='*') return(T_WATER);
  if(c=='h') return(T_HILLS);
  if(c=='M') return(T_MOUNTAINS);
  die("misc: char2tiletype(): "
      "unknown tiletype '%c'\n", c);
  return(0);
}

Tile *
str2map (char *s){
  Tile *i; /*used for iteration through 'map'*/
  Tile *map = malloc(sizeof(Tile)
      * map_size.x * map_size.y);
  i = map;
  while(s && *s!=0){
    if(*s!=' ' && *s!='\n' && *s!='\0'){
      i->t = char2tiletype(*s);
      i++;
    }
    s++;
  }
  return(map);
}

