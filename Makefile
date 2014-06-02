CFLAGS = -I ./include
LIB    = ./lib/fmod/libfmodex64.so ./libggfonts.so
LFLAGS = $(LIB) -lrt -lX11 -lGLU -lGL -pthread -lm #-lXrandr

all: space

space: space.c ppm.c fmod.c log.c lamMods.c 
	gcc $(CFLAGS) space.c ppm.c log.c fmod.c lamMods.c -Wall -Wextra $(LFLAGS) -o space

clean:
	rm -f space
	rm -f *.o

