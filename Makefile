CC=gcc
CFLAGS = -lm -Wall

transfer: utilities.h
	$(CC) dataLink.c -o transfer  $(CFLAGS)

clean:
	rm -f transfer
