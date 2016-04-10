/*
** psmsc - CLI utility to send SMS message via psmsd daemon
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
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <door.h>
#include <sysexits.h>
#include <syslog.h>

#if HAVE_DOORS
#include <door.h>
#endif

#include "common.h"
#include "buffer.h"
#include "strmisc.h"


int debug = 0;

char *fifo_path = FIFO_PATH;

#if HAVE_DOORS
char *door_path = DOOR_PATH;
int door_fd = -1;
#endif

int mailmode = 0;


void
usage(FILE *fp,
      char *argv0)
{
    fprintf(fp, "Usage: %s [<options>] [<user-1> [.. <user-N>]]\n",
	    argv0);
    fprintf(fp, "Options:\n");
    fprintf(fp, "  -h                Display this information\n");
    fprintf(fp, "  -V                Print version and exit\n");
    fprintf(fp, "  -m                Mail mode\n");
    fprintf(fp, "  -d                Debug mode\n");
#if HAVE_DOORS
    fprintf(fp, "  -D<path>          Path to door file (default: %s)\n", door_path);
#endif
    fprintf(fp, "  -F<path>          Path to fifo file (default: %s)\n", fifo_path);
}



#if HAVE_DOORS
int
door_send_sms(const char *to,
	      const char *msg)
{
    struct door_arg da;
    DOORSMS dsp;
    int rc;
    char rbuf[1024];
    

    if (door_fd < 0)
	return -1;
    
    memset(&dsp, 0, sizeof(dsp));
    strncpy(dsp.phone, to, sizeof(dsp.phone)-1);
    strncpy(dsp.message, msg, sizeof(dsp.message)-1);
    
    memset(&da, 0, sizeof(da));
    da.data_ptr = (char *) &dsp;
    da.data_size = sizeof(dsp);
    da.desc_ptr = NULL;
    da.desc_num = 0;
    da.rbuf = rbuf;
    da.rsize = sizeof(rbuf);
    
    while ((rc = door_call(door_fd, &da)) < 0 && errno == EINTR)
	;
    
    if (rc < 0)
	return -2;
    
    if (sizeof(rc) != da.data_size)
	return -3;
    
    return * (int *) da.data_ptr;
}
#endif

int
fifo_send_sms(const char *to,
	      const char *msg)
{
    FILE *fp;

    fp = fopen(FIFO_PATH, "w");
    if (!fp)
	return -1;

    fprintf(fp, "%s\t%s\n", to, msg);
    return fclose(fp);
}

int
send_sms(const char *to,
	 const char *msg)
{
#if HAVE_DOORS
    if (door_fd > 0)
	return door_send_sms(to, msg);
#endif

    return fifo_send_sms(to, msg);
}

void
p_header(void)
{
    printf("[psmsc, version %s - Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>]\n", VERSION);
}

int
main(int argc,
     char *argv[])
{
    int i, rc, nerr = 0, e;
    char *to, *msg, *cp;
    BUFFER buf;
#if HAVE_DOORS
    struct door_info dib;
#endif
    
    for (i = 1; i < argc && argv[i][0] == '-'; i++)
	switch (argv[i][1])
	{
	  case 'V':
	    p_header();
	    exit(0);
	    
	  case 'm':
	    ++mailmode;
	    break;
	    
	  case 'd':
	    ++debug;
	    break;

#if HAVE_DOORS
	  case 'D':
	    door_path = strdup(argv[i]+2);
	    break;
#endif
	    
	  case 'F':
	    fifo_path = strdup(argv[i]+2);
	    break;
	    
	  case '-':
	    goto EndOptions;

	  case 'h':
	    usage(stdout, argv[0]);
	    exit(0);
	    
	  default:
	    fprintf(stderr, "%s: unknown switch: %s\n", argv[0], argv[i]);
	    exit(1);
	}

  EndOptions:
    if (i >= argc)
    {
	fprintf(stderr, "%s: Missing recipient of message\n", argv[0]);
	exit(1);
    }

#if HAVE_DOORS
    door_fd = open(door_path, O_RDONLY);
    if (door_fd < 0)
    {
	e = errno;
	if (errno != ENOENT) {
	    syslog(LOG_ERR, "%s: open: %s\n", door_path, strerror(e));
	    fprintf(stderr, "%s: open(%s): %s\n", argv[0], door_path, strerror(e));
	    exit(1);
	}
    }

    if (door_fd > 0) {
	if (door_info(door_fd, &dib) < 0)
	{
	    e = errno;
	    syslog(LOG_ERR, "%s: door_info: %s\n", door_path, strerror(e));
	    fprintf(stderr, "%s: %s: door_info: %s\n", argv[0], door_path, strerror(e));
	    exit(1);
	}
	
	if (debug)
	{
	    printf("door_info() -> server pid = %ld, uniquifier = %ld",
		   (long) dib.di_target,
		   (long) dib.di_uniquifier);
	    
	    if (dib.di_attributes & DOOR_LOCAL)
		printf(", LOCAL");
	    if (dib.di_attributes & DOOR_PRIVATE)
		printf(", PRIVATE");
	    if (dib.di_attributes & DOOR_REVOKED)
		printf(", REVOKED");
	    if (dib.di_attributes & DOOR_UNREF)
		printf(", UNREF");
	    putchar('\n');
	}
	
	if (dib.di_attributes & DOOR_REVOKED)
	{
	    syslog(LOG_ERR, "%s: door revoked\n", door_path);
	    fprintf(stderr, "%s: door revoked\n", argv[0]);
	    exit(1);
	}
    }
#endif

    if (isatty(fileno(stdin)))
	puts("Enter message:");
    
    buf_init(&buf);
    buf_load(&buf, stdin);

    msg = buf_getall(&buf);
    if (mailmode)
    {
	while (*msg && !((msg[0] == '\n' && msg[1] == '\n') ||
			 (msg[0] == '\r' && msg[1] == '\n' &&
			  msg[2] == '\r' && msg[3] == '\n')))
	    ++msg;
	
	switch (*msg)
	{
	  case '\r':
	    msg += 4;
	    break;
	    
	  case '\n':
	    msg += 2;
	}
    }

    while (i < argc)
    {
	to = s_dup(argv[i]);
	if (!to) {
	    fprintf(stderr, "%s: %s: s_dup: %s\n", argv[0], argv[i], strerror(errno));
	    exit(1);
	}
	    
	cp = strchr(to, '@');
	if (cp)
	    *cp = '\0';
	
	rc = send_sms(to, msg);
	if (rc != 0)
	{
	    ++nerr;
	    e = errno;
	    syslog(LOG_ERR, "%s: send failed (door_path=%s, rc=%d): %s", argv[i], door_path, strerror(e));
	    fprintf(stderr, "%s: %s: send failed (rc=%d): %s\n", argv[0], argv[i], rc, strerror(e));
	}
	
	++i;
    }
    
    exit(nerr);
}
