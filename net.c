
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

static Event_move  mk_event_move (Byte * d);
static Event_range mk_event_range(Byte * d);
static Event_melee mk_event_melee(Byte * d);

static void        send_move (Event_move e);
static void        send_melee(Event_melee e);
static void        send_range(Event_range e);

static void        get_data (Byte * data, Byte * size);
static void        print_data (Byte * data, Byte size);
static Event       mk_event (Byte * d);


static Event_move
mk_event_move (Byte * d){
  Event_move e;
  e.t    = EVENT_MOVE;
  e.u    = d[1];
  e.dest = mk_mcrd(d[2], d[3]);
  return(e);
}



static Event_range
mk_event_range (Byte * d){
  Event_range e;
  e.t    = EVENT_RANGE;
  e.a    = d[1];
  e.d    = d[2];
  e.dmg  = d[3];
  return(e);
}



static Event_melee
mk_event_melee (Byte * d){
  Event_melee e;
  e.t    = EVENT_MELEE;
  e.a    = d[1];
  e.d    = d[2];
  e.attackers_killed = d[3];
  e.defenders_killed = d[4];
  return(e);
}



static void
send_move (Event_move e){
  Byte size = 4;
  Byte d[4];
  SDLNet_TCP_Send(socket, &size, 1);
  d[0] = e.t;
  d[1] = e.u;
  d[2] = e.dest.x;
  d[3] = e.dest.y;
  SDLNet_TCP_Send(socket, d, size);
}



static void
send_melee (Event_melee e){
  Byte size = 5;
  Byte d[5];
  SDLNet_TCP_Send(socket, &size, 1);
  d[0] = e.t;
  d[1] = e.a;
  d[2] = e.d;
  d[3] = e.attackers_killed;
  d[4] = e.defenders_killed;
  SDLNet_TCP_Send(socket, d, size);
}



static void
send_range (Event_range e){
  Byte size = 4;
  Byte d[4];
  SDLNet_TCP_Send(socket, &size, 1);
  d[0] = e.t;
  d[1] = e.a;
  d[2] = e.d;
  d[3] = e.dmg;
  SDLNet_TCP_Send(socket, d, size);
}



static void
get_data (Byte * data, Byte * size){
  int bytes_recieved;
  SDLNet_TCP_Recv(socket, size, 1);
  bytes_recieved = SDLNet_TCP_Recv(socket, data, *size);
  if(bytes_recieved != *size){
    printf("recieved: %i, expected: %i", 
        bytes_recieved, (unsigned int)size);
  }
}



static void
print_data (Byte * data, Byte size){
  int i;
  for(i=0; i<size; i++)
    printf("%u ", (unsigned int)(data[i]));
  puts("");
}



void
send_event (Event e) {
  switch(e.t){
    case EVENT_MELEE: send_melee(e.melee); break;
    case EVENT_MOVE : send_move (e.move ); break;
    case EVENT_RANGE: send_range(e.range); break;
  }
}



void
send_int_as_uint8 (int n){
  Byte x = n;
  SDLNet_TCP_Send(socket, &x, 1);
}



static Event
mk_event (Byte * d){
  Event e;
  if(d[0]==EVENT_MOVE ) e.move  = mk_event_move (d);
  if(d[0]==EVENT_MELEE) e.melee = mk_event_melee(d);
  if(d[0]==EVENT_RANGE) e.range = mk_event_range(d);
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
  SDLNet_ResolveHost(&ip, hostname, port);
  socket = SDLNet_TCP_Open(&ip);
  if(!socket) {
    puts("cannot open socket"); 
    exit(1); 
  }
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
      SDLNet_TCP_Recv(socket, str, size);
      return;
    }
  }
}
