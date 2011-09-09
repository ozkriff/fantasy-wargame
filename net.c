/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <stdbool.h>
#include "SDL/SDL_net.h"
#include "list.h"
#include "structs.h"
#include "misc.h"
#include "core.h"
#include "core_private.h"
#include "net.h"

#define Byte uint8_t

static TCPsocket socket;
static SDLNet_SocketSet sockets;

static Event_move
mk_event_move (Byte * d){
  Event_move e;
  e.t    = E_MOVE;
  e.u    = (int)d[1];
  e.dir  = (int)d[2];
  return(e);
}

static Event_range
mk_event_range (Byte * d){
  Event_range e;
  e.t    = E_RANGE;
  e.a    = (int)d[1];
  e.d    = (int)d[2];
  e.dmg  = (int)d[3];
  return(e);
}

static Event_melee
mk_event_melee (Byte * d){
  Event_melee e;
  e.t                = E_MELEE;
  e.a                = (int)d[1];
  e.d                = (int)d[2];
  e.attackers_killed = (int)d[3];
  e.defenders_killed = (int)d[4];
  return(e);
}

static Event_endturn
mk_event_endturn (Byte * d){
  Event_endturn e;
  e.t          = E_ENDTURN;
  e.old_player = (int)d[1];
  e.new_player = (int)d[2];
  return(e);
}

static Event_death
mk_event_death (Byte *d){
  Event_death e;
  e.t = E_DEATH;
  e.u = *id2unit((int)d[1]);
  return(e);
}

static void
send_move (Event_move e){
  Byte size = 3;
  Byte d[3];
  d[0] = (Byte)e.t;
  d[1] = (Byte)e.u;
  d[2] = (Byte)e.dir;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_melee (Event_melee e){
  Byte size = 5;
  Byte d[5];
  d[0] = (Byte)e.t;
  d[1] = (Byte)e.a;
  d[2] = (Byte)e.d;
  d[3] = (Byte)e.attackers_killed;
  d[4] = (Byte)e.defenders_killed;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_range (Event_range e){
  Byte size = 4;
  Byte d[4];
  d[0] = (Byte)e.t;
  d[1] = (Byte)e.a;
  d[2] = (Byte)e.d;
  d[3] = (Byte)e.dmg;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

/*  */
static void
send_endturn (Event_endturn e){
  Byte size = 3;
  Byte d[3];
  d[0] = (Byte)e.t;
  d[1] = (Byte)e.old_player;
  d[2] = (Byte)e.new_player;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_death (Event_death e){
  Byte size = 2;
  Byte d[2];
  d[0] = (Byte)e.t;
  d[1] = (Byte)e.u.id;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
get_data (Byte * data, Byte * size){
  int bytes_recieved;
  SDLNet_TCP_Recv(socket, size, 1);
  bytes_recieved = SDLNet_TCP_Recv(socket, data, (int)*size);
  if(bytes_recieved != (int)*size){
    printf("recieved: %i, expected: %i", 
        bytes_recieved, (int)size);
  }
}

/* debugging */
/*
static void
print_data (Byte * data, Byte size){
  int i;
  for(i=0; i<size; i++)
    printf("%u ", (unsigned int)(data[i]));
  puts("");
}
*/

void
send_event (Event e) {
  switch(e.t){
    case E_MELEE  : send_melee(e.melee);     break;
    case E_MOVE   : send_move (e.move );     break;
    case E_RANGE  : send_range(e.range);     break;
    case E_ENDTURN: send_endturn(e.endturn); break;
    case E_DEATH  : send_death(e.death);     break;
  }
}

void
send_int_as_uint8 (int n){
  Byte x = (Byte)n;
  SDLNet_TCP_Send(socket, &x, 1);
}

static Event
mk_event (Byte * d){
  Event e;
  if(d[0]==E_MOVE   ) e.move  = mk_event_move (d);
  if(d[0]==E_MELEE  ) e.melee = mk_event_melee(d);
  if(d[0]==E_RANGE  ) e.range = mk_event_range(d);
  if(d[0]==E_ENDTURN) e.endturn = mk_event_endturn(d);
  if(d[0]==E_DEATH  ) e.death = mk_event_death(d);
  return(e);
}

void
do_network (){
  Event e;
  Byte size, data[10];
  if(SDLNet_CheckSockets(sockets, 0)
  && SDLNet_SocketReady(socket)){
    get_data(data, &size);
    /*print_data(data, size);*/
    e = mk_event(data);
    add_event_local(e);
    event2log(e);
  }
}

void
init_network (char * hostname, int port){
  IPaddress ip;
  SDLNet_Init();
  SDLNet_ResolveHost(&ip, hostname, (Uint16)port);
  socket = SDLNet_TCP_Open(&ip);
  if(!socket)
    die("cannot open socket: "
        "host \'%s\', port %i\n",
        hostname, port);
  sockets = SDLNet_AllocSocketSet(1);
  SDLNet_TCP_AddSocket(sockets, socket);
}

void
get_scenario_name_from_server (char * str){
  while(1){
    if(SDLNet_CheckSockets(sockets, 100)==0)
      continue;
    if(SDLNet_SocketReady(socket)){
      /*incoming data size in bytes*/
      Byte size;
      SDLNet_TCP_Recv(socket, &size, 1);
      SDLNet_TCP_Recv(socket, str, (int)size);
      return;
    }
  }
}
