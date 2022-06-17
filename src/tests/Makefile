
all: storage

CFLAGS="-Wall"

storage: storage.o nodepage.o error.o
	$(CC) $^ -o $@

nodepage.o: ../nodepage.c ../nodepage.h
	$(CC) $(CFLAGS) -c ../nodepage.c

error.o: ../error.c ../error.h
	$(CC) $(CFLAGS) -c ../error.c

clean:
	rm storage *.o
