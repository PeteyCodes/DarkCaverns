# Makefile for building DarkCaverns on Unix systems

target = dark
src = $(wildcard src/*.c)
obj = $(src:.c=.o)

INCLUDES = -I/usr/local/include
CFLAGS = -c -Wall -Wextra -Wpedantic -DHAVE_ASPRINTF -g -O0 -std=gnu11
LDFLAGS = -L/usr/local/lib -lSDL2


$(target): $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $<

all: clean $(target)
.PHONY: clean

clean:
	-rm $(target) 
	-rm src/*.o

run: clean $(target)
	echo "Running..."
	./$(target)
