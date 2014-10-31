#
#  Makefile pour le projet de réseau 2014
#

GCC = gcc
AR = ar
ARFLAGS = -cvq
CFLAGS =  -g -c 
LDFLAGS = --static -g

all: sender receiver

sender: src/sender.c src/struct.h paquet_creator.o selective_repeat.o
	$(GCC) src/sender.c -lz  -g -o sender paquet_creator.o selective_repeat.o 	

receiver: src/receiver_new.c paquet_creator.o
	$(GCC) src/receiver.c -lz -o receiver paquet_creator.o

paquet_creator.o : src/paquet_creator.h src/struct.h src/paquet_creator.c
	$(GCC) $(CFLAGS)  src/paquet_creator.c -lz

selective_repeat.o : src/selective_repeat.h src/selective_repeat.c
	$(GCC) $(CFLAGS) src/selective_repeat.c -lz

clean:
	rm ./sender *.o ./receiver
