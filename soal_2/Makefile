CC = gcc
CFLAGS = -Wall

all: orion eternal

server: orion.c arena.h
	$(CC) $(CFLAGS) orion.c -o orion
client: eternal.c arena.h
	$(CC) $(CFLAGS) eternal.c -o eternal

clean:
	rm -f orion eternal
clean_ipc:
	ipcrm -m 1 || true
	ipcrm -q 1 || true
