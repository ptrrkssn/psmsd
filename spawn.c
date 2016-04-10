/*
** spawn.c - Spawn subprocesses
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
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

