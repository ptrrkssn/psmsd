/*
** uucp.h - UUCP compatibility locking routines
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
