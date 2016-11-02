CC=gcc
CFLAGS = -lm -Wall

transfer: utilities.h dataLink.c
	$(CC) utilities.c dataLink.c -o transfer  $(CFLAGS)

clean:
	rm -f transfer
