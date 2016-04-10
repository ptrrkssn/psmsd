/*
** cap.c - Capabilities file parsing stuff (termcap, /etc/remote etc...)
**
** Copyright (c) 2002 Peter Eriksson <pen@lysator.liu.se>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef CAP_H
#define CAP_H

extern int
cap_getent(FILE *fp,
	   char *buf,
	   int bufsize);

extern char *
cap_getstr(const char *path,
	   const char *entname,
	   const char *strname,
	   char *buf,
	   int bufsize);

extern int
cap_getint(const char *path,
	   const char *entname,
	   const char *strname,
	   int *val);

#endif
