/*
** uucp.c - UUCP compatibility locking routines
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mkdev.h>

#include "uucp.h"

#define USER_UUCP 5
#define GROUP_UUCP 5

int debug;
int verbose;


int
uucp_lockdev(int dmajor,
	     int major,
	     int minor)
{
    int fd = -1, pid, i;
    char buf1[1024], buf2[1024];
    char pidbuf[12];
    
    
    pid = getpid();
    
    snprintf(buf1, sizeof(buf1), "/var/spool/locks/TMP.%d", pid);
    fd = open(buf1, O_WRONLY|O_EXCL|O_CREAT, 0444);
    if (fd < 0)
	return UUCP_E_LOCK_TMP_OPEN_FAILED;

    snprintf(pidbuf, sizeof(pidbuf), "%10d\n", pid);
    if (write(fd, pidbuf, 11) != 11)
    {
	(void) unlink(buf1);
	return UUCP_E_LOCK_TMP_WRITE_FAILED;
    }

    (void) fchown(fd, USER_UUCP, GROUP_UUCP);
    (void) close(fd);

    snprintf(buf2, sizeof(buf2), "/var/spool/locks/LK.%03d.%03d.%03d", dmajor, major, minor);
    for (i = 0; i < 10; i++)
    {
	if (link(buf1, buf2) == 0)
	    break;
	
	if (errno == EEXIST)
	{
	    fd = open(buf2, O_RDONLY);
	    if (fd < 0)
		continue; /* Failed - perhaps the lock just got released */
	    
	    if (read(fd, pidbuf, 11) != 11)
	    {
		(void) close(fd);
		return UUCP_E_LOCK_FILE_INVALID;
	    }
	    
	    (void) close(fd);
	    
	    if (sscanf(pidbuf, "%d", &pid) == 1 &&
		kill(pid, 0) < 0 && errno == ESRCH)
	    {
		/* State lock file - let's try to remove it and retry */
		if (unlink(buf2) == 0)
		    continue;
	    }
	    
	    if (verbose)
		fprintf(stderr,
			"*** Lock file in use - Sleeping 1 second...\n");
	    
	    sleep(1);
	}
    }

    if (i == 10)
    {
	unlink(buf1);
	return UUCP_E_LOCK_HELD;
    }

    unlink(buf1);
    return UUCP_E_OK;
}

int
uucp_unlockdev(int dmajor,
	       int major,
	       int minor)
{
    char buf[1024];
    

    snprintf(buf, sizeof(buf), "/var/spool/locks/LK.%03d.%03d.%03d", dmajor, major, minor);

    (void) unlink(buf);

    return UUCP_E_OK;
}
     
int
uucp_flock(const char *path)
{
    struct stat sb;

    
    if (stat(path, &sb) < 0)
	return UUCP_E_LOCK_STAT_FAILED;

    return uucp_lockdev(major(sb.st_dev), major(sb.st_rdev), minor(sb.st_rdev));
}

int
uucp_funlock(const char *path)
{
    struct stat sb;
    
    if (stat(path, &sb) < 0)
	return UUCP_E_LOCK_STAT_FAILED;

    return uucp_unlockdev(major(sb.st_dev), major(sb.st_rdev), minor(sb.st_rdev));
}

int
uucp_unlock(int fd)
{
    struct stat sb;
    
    if (fstat(fd, &sb) < 0)
	return UUCP_E_LOCK_STAT_FAILED;

    return uucp_unlockdev(major(sb.st_dev), major(sb.st_rdev), minor(sb.st_rdev));
}

