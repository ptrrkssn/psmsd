# Makefile for psmsd

SOLARIS_CFLAGS=-D_POSIX_PTHREAD_SEMANTICS -DHAVE_DOORS=1 -DHAVE_LOADAVG=1 -DHAVE_CLOSEFROM=1
SOLARIS_LIBS=-lsocket -lpthread -ldoor

LINUX_CFLAGS=
LINUX_LIBS=-lpthread

LIBS=$(LINUX_LIBS)

CC=gcc -pthread
CFLAGS=-g -Wall $(LINUX_CFLAGS)


BINS=psmsd psmsc

LOBJS=buffer.o users.o strmisc.o
DOBJS=psmsd.o gsm.o serial.o uucp.o cap.o queue.o argv.o spawn.o ptime.o $(LOBJS)
COBJS=psmsc.o $(LOBJS)


all:		$(BINS)


psmsd:		$(DOBJS)
		$(CC) -o psmsd $(DOBJS) -lpthread $(LIBS)

psmsc:		$(COBJS)
		$(CC) -o psmsc $(COBJS) $(LIBS)


psmsd.o:	psmsd.c common.h serial.h queue.h gsm.h argv.h buffer.h users.h spawn.h ptime.h
psmsc.o:	psmsc.c common.h buffer.h users.h

gsm.o:		gsm.c gsm.h
serial.o:	serial.c serial.h
uucp.o:		uucp.c uucp.h
cap.o:		cap.c cap.h strmisc.h
queue.o:	queue.c queue.h
buffer.o:	buffer.c buffer.h
argv.o:		argv.c argv.h buffer.h strmisc.h
spawn.o:	spawn.c spawn.h
users.o:	users.c users.h strmisc.h
ptime.o:	ptime.c ptime.h
strmisc.o:	strmisc.c strmisc.h


clean distclean:
	-rm -f  $(BINS) *.o *~ \#* */*~ */#*

version:
	@VERSION="`sed -e 's/^#define *VERSION *\"\(.*\)\"$$/\1/' <common.h`" && echo $$VERSION


push:	clean
	git add -A && git commit -a && git push
