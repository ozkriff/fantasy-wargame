
Event_move mk_event_move (uint8_t * d){
  Event_move e;
  e.t    = EVENT_MOVE;
  e.u    = d[1];
  e.dest = mk_mcrd(d[2], d[3]);
  return(e);
}



Event_range mk_event_range (uint8_t * d){
  Event_range e;
  e.t    = EVENT_RANGE;
  e.a    = d[1];
  e.d    = d[2];
  e.md   = mk_mcrd(d[3], d[4]);
  e.dmg  = d[5];
  return(e);
}



Event_melee mk_event_melee (uint8_t * d){
  Event_melee e;
  e.t    = EVENT_MELEE;
  e.a    = d[1];
  e.d    = d[2];
  e.md   = mk_mcrd(d[3], d[4]);
  e.dmg  = d[5];
  return(e);
}



void send_move (Event_move e){
  uint8_t size = 4;
  SDLNet_TCP_Send(socket, &size, 1);

  uint8_t d[4] = {
      (uint8_t)( e.t      ),
      (uint8_t)( e.u      ),
      (uint8_t)( e.dest.x ),
      (uint8_t)( e.dest.y ) };
  SDLNet_TCP_Send(socket, d, 4);
}



void send_melee (Event_melee e){
  uint8_t size = 6;
  SDLNet_TCP_Send(socket, &size, 1);

  uint8_t d[6] = {
      (uint8_t)( e.t      ),
      (uint8_t)( e.a      ),
      (uint8_t)( e.d      ),
      (uint8_t)( e.md.x   ),
      (uint8_t)( e.md.y   ),
      (uint8_t)( e.dmg    )  };
  SDLNet_TCP_Send(socket, d, 6);
}



void send_range (Event_range e){
  uint8_t size = 6;
  SDLNet_TCP_Send(socket, &size, 1);

  uint8_t d[6] = {
      (uint8_t)( e.t      ),
      (uint8_t)( e.a      ),
      (uint8_t)( e.d      ),
      (uint8_t)( e.md.x   ),
      (uint8_t)( e.md.y   ),
      (uint8_t)( e.dmg    )  };
  SDLNet_TCP_Send(socket, d, 6);
}



void get_data (uint8_t * data, uint8_t * size){
  SDLNet_TCP_Recv(socket, size, 1);
  int bytes_recieved = SDLNet_TCP_Recv(socket, data, *size);
  
  if(bytes_recieved != *size){
    printf("recieved: %i, expected: %i", 
        bytes_recieved, (unsigned int)size);
  }
}



/* debug */

void print_data (uint8_t * data, uint8_t size){
  int i;
  for(i=0; i<size; i++)
    printf("%u ", (unsigned int)(data[i]));
  puts("");
}


void init_network (char * hostname, int port){
  IPaddress ip;

  SDLNet_Init();
  SDLNet_ResolveHost(&ip, hostname, port);
  socket = SDLNet_TCP_Open(&ip);
  if(!socket) { puts("cannot open socket"); exit(1); }

  sockets = SDLNet_AllocSocketSet(1);
  SDLNet_TCP_AddSocket(sockets, socket);
}


void construct_event (uint8_t * d){
  if(d[0]==EVENT_MOVE){
    Event_move e = mk_event_move(d);
    add_event((Event*)&e);
  }
  if(d[0]==EVENT_MELEE){
    Event_melee e = mk_event_melee(d);
    add_event((Event*)&e);
  }
  if(d[0]==EVENT_RANGE){
    Event_range e = mk_event_range(d);
    add_event((Event*)&e);
  }
}



void do_network (){
  if(!SDLNet_CheckSockets(sockets, 0)
  || !SDLNet_SocketReady(socket))
    return;

  uint8_t size, data[10];
  
  get_data(data, &size);
  /*print_data(data, size);*/
  construct_event(data);
}

