/*
** argc.h - Argv-like generating routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef ARGV_H
#define ARGV_H 1

extern char *
argv_get(char **argv,
	 int idx);

extern char *
argv_getm(char **argv,
	  int start,
	  int stop);

extern char *
argv_strtok(const char *bp,
	    char *(*escape_handler)(const char *escape, void *xtra),
	    void *xtra);

extern char **
argv_create(const char *command,
	    char *(*escape_handler)(const char *escape,
				    void *xtra),
	    void *xtra);

extern void
argv_destroy(char **argv);

#endif
