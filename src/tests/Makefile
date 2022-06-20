
all: storage

CFLAGS=-Wall -O3

storage: storage.o nodeman.o nodepage.o error.o
	$(CC) $^ -o $@

nodepage.o: ../nodepage.c ../nodepage.h
	$(CC) $(CFLAGS) -c ../nodepage.c

nodeman.o: ../nodeman.c ../nodeman.h
	$(CC) $(CFLAGS) -c ../nodeman.c

error.o: ../error.c ../error.h
	$(CC) $(CFLAGS) -c ../error.c

clean:
	rm storage *.o
