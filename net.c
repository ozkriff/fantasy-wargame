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

static Event
mk_event_move (Byte * d){
  Event e;
  e.t = E_MOVE;
  e.e.move.u    = (int)d[1];
  e.e.move.dir  = (int)d[2];
  e.e.move.cost = (int)d[3];
  return(e);
}

static Event
mk_event_range (Byte * d){
  Event e;
  e.t = E_RANGE;
  e.e.range.a   = (int)d[1];
  e.e.range.d   = (int)d[2];
  e.e.range.dmg = (int)d[3];
  return(e);
}

static Event
mk_event_melee (Byte * d){
  Event e;
  e.t = E_MELEE;
  e.e.melee.a                = (int)d[1];
  e.e.melee.d                = (int)d[2];
  e.e.melee.attackers_killed = (int)d[3];
  e.e.melee.defenders_killed = (int)d[4];
  return(e);
}

static Event
mk_event_endturn (Byte * d){
  Event e;
  e.t = E_ENDTURN;
  e.e.endturn.old_id = (int)d[1];
  e.e.endturn.new_id = (int)d[2];
  return(e);
}

static Event
mk_event_death (Byte *d){
  Event e;
  e.t = E_DEATH;
  e.e.death.id = (int)d[1];
  return(e);
}

static void
send_move (Event_move e){
  Byte size = 4;
  Byte d[4];
  d[0] = E_MOVE;
  d[1] = (Byte)e.u;
  d[2] = (Byte)e.dir;
  d[3] = (Byte)e.cost;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_melee (Event_melee e){
  Byte size = 5;
  Byte d[5];
  d[0] = E_MELEE;
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
  d[0] = E_RANGE;
  d[1] = (Byte)e.a;
  d[2] = (Byte)e.d;
  d[3] = (Byte)e.dmg;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_endturn (Event_endturn e){
  Byte size = 3;
  Byte d[3];
  d[0] = E_ENDTURN;
  d[1] = (Byte)e.old_id;
  d[2] = (Byte)e.new_id;
  SDLNet_TCP_Send(socket, &size, 1);
  SDLNet_TCP_Send(socket, d, (int)size);
}

static void
send_death (Event_death e){
  Byte size = 2;
  Byte d[2];
  d[0] = E_DEATH;
  d[1] = (Byte)e.id;
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
    case E_MELEE:   send_melee(e.e.melee);     break;
    case E_MOVE:    send_move(e.e.move);       break;
    case E_RANGE:   send_range(e.e.range);     break;
    case E_ENDTURN: send_endturn(e.e.endturn); break;
    case E_DEATH:   send_death(e.e.death);     break;
    default:
      die("net: send_event(): "
          "Unknown event '%i'\n", e.t);
      break;
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
  int type = (int)d[0];
  switch(type){
    case E_MOVE:    e = mk_event_move(d);    break;
    case E_MELEE:   e = mk_event_melee(d);   break;
    case E_RANGE:   e = mk_event_range(d);   break;
    case E_ENDTURN: e = mk_event_endturn(d); break;
    case E_DEATH:   e = mk_event_death(d);   break;
    default:
      die("net: mk_event(): "
          "Unknown event '%i'\n", type);
      break;
  }
  return(e); 
}

void
do_network (void){
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
  if(!socket){
    die("cannot open socket: "
        "host \'%s\', port %i\n",
        hostname, port);
  }
  sockets = SDLNet_AllocSocketSet(1);
  SDLNet_TCP_AddSocket(sockets, socket);
}

int
get_scenario_from_server (void){
  while(1){
    if(SDLNet_CheckSockets(sockets, 100)==0)
      continue;
    if(SDLNet_SocketReady(socket)){
      Byte id;
      SDLNet_TCP_Recv(socket, &id, 1);
      return((int)id);
    }
  }
}
