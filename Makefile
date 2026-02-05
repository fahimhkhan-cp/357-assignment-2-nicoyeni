CC = gcc
CFLAGS = -Wall -std=c99 -pedantic
MAIN = fsim
OBJS = asgn2.o

all : $(MAIN)

$(MAIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

asgn2.o : asgn2.c
	$(CC) $(CFLAGS) -c asgn2.c

clean:
	rm -f *.o $(MAIN) core
