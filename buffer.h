/*
** buffer.h - Buffered I/O routines
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
