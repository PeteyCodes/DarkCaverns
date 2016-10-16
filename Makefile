# Makefile for building DarkCaverns on Unix systems

all: clean dark

dark: dark.o
	clang -L/usr/local/lib -lSDL2 dark.o -o dark

dark.o:
	clang -c -Wall -Wextra -Wpedantic -g -std=gnu11 -I/usr/local/include dark.c -o dark.o

clean:
	-rm dark *.o
