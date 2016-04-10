/*
** ptime.c - Time/Date conversion routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#include <stdio.h>
#include <stdlib.h>

#include "ptime.h"


int
time_get(const char *str,
	 double *t)
{
    char c1, c2;
    int rc;


    c1 = c2 = 0;
    
    rc = sscanf(str, "%lf%c%c", t, &c1, &c2);
    if (rc >= 2)
    {
	switch (c1)
	{
	  case 'h':
	    *t *= 60.0*60.0;
	    break;
	    
	  case 'm':
	    switch (c2)
	    {
	      case 0:
		*t *= 60.0;
		break;
	      case 's':
		*t /= 1000.0;
		break;
	      default:
		return -1;
	    }
	    break;

	  case 's':
	    break;

	  case 'u':
	  case 'µ':
	    *t /= 1000000.0;
	    break;

	  default:
	    return -1;
	}
    }

    if (rc > 0)
	return 0;
    
    return -1;
}


char *
time_unit(double t)
{
    if (t > 60.0*60.0)
	return "h";
    if (t > 60)
	return "m";
    if (t < 0.0001)
	return "µs";
    if (t < 0.1)
	return "ms";
    return "s";
}

double
time_visual(double t)
{
    if (t > 60.0*60.0)
	return t/(60.0*60.0);
    if (t > 60.0)
	return t/60.0;
    if (t < 0.0001)
	return t*1000000.0;
    if (t < 0.1)
	return t*1000.0;
    return t;
}

