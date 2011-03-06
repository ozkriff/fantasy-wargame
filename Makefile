all:
	gcc -ansi -Wall -Wextra --pedantic -g -std=c99 -lSDL -lSDL_image -lSDL_ttf hex2d.c
	gdb -q a.out -ex run

