/*
** buffer.h - Buffered I/O routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef BUFFER_H
#define BUFFER_H 1

typedef struct buffer
{
    char *buf;
    int len;
    int size;
} BUFFER;


extern void
buf_init(BUFFER *bp);

extern void
buf_clear(BUFFER *bp);

extern BUFFER *
buf_new(void);

extern void
buf_free(BUFFER *bp);

extern int
buf_putc(BUFFER *bp,
	 char c);

extern int
buf_puts(BUFFER *bp,
	 const char *s);


extern char *
buf_getall(BUFFER *bp);

extern int
buf_save(BUFFER *bp,
	 FILE *fp);

extern int
buf_load(BUFFER *bp,
	 FILE *fp);
#endif
