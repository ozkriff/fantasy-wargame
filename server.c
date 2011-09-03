/* See LICENSE file for copyright and license details. */

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "SDL/SDL_net.h"
#include "list.h"

#define PRINT_DATA false

typedef struct Client {
  TCPsocket sock;
  List players;
} Client;



List clients = {0, 0, 0};

TCPsocket listening_socket;
SDLNet_SocketSet sockets;



/* read through scenario to get players_count */
static int
get_players_count_from_scenario (char * filename){
  int players_count;
  char line[100];
  FILE * cfg = fopen(filename, "r");
  if(!cfg){
    puts("Can't open scenario.");
    exit(EXIT_FAILURE);
  }
  while( fgets(line, 90, cfg) ){
    /* Skip comments and empty lines. */
    if(line[0]=='#' || line[0]=='\n')
      continue;
    if(!strncmp("[NUM-OF-PLAYERS]", line, 15))
      sscanf(line, "[NUM-OF-PLAYERS] %i", &players_count);
  }
  fclose(cfg);
  return(players_count);
}



static bool
is_id_free (int id){
  Node * client_node;
  FOR_EACH_NODE(clients, client_node){
    Client * c = client_node->d;
    Node * id_node;
    FOR_EACH_NODE(c->players, id_node)
      if(id == *(int*)id_node->d)
        return(false);
  }
  return(true);
}



static void
wait_for_all_players (int players_count){
  Client * c;
  while(clients.count < players_count) {
    if( SDLNet_CheckSockets(sockets, 100)
    && SDLNet_SocketReady(listening_socket) ){
      TCPsocket sock = SDLNet_TCP_Accept(listening_socket);
      SDLNet_TCP_AddSocket(sockets, sock);
      c = calloc(1, sizeof(Client));
      c->sock = sock;
      while(1){
        int data = 0;
        int * player_id;
        SDLNet_TCP_Recv(sock, &data, 1);
        /*mark that all players are already sent*/
        if(data == 0xff)
          break;
        if(!is_id_free(data)){
          printf("id [%i] is not free!", data);
          continue;
        }
        player_id = calloc(1, sizeof(int));
        *player_id = data;
        l_push(&c->players, player_id);
        printf("added player to client...\n");
      }
      l_push(&clients, c);
      printf("added client...\n");
    }
  }
}



static void
send_scenario_name_to_clients (char * scenario_name){
  uint8_t size = strlen(scenario_name) + 1;
  Node * n;
  FOR_EACH_NODE(clients, n){
    Client * c = n->d;
    SDLNet_TCP_Send(c->sock, &size, 1);
    SDLNet_TCP_Send(c->sock, scenario_name, (int)size);
  }
}



static void
init (int ac, char **av){
  int port;
  int players_count;
  IPaddress ip;
  if(ac != 3){
    puts("usage: ./server [port] [scenario]");
    exit(EXIT_FAILURE);
  }
  /* get port */
  sscanf(av[1], "%i", &port);
  players_count = get_players_count_from_scenario(av[2]);
  SDLNet_Init();
  /* allocate memory for each client + for server */
  sockets = SDLNet_AllocSocketSet(players_count+1);
  SDLNet_ResolveHost(&ip, NULL, (Uint16)port);
  listening_socket = SDLNet_TCP_Open(&ip);
  SDLNet_TCP_AddSocket(sockets, listening_socket);
  wait_for_all_players(players_count);
  send_scenario_name_to_clients(av[2]);
}



static void
cleanup (){
  SDLNet_Quit();
}



static void
receive_data (Client *c, uint8_t *data, uint8_t *size)
{
  /* receive size of data, then data */
  SDLNet_TCP_Recv(c->sock, size, 1);
  SDLNet_TCP_Recv(c->sock, data, (int)*size);
}



#if PRINT_DATA
static void
print_data (uint8_t * data, uint8_t size){
  int i;
  for(i=0; i<size; i++)
    printf("%u ", (unsigned int)(data[i]));
  puts("");
}
#endif



/* Resend data to all clients, except <Client * exception> 
  who has sent that data to us.
  Send data size (in bytes), then data. */

static void
resend_data (Client * exception, uint8_t * data, uint8_t size){
  Node * n;
  FOR_EACH_NODE(clients, n){
    Client * c = n->d;
    if(c != exception){
      SDLNet_TCP_Send(c->sock, &size, 1);
      SDLNet_TCP_Send(c->sock, data, (int)size);
    }
  }
}



static void
mainloop (){
  uint8_t data_size;
  uint8_t data[32];
  int active_sockets_count;
  while(1){
    Node * n;
    active_sockets_count = SDLNet_CheckSockets(sockets, 1000);
    n = clients.h;
    while(active_sockets_count > 0 && n) {
      Client * c = n->d;
      if(SDLNet_SocketReady(c->sock)){
        receive_data(c, data, &data_size);
#if PRINT_DATA
        print_data(data, data_size);
#endif
        resend_data(c, data, data_size);
        active_sockets_count--;
      }
      n = n->n;
    }
  }
}



#undef main
int
main (int ac, char **av){
  init(ac, av);
  mainloop();
  cleanup();
  return(EXIT_SUCCESS);
}

