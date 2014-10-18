#
#  Makefile pour le projet de réseau 2014
#

GCC = gcc
AR = ar
ARFLAGS = -cvq
CFLAGS = -Wall -std=c99 -g -c
LDFLAGS = --static -g

sender: sender.c
	$(GCC) sender.c -Wall -std=c99 -g -o sender
	./sender
clean:
	rm sender
