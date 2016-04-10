/*
** serial.c - Serial I/O handling routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef SERIAL_H
#define SERIAL_H

#define SERIAL_E_OK                      0
#define SERIAL_E_UNIX_ERROR             -1
#define SERIAL_E_INVALID_SPEED          -2
#define SERIAL_E_INVALID_DEVICE         -3
#define SERIAL_E_LOCK_FAILED            -4
#define SERIAL_E_OPEN_FAILED            -5
#define SERIAL_E_GETATTR_FAILED         -6
#define SERIAL_E_SETSPEED_FAILED        -7
#define SERIAL_E_SETATTR_FAILED         -8

extern const char *
serial_strerror(int code);

extern void
serial_close(int fd);

extern int
serial_open(const char *devname,
	    int speed,
	    int timeout);

extern int
serial_read(int fd,
	    char *buf,
	    int bufsize,
	    int timeout);

extern int
serial_write(int fd,
	     const char *buf,
	     int bufsize,
	     int timeout);




#endif
