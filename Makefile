# Makefile for psmsd

TAR=/bin/tar
CC=gcc
CFLAGS=-O -g -Wall -D_POSIX_PTHREAD_SEMANTICS

UPLOAD_TARGET=pen@ftp.lysator.liu.se:/lysator/ftp/pub/unix/psmsd


BINS=psmsd smsclient
OBJS=gsm.o serial.o uucp.o cap.o queue.o buffer.o argv.o spawn.o users.o version.o ptime.o strmisc.o

all:		$(BINS)

psmsd:		psmsd.o $(OBJS)
		$(CC) -o psmsd psmsd.o $(OBJS) -lsocket -lpthread -ldoor

smsclient:	smsclient.o buffer.o users.o
		$(CC) -o smsclient smsclient.o buffer.o users.o -ldoor -lpthread

queue.o:	queue.c queue.h
serial.o:	serial.c serial.h
monitor.o:	monitor.c queue.h serial.h
sendsms.o:	sendsms.c serial.h
buffer.o:	buffer.c buffer.h
argv.o:		argv.c argv.h
users.o:	users.c users.h
spawn.o:	spawn.c spawn.h
ptime.o:	ptime.c ptime.h
strmisc.o:	strmisc.c strmisc.h

clean distclean:
	-rm -f  $(BINS) *.o *~ \#*

version:
	(PACKNAME=`basename \`pwd\`` ; echo 'char version[] = "'`echo $$PACKNAME | cut -d- -f2`'";' >version.c)

#

dist:	version distclean
	(PACKNAME=`basename \`pwd\`` ; cd .. ; $(TAR) cf - $$PACKNAME | gzip -9 >$$PACKNAME.tar.gz)

sign:
	(PACKNAME=`basename \`pwd\`` ; cd .. ; $(PGPSIGN) -ab $$PACKNAME.tar.gz -o $$PACKNAME.sig && chmod go+r $$PACKNAME.sig)

md5:
	(PACKNAME=`basename \`pwd\`` ; cd .. ; $(MD5SUM)  $$PACKNAME.tar.gz >$$PACKNAME.md5 && chmod go+r $$PACKNAME.md5)

upload:	dist md5 sign
	(PACKNAME=`basename \`pwd\`` ; cd .. ; $(SCP) $$PACKNAME.tar.gz $$PACKNAME.sig $$PACKNAME.md5 $(UPLOAD_TARGET))
