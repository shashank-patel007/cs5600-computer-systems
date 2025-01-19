CC = gcc
CFLAGS = -Wall

all: queue

queue: queue_main.o queue.o
	$(CC) $(CFLAGS) -o queue queue_main.o queue.o

queue_main.o: queue_main.c queue.h
	$(CC) $(CFLAGS) -c queue_main.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f *.o queue