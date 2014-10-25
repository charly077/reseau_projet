#
#  Makefile pour le projet de réseau 2014
#

GCC = clang
AR = ar
ARFLAGS = -cvq
CFLAGS =  -g -c 
LDFLAGS = --static -g

all: sender receiver

sender: sender.c struct.h paquet_creator.o selective_repeat.o
	$(GCC) sender.c -lz  -g -o sender paquet_creator.o selective_repeat.o 	
receiver : receiver_new.c paquet_creator.o
	$(GCC) receiver_new.c -lz -o receiver paquet_creator.o

paquet_creator.o : paquet_creator.h struct.h paquet_creator.c
	$(GCC) $(CFLAGS)  paquet_creator.c -lz

selective_repeat.o : selective_repeat.h selective_repeat.c
	$(GCC) $(CFLAGS) selective_repeat.c -lz

clean:
	rm ./sender *.o ./receiver
