/*
** strmisc.c - Misc. string-handling routines
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

#include <stdlib.h>
#include <string.h>

#include "strmisc.h"

char *
s_ndup(const char *s,
       unsigned int len)
{
    char *rs;

    
    if (!s)
	return NULL;
    
    rs = malloc(len+1);
    if (!rs)
	return NULL;
    
    strncpy(rs, s, len);
    rs[len] = '\0';
    
    return rs;
}

