CC = gcc
CFLAGS = -g -Wall
OBJECTS = main.o vlib.o

paging: main.o vlib.o
	$(CC) $(CFLAGS) $(OBJECTS) -o paging
	
%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
.PHONY: clean

clean:
	rm -f *.o *~