CC = gcc
CFLAGS = -Wall

all: caesar queue ciphered_queue

caesar: caesar_main.o caesar.o
	$(CC) $(CFLAGS) -o caesar caesar_main.o caesar.o

queue: queue_main.o queue.o
	$(CC) $(CFLAGS) -o queue queue_main.o queue.o

ciphered_queue: ciphered_queue_main.o queue.o caesar.o
	$(CC) $(CFLAGS) -o ciphered_queue ciphered_queue_main.o queue.o caesar.o

caesar_main.o: caesar_main.c caesar.h
	$(CC) $(CFLAGS) -c caesar_main.c

caesar.o: caesar.c caesar.h
	$(CC) $(CFLAGS) -c caesar.c

queue_main.o: queue_main.c queue.h
	$(CC) $(CFLAGS) -c queue_main.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

ciphered_queue_main.o: ciphered_queue_main.c queue.h caesar.h
	$(CC) $(CFLAGS) -c ciphered_queue_main.c

clean:
	rm -f *.o caesar queue ciphered_queue