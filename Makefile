# See LICENSE file for copyright and license details.

all: ui_sdl ui_cli server

CC=gcc
CFLAGS=-g -ansi -std=c89 -Wall -Wextra --pedantic
common_obj= \
  list.o \
  path.o \
  misc.o \
  core.o \
  net.o \
  ai.o \
  utype.o \
  scenarios.o \
  scenario_01.o \
  scenario_02.o

cli_obj=ui_cli.o $(common_obj)
cli_lib=-lSDL_net
ui_cli: $(cli_obj)
	$(CC) $(CFLAGS) -o ui_cli $(cli_obj) $(cli_lib)

sdl_obj=ui_sdl.o $(common_obj)
sdl_lib=-lSDL -lSDL_image -lSDL_ttf -lSDL_net
ui_sdl: $(sdl_obj)
	$(CC) $(CFLAGS) -o ui_sdl $(sdl_obj) $(sdl_lib)
	
server: server.o list.o
	$(CC) $(CFLAGS) -o server server.o list.o -lSDL_net

list.o:   list.c          list.h
path.o:   path.c   path.h list.h structs.h
misc.o:   misc.c   misc.h list.h structs.h
core.o:   core.c   core.h list.h structs.h core_private.h
net.o:    net.c    net.h  list.h structs.h core_private.h 
ai.o:     ai.c     ai.h   list.h structs.h
utype.o:  utype.c  utype.h       structs.h core_private.h
ui_cli.o: ui_cli.c core.h list.h structs.h misc.h
ui_sdl.o: ui_sdl.c core.h list.h structs.h misc.h
server.o: server.c        list.h

scenarios.o:   scenarios.h structs.h
scenario_01.o: scenario_01.c scenarios.h structs.h
scenario_02.o: scenario_02.c scenarios.h structs.h

clean:
	rm -f *.o ui_cli ui_sdl server
