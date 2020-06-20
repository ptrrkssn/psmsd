/*
 * spawn.c - Spawn subprocesses
 *
 * Copyright (c) 2016-2020 Peter Eriksson <pen@lysator.liu.se>
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

