/*
 * cap.c - Capabilities file parsing stuff (termcap, /etc/remote etc...)
 *
 * Copyright (c) 2002-2020 Peter Eriksson <pen@lysator.liu.se>
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
#include <string.h>
#include <ctype.h>

#include "cap.h"
#include "strmisc.h"


int debug;
int verbose;


int
cap_getent(FILE *fp,
	   char *buf,
	   int bufsize)
{
    char *cp;
    int idx = 0, len;
    

    if (debug > 1)
	fprintf(stderr, "*** cap_getent(): Start\n");
    
    while (idx < bufsize && fgets(buf+idx, bufsize-idx, fp))
    {
	len = strlen(buf+idx);
	cp = buf+idx;
	
	if (debug > 1)
	    fprintf(stderr, "*** cap_getent(): Got %d chars: %s\n", len, cp);
    
	if (idx == 0)
	{
	    if (*cp == '#' || len == 0)
		continue;
	    if (!isspace((unsigned char) *cp) && !isalpha((unsigned char) *cp))
		break;
	}

	if (buf[idx+len-1] == '\n' || buf[idx+len-1] == '\r')
	{
	    --len;
	    buf[idx+len] = '\0';

	    if (len == 0)
		continue;
	    
	    if (buf[idx+len-1] == '\\')
		idx += len-1;
	    else
		return 1;
	}
	else
	    return 1;
    }

    return 0;
}


char *
cap_getstr(const char *path,
	   const char *entname,
	   const char *strname,
	   char *buf,
	   int bufsize)
{
    FILE *fp;
    char *cp, *tp;
    char *lcp, *ltp;
    static int rlev = 0;
    char *retval = NULL;
    char entbuf[2048];
    
    
    if (++rlev > 32)
	return NULL;
    
    fp = fopen(path, "r");
    if (fp == NULL)
	goto Exit;

    while (cap_getent(fp, entbuf, sizeof(entbuf)) > 0)
    {
	tp = strchr(entbuf, ':');
	*tp++ = '\0';

	cp = strtok_r(entbuf, "|", &lcp);
	while (cp)
	{
	    if (strcmp(cp, entname) == 0)
	    {
		/* Got a match */

		tp = strtok_r(tp, ":", &ltp);
		while (tp)
		{
		    if (strncmp(tp, strname, 2) == 0 && tp[2] == '=')
		    {
			/* Found the device */
			if (buf)
			{
			    if (strlen(tp+3) >= bufsize)
				return NULL;
			    
			    strcpy(buf, tp+3);
			    retval = buf;
			}
			else
			    retval = s_dup(tp+3);
			
			goto Exit;
		    }
		    else if (strncmp(tp, "tc=", 3) == 0)
		    {
			/* Got a continuation link */
			retval = cap_getstr(path, tp+3, strname,
					    buf, bufsize);
			if (retval)
			    goto Exit;
		    }
			
		    tp = strtok_r(NULL, ":", &ltp);
		}

		goto Exit;
	    }
	    
	    cp = strtok_r(NULL, "|", &lcp);
	}
    }
    
  Exit:
    --rlev;
    if (fp)
	fclose(fp);
    return retval;
}

int
cap_getint(const char *path,
	   const char *entname,
	   const char *strname,
	   int *val)
{
    FILE *fp;
    char *cp, *tp;
    char *lcp, *ltp;
    static int rlev = 0;
    int retval = -1;
    char entbuf[2048];
    
    
    if (++rlev > 32)
	return -1;
    
    fp = fopen(path, "r");
    if (fp == NULL)
	goto Exit;

    while (cap_getent(fp, entbuf, sizeof(entbuf)) > 0)
    {
	tp = strchr(entbuf, ':');
	*tp++ = '\0';

	cp = strtok_r(entbuf, "|", &lcp);
	while (cp)
	{
	    if (strcmp(cp, entname) == 0)
	    {
		/* Got a match */

		tp = strtok_r(tp, ":", &ltp);
		while (tp)
		{
		    if (strncmp(tp, strname, 2) == 0 && tp[2] == '#')
		    {
			/* Found the device */

			retval = sscanf(tp+3, "%d", val);
			goto Exit;
		    }
		    else if (strncmp(tp, "tc=", 3) == 0)
		    {
			/* Got a continuation link */
			retval = cap_getint(path, tp+3, strname, val);
			if (retval)
			    goto Exit;
		    }
			
		    tp = strtok_r(NULL, ":", &ltp);
		}

		goto Exit;
	    }
	    
	    cp = strtok_r(NULL, "|", &lcp);
	}
    }
    
  Exit:
    --rlev;
    if (fp)
	fclose(fp);
    return retval;
}

