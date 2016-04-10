# Makefile for psmsd

SOLARIS_CFLAGS= -D_POSIX_PTHREAD_SEMANTICS -DHAVE_DOORS=1
LINUX_CFLAGS=

CC=gcc
CFLAGS=-O -g -Wall $(SOLARIS_CFLAGS)


BINS=psmsd psmsc

LOBJS=buffer.o users.o
DOBJS=psmsd.o gsm.o serial.o uucp.o cap.o queue.o argv.o spawn.o ptime.o strmisc.o $(LOBJS)
COBJS=psmsc.o $(LOBJS)


all:		$(BINS)


psmsd:		$(DOBJS)
		$(CC) -o psmsd $(DOBJS) -lsocket -lpthread -ldoor

psmsc:		$(COBJS)
		$(CC) -o psmsc $(COBJS) -ldoor


psmsd.o:	psmsd.c version.h serial.h queue.h gsm.h argv.h buffer.h users.h spawn.h ptime.h doorsms.h
psmsc.o:	psmsc.c version.h doorsms.h buffer.h users.h

gsm.o:		gsm.c gsm.h
serial.o:	serial.c serial.h
uucp.o:		uucp.c uucp.h
cap.o:		cap.c cap.h
queue.o:	queue.c queue.h
buffer.o:	buffer.c buffer.h
argv.o:		argv.c argv.h
spawn.o:	spawn.c spawn.h
users.o:	users.c users.h
ptime.o:	ptime.c ptime.h
strmisc.o:	strmisc.c strmisc.h


clean distclean:
	-rm -f  $(BINS) *.o *~ \#*


