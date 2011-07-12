libs=-lSDL -lSDL_image -lSDL_ttf 
#flags=-ansi -std=c89 -Wall -Wextra --pedantic -g
flags=-ansi -std=c89 -Wall -Wextra -g
all:
	gcc $(libs) $(flags) main.c -o hex2d
	gcc -lSDL -lSDL_net $(flags) server.c -o server

