
all: storage check_ut

CFLAGS=-Wall -O3

storage: storage.o nodeman.o nodepage.o error.o unpacked.o
	$(CC) $^ -o $@

check_ut: check_ut.o unique.o nodeman.o nodepage.o error.o unpacked.o
	$(CC) $^ -o $@

unique.o: ../unique.c ../unique.h
	$(CC) $(CFLAGS) -c ../unique.c

nodepage.o: ../nodepage.c ../nodepage.h
	$(CC) $(CFLAGS) -c ../nodepage.c

nodeman.o: ../nodeman.c ../nodeman.h
	$(CC) $(CFLAGS) -c ../nodeman.c

unpacked.o: ../unpacked.h ../unpacked.c
	$(CC) $(CFLAGS) -c ../unpacked.c

error.o: ../error.c ../error.h
	$(CC) $(CFLAGS) -c ../error.c

clean:
	rm storage *.o
