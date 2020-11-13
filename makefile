CC = gcc
LIBS = 
CFLAGS = -DSDL_MAIN_HANDLED -lSDL2 -lSDL2_ttf -Wall -O2
SRC = $(wildcard *.c)

boulder: $(SRC)
	$(CC) -s -o $@ $^ $(CFLAGS) $(LIBS)
