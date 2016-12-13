CC=gcc
CFLAGS = -lm -Wall

transfer: Util.h clientTCP.c
	$(CC) Util.c clientTCP.c -o client  $(CFLAGS)

clean:
	rm -f client
