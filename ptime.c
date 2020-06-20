/*
 * ptime.c - Time/Date conversion routines
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

#include "ptime.h"


int
time_get(const char *str,
	 double *t)
{
    unsigned char c1, c2;
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
	  case 0xB5: /* Latin-1 "micro" */
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
	return "us";
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

