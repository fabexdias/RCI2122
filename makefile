# makefile
CC = gcc
CFLAGS = -Wall

OBJECTS = ring.o verify.o link.o

ring: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

ring.o: ring.c ring.h verify.h link.h

verify.o: verify.c verify.h

link.o: link.c link.h

clean:
	rm -f *.o