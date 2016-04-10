/*
** ptime.h - Time/Date conversion routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef PTIME_H
#define PTIME_H 1

extern int
time_get(const char *str,
	 double *t);

extern char *
time_unit(double t);

extern double
time_visual(double t);

#endif
