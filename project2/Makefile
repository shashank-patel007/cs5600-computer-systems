#
# file:        Makefile - project 2
# description: compile, link with pthread and zlib (crc32) libraries
#

CC = gcc
LDLIBS=-lz -lpthread
CFLAGS=-ggdb3 -Wall -Wno-format-overflow

EXES = dbserver dbtest

all: $(EXES)

dbtest: dbtest.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

dbserver: dbserver.o queue.o database.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(EXES) *.o /tmp/data.*
