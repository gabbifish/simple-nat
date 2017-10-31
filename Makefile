# for C++ define  CC = g++
CC = gcc
CFLAGS  = -g -Wall

default: nat

nat:  nat.o
	$(CC) $(CFLAGS) -o nat nat.o

nat.o:  nat.c nat.h
	$(CC) $(CFLAGS) -c nat.c

clean:
	$(RM) count *.o *~
