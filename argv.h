/*
** argc.h - Argv-like generating routines
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
