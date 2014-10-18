#
#  Makefile pour le projet de réseau 2014
#

GCC = gcc
AR = ar
ARFLAGS = -cvq
CFLAGS = -Wall -std=c99 -g -c
LDFLAGS = --static -g

all: sender receiver


