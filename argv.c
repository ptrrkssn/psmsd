/*
 * argv.c - Argv-like generating routines
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
#include <string.h>
#include <ctype.h>

#include "argv.h"
#include "strmisc.h"
#include "buffer.h"


char *
argv_get(char **argv,
	 int idx)
{
    int i;


    if (!argv)
	return NULL;
    
    for (i = 0; i < idx && argv[i]; i++)
	;

    if (i < idx)
	return NULL;

    return argv[i];
}

char *
argv_getm(char **argv,
	  int start,
	  int stop)
{
    BUFFER buf;
    char *rp;
    int i;


    if (!argv)
	return NULL;

    for (i = 0; i < start && argv[i]; i++)
	;

    if (i < start)
	return NULL;
    
    buf_init(&buf);
    for (; argv[i] && (!stop || i <= stop); i++)
    {
	if (i != start)
	    buf_putc(&buf, ' ');
	buf_puts(&buf, argv[i]);
    }

    rp = s_dup(buf_getall(&buf));
    buf_clear(&buf);
    
    return rp;
}



char *
argv_strtok(const char *bp,
	    char *(*escape_handler)(const char *escape, void *xtra),
	    void *xtra)
{
    static const char *start = NULL;
    const char *rp;
    char *sp, *cp, *esc;
    int delim = 0, n;
    BUFFER buf;
    
    
    if (bp)
	start = bp;

    if (!start)
	return NULL;

    buf_init(&buf);
    rp = start;


    /* Skip leading whitespace */
    while (*rp && isspace(*rp))
	++rp;

    if (!*rp)
	return NULL;
    
    while (*rp && (delim || !isspace(*rp)))
    {
	switch (*rp)
	{
	  case '"':
	  case '\'':
	    if (!delim)
		delim = *rp;
	    else if (delim == *rp)
		delim = 0;
	    else
		buf_putc(&buf, *rp);
	    break;
	    
	  case '\\':
	    switch (*++rp)
	    {
	      case 'a':
		buf_putc(&buf, '\a');
		break;
		
	      case 'b':
		buf_putc(&buf, '\b');
		break;
		
	      case 'f':
		buf_putc(&buf, '\f');
		break;
		
	      case 'n':
		buf_putc(&buf, '\n');
		break;
		
	      case 'r':
		buf_putc(&buf, '\r');
		break;
		
	      case 't':
		buf_putc(&buf, '\t');
		break;
		
	      case 'v':
		buf_putc(&buf, '\v');
		break;
		
	      case '\0':
		break;
		
	      default:
		buf_putc(&buf, *rp);
	    }
	    break;

	  case '%':
	    if (escape_handler && delim != '\'')
	    {
		switch (*++rp)
		{
		  case '%':
		    buf_putc(&buf, '%');
		    break;
		    
		  case '\0':
		    break;
		    
		  default:
		    if (*rp == '{')
		    {
			++rp;
			for (n = 0; rp[n] && rp[n] != '}'; ++n)
			    ;
			esc = s_ndup(rp, n);
			rp += n;
		    }
		    else
			esc = s_ndup(rp, 1);
			
		    cp = escape_handler(esc, xtra);
		    if (cp)
		    {
			for (n = 0; cp[n]; ++n)
			    buf_putc(&buf, cp[n]);
			free(cp);
		    }
		    
		    free(esc);
		}
	    }
	    else
		buf_putc(&buf, *rp);
	    break;

	  default:
	    buf_putc(&buf, *rp);
	}

	if (*rp)
	    ++rp;
    }
    
    start = rp;

    sp = s_dup(buf_getall(&buf));
    buf_clear(&buf);

    return sp;
}


char **
argv_create(const char *command,
	    char *(*escape_handler)(const char *escape, void *xtra),
	    void *xtra)
{
    char **argv, *cp;
    int argc = 0, args = 32;

    
    argv = malloc(sizeof(char *) * args);
    if (!argv)
	return NULL;

    cp = argv_strtok(command, escape_handler, xtra);
    while (cp)
    {
	if (argc+1 >= args)
	{
	    argv = realloc(argv, sizeof(char *) * (args += 32));
	    if (!argv)
		return NULL;
	}
	
	argv[argc++] = cp;
	
	cp = argv_strtok(NULL, escape_handler, xtra);
    }

    argv[argc] = NULL;
    return argv;
}


void
argv_destroy(char **argv)
{
    int i;

    if (!argv)
	return;
    
    for (i = 0; argv[i]; i++)
	free(argv[i]);
    
    free(argv);
}


#ifdef DEBUG
char *my_esc_handler(const char *esc,
		     void *xtra)
{
    printf("Parsing ESC [%s]\n", esc ? esc : "<null>");

    if (strcmp(esc, "P") == 0 || strcmp(esc, "phone") == 0)
	return s_dup("+46705182786");
    
    if (strcmp(esc, "D") == 0 || strcmp(esc, "date") == 0)
	return s_dup("2006-11-24 13:37");

    return NULL;
}

int
main()
{
    char buf[1024];
    char **argv;
    int i;

    
    while (gets(buf))
    {
	argv = argv_create(buf, my_esc_handler, NULL);
	for (i = 0; argv[i]; i++)
	    printf("%3d : %s\n", i, argv[i]);
	argv_destroy(argv);
    }
}
#endif
