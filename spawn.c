/*
** spawn.c - Spawn subprocesses
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "spawn.h"



int
spawn(const char *path,
      char * const *argv,
      int uid, int gid,
      FILE *fpin,
      FILE *fpout,
      FILE *fperr)
{
    int fdin, fdout, fderr, pid;

    
    if (!path || !argv)
	return -1;

    if (fpin)
	fdin = fileno(fpin);
    else
	fdin = open("/dev/null", O_RDONLY);
    
    if (fpout)
	fdout = fileno(fpout);
    else
	fdout = open("/dev/null", O_WRONLY);
    
    if (fperr)
	fderr = fileno(fperr);
    else
	fderr = open("/dev/null", O_WRONLY);

    if (fdin < 0 || fdout < 0 || fderr < 0)
	goto Fail;
    
    pid = fork();
    if (pid < 0)
	goto Fail;

    if (pid == 0)
    {
	/* In child */
	dup2(fdin, 0);
	dup2(fdout, 1);
	dup2(fderr, 2);
	closefrom(3);

	setgroups(0, NULL);
	setgid(gid);
	setuid(uid);

	execv(path, argv);
	_exit(1);
    }

    if (!fpin)
	close(fdin);
    if (!fpout)
	close(fdout);
    if (!fperr)
	close(fderr);
    return pid;
    
  Fail:
    if (!fpin)
	close(fdin);
    if (!fpout)
	close(fdout);
    if (!fperr)
	close(fderr);
    return -1;
}

