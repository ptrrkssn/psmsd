/*
** buffer.c - Buffered I/O routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "buffer.h"


void
buf_init(BUFFER *bp)
{
    bp->buf = NULL;
    bp->size = 0;
    bp->len = 0;
}

void
buf_clear(BUFFER *bp)
{
    if (bp->buf)
	free(bp->buf);
    buf_init(bp);
}

BUFFER *
buf_new(void)
{
    BUFFER *bp;

    bp = malloc(sizeof(*bp));
    if (!bp)
	return NULL;

    buf_init(bp);
    return bp;
}

void
buf_free(BUFFER *bp)
{
    buf_clear(bp);
    free(bp);
}


int
buf_putc(BUFFER *bp,
	 char c)
{
    if (bp->len >= bp->size)
    {
	if (!bp->buf)
	    bp->buf = malloc((bp->size = 256)+1);
	else
	    bp->buf = realloc(bp->buf, (bp->size += 256)+1);
	if (!bp->buf)
	    return -1;
	
	memset(bp->buf+bp->len, 0, bp->size+1-bp->len);
    }

    bp->buf[bp->len++] = c;
    return bp->len;
}



int
buf_puts(BUFFER *bp,
	 const char *s)
{
    int rc = 0;

    
    while (rc >= 0 && *s)
	rc = buf_putc(bp, *s++);

    return rc;
}





char *
buf_getall(BUFFER *bp)
{
    return bp->buf ? bp->buf : "";
}


int
buf_save(BUFFER *bp,
	 FILE *fp)
{
    if (!bp->buf)
	return 0;
    
    return fwrite(bp->buf, 1, bp->len, fp);
}


int
buf_load(BUFFER *bp,
	 FILE *fp)
{
    int rc, nsize;
    struct stat sb;


    if (fstat(fileno(fp), &sb) < 0 || !S_ISREG(sb.st_mode))
    {
	while ((rc = getc(fp)) != EOF)
	    buf_putc(bp, rc);

	return bp->len;
    }

    nsize = sb.st_size + bp->len;
    if (nsize >= bp->size)
    {
	char *nbuf = realloc(bp->buf, nsize+1);
	
	if (!nbuf)
	    return -1;

	bp->buf = nbuf;
	bp->size = nsize;
	
	memset(bp->buf+bp->len, 0, bp->size+1-bp->len);
    }
    

    rc = fread(bp->buf+bp->len, 1, sb.st_size, fp);
    if (rc > 0)
	bp->len += rc;
    
    if (rc < 0)
	return -1;

    return bp->len;
}
