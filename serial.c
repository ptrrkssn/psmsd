/*
** serial.c - Serial I/O handling routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
**
** This file is part of psmsd.
**
** psmsd is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** psmsd is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with psmsd.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "serial.h"
#include "uucp.h"
#include "cap.h"

int debug;
int verbose;


static struct
{
    int speed;
    int code;
} speedtab[] =
{
    { 0, B0 },
    { 50, B50 },
    { 75, B75 },
    { 110, B110 },
    { 134, B134 },
    { 150, B150 },
    { 200, B200 },
    { 300, B300 },
    { 600, B600 },
    { 1200, B1200 },
    { 2400, B2400 },
    { 4800, B4800 },
    { 9600, B9600 },
    { 19200, B19200 },
    { 38400, B38400 },
    { 57600, B57600 },
    { 76800, B76800 },
    { 115200, B115200 },
    { 153600, B153600 },
    { 230400, B230400 },
    { 307200, B307200 },
    { 460800, B460800 },
    { -1, -1 }
};

static int
speed2code(int speed)
{
    int i;

    i = 0;

    while (speedtab[i].speed != -1)
	if (speedtab[i].speed == speed)
	    return speedtab[i].code;
	else
	    ++i;
	

    return SERIAL_E_INVALID_SPEED;
}


void
serial_close(int fd)
{
    struct stat sb;

    
    if (fstat(fd, &sb) == 0 && S_ISSOCK(sb.st_mode))
	(void) close(fd);
    else
    {
	(void) uucp_unlock(fd);
	(void) close(fd);
    }
}


int
serial_read(int fd,
	    char *buf,
	    int bufsize,
	    int timeout)
{
    struct pollfd pfd;
    int code;
    int avail;
    

    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    code = poll(&pfd, 1, timeout);
    if (code == 0)
    {
	errno = EINTR;
	return SERIAL_E_UNIX_ERROR;
    }

    if (code < 0)
	return code;

    if (ioctl(fd, FIONREAD, &avail) < 0)
	return SERIAL_E_UNIX_ERROR;

    if (avail == 0)
	return 0;

    if (avail > bufsize)
	avail = bufsize;
    
    code = read(fd, buf, avail);

    if (debug > 2)
    {
	int i;

	for (i = 0; i < code; i++)
	    fprintf(stderr, "serial_read: read char 0x%02x\n", buf[i]);
    }

    return code;
}


int
serial_write(int fd,
	     const char *buf,
	     int bufsize,
	     int timeout)
{
    struct pollfd pfd;
    int i, code;


    pfd.fd = fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;

    for (i = 0; i < bufsize; i++)
    {
	code = poll(&pfd, 1, timeout);
	if (code == 0)
	{
	    errno = EINTR;
	    return i ? i : SERIAL_E_UNIX_ERROR;
	}
	
	if (code < 0)
	    return i ? i : code;
	
	code = write(fd, buf+i, 1);

	if (debug > 2)
	    fprintf(stderr, "serial_write: wrote char 0x%02x\n", buf[i]);
	
	if (code <= 0)
	    return i ? i : code;
    }

    return bufsize;
}


const char *
serial_strerror(int code)
{
    static char buf[1024];

    
    switch (code)
    {
      case SERIAL_E_OK:
	return "No error";

      case SERIAL_E_UNIX_ERROR:
	snprintf(buf, sizeof(buf), "Error: %s", strerror(errno));
	return buf;
	    
      case SERIAL_E_INVALID_SPEED:
	return "Invalid speed";
	
      case SERIAL_E_INVALID_DEVICE:
	return "Invalid tty device";
	
      case SERIAL_E_LOCK_FAILED:
	return "Failed to get device lock";
	
      case SERIAL_E_OPEN_FAILED:
	snprintf(buf, sizeof(buf), "Device open failed: %s", strerror(errno));
	return buf;
	
      case SERIAL_E_GETATTR_FAILED:
	snprintf(buf, sizeof(buf), "Failed reading device attributes: %s", strerror(errno));
	return buf;
	
      case SERIAL_E_SETSPEED_FAILED:
	snprintf(buf, sizeof(buf), "Failed setting device speed: %s", strerror(errno));
	return buf;
	
      case SERIAL_E_SETATTR_FAILED:
	snprintf(buf, sizeof(buf), "Failed changing device attributes: %s", strerror(errno));
	return buf;

      default:
	snprintf(buf, sizeof(buf), "Unknown error (errno=%d \"%s\")",
		errno, strerror(errno));
	return buf;
    }
}


int
serial_open(const char *devname,
	    int speed,
	    int timeout) /* should be device open timeout - not used */
{
    int fd, scode;
    struct termios tiob;
    char devbuf[1024];
    struct stat sb;
    

    if (debug)
	fprintf(stderr, "*** serial_open(\"%s\", %d, %d): Start\n",
		devname, speed, timeout);
    
    if (*devname != '/')
	devname = cap_getstr("/etc/remote", devname, "dv",
			     devbuf, sizeof(devbuf));

    if (devname == NULL)
	return SERIAL_E_INVALID_DEVICE;

    if (*devname == '/' && stat(devname, &sb) == 0 && S_ISSOCK(sb.st_mode))
    {
	/* UNIX Socket device */
	struct sockaddr_un sb;
	int len;
	

	len = strlen(devname);
	if (len > sizeof(sb.sun_path))
	    return SERIAL_E_INVALID_DEVICE;
	
	memset(&sb, 0, sizeof(sb));
	sb.sun_family = AF_UNIX;
	memcpy(sb.sun_path, devname, len);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	    return SERIAL_E_UNIX_ERROR;

	/* Set socket to non-blocking and poll (for timeout) */
	
	if (connect(fd, (struct sockaddr *) &sb, sizeof(sb.sun_family)+len) < 0)
	{
	    (void) close(fd);
	    return SERIAL_E_UNIX_ERROR;
	}
    }
    else
    {
	if (speed == 0)
	    cap_getint("/etc/remote", devname, "br", &speed);
	
	if (speed <= 0)
	    return SERIAL_E_INVALID_SPEED;
	
	scode = speed2code(speed);
	if (scode < 0)
	    return SERIAL_E_INVALID_SPEED;

#if 0
	if (uucp_flock(devname) != UUCP_E_OK)
	    return SERIAL_E_LOCK_FAILED;
#endif
	
	fd = open(devname, O_RDWR|O_NOCTTY, 0);
	if (fd < 0)
	{
	    (void) uucp_funlock(devname);
	    return SERIAL_E_OPEN_FAILED;
	}
	
	if (tcgetattr(fd, &tiob) < 0)
	{
	    close(fd);
	    (void) uucp_unlock(fd);
	    return SERIAL_E_GETATTR_FAILED;
	}
	
	if (cfsetispeed(&tiob, scode) < 0 || 
	    cfsetospeed(&tiob, scode) < 0)
	{
	    (void) close(fd);
	    (void) uucp_unlock(fd);
	    return SERIAL_E_SETSPEED_FAILED;
	}
	
	tiob.c_cflag &= ~CSIZE;   /* 8 bits */
	tiob.c_cflag |= CS8;
	tiob.c_cflag |= CREAD;
	tiob.c_cflag &= ~CSTOPB;   /* One stop bit */
	tiob.c_cflag &= ~PARENB;   /* No parity */
	tiob.c_cflag &= ~CLOCAL;   /* Modem control */
	tiob.c_cflag |= CRTSCTS;   /* Flow control - outgoing*/
	tiob.c_cflag |= CRTSXOFF;  /* Flow control - incoming */

	tiob.c_iflag &= ~BRKINT;
	tiob.c_iflag |= ISTRIP;
	tiob.c_iflag &= ~IXON;
	tiob.c_iflag &= ~ICRNL;
	tiob.c_iflag &= ~IMAXBEL;

	tiob.c_oflag &= ~OPOST;
	tiob.c_oflag &= ~ONLCR;
	
	tiob.c_lflag &= ~ICANON;
	tiob.c_lflag &= ~ISIG;
	tiob.c_lflag &= ~ECHO;
	
	tiob.c_cc[VMIN] = 1;
	tiob.c_cc[VTIME] = 0; /* timeout */
	
	if (tcsetattr(fd, TCSANOW, &tiob) < 0)
	{
	    (void) close(fd);
	    (void) uucp_unlock(fd);
	    return SERIAL_E_SETATTR_FAILED;
	}

	tcflush(fd, TCIOFLUSH);
    }
    
    return fd;
}




