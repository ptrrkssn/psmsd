/*
** uucp.h - UUCP compatibility locking routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef UUCP_H
#define UUCP_H

#define UUCP_E_OK                      0
#define UUCP_E_LOCK_STAT_FAILED       -1
#define UUCP_E_LOCK_TMP_OPEN_FAILED   -2
#define UUCP_E_LOCK_TMP_WRITE_FAILED  -3
#define UUCP_E_LOCK_FILE_INVALID      -4
#define UUCP_E_LOCK_HELD              -5

extern int
uucp_lockdev(int dmajor,
	     int major,
	     int minor);

extern int
uucp_unlockdev(int dmajor,
	       int major,
	       int minor);

extern int
uucp_flock(const char *path);

extern int
uucp_funlock(const char *path);

extern int
uucp_unlock(int fd);

#endif
