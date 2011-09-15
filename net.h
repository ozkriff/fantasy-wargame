/* See LICENSE file for copyright and license details. */

void init_network (char * hostname, int port);
void do_network ();
void send_int_as_uint8 (int n);
int  get_scenario_from_server();
void send_event (Event e);
