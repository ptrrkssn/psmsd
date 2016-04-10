/*
** strmisc.c - Misc. string-handling routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
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

