CC = gcc
CFLAGS = -Wall

all: caesar

caesar: caesar_main.o caesar.o
	$(CC) $(CFLAGS) -o caesar caesar_main.o caesar.o

caesar_main.o: caesar_main.c caesar.h
	$(CC) $(CFLAGS) -c caesar_main.c

caesar.o: caesar.c caesar.h
	$(CC) $(CFLAGS) -c caesar.c

clean:
	rm -f *.o caesar