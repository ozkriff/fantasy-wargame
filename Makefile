libs=-lSDL -lSDL_image -lSDL_ttf 
flags=-ansi -Wall -Wextra --pedantic -g -std=c99 
all:
	gcc $(libs) $(flags) main.c -o hex2d

