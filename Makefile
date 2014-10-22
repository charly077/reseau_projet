#
#  Makefile pour le projet de réseau 2014
#

GCC = clang
AR = ar
ARFLAGS = -cvq
CFLAGS =  -g -c
LDFLAGS = --static -g

sender: sender.c struct.h paquet_creator.o
	$(GCC) sender.c -lz  -g -o sender paquet_creator.o 
	

paquet_creator.o : paquet_creator.h struct.h paquet_creator.c
	$(GCC) $(CFLAGS) paquet_creator.c


clean:
	rm ./sender *.o
