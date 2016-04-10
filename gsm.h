/*
** gsm.c - GSM character set handling routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef GSM_H
#define GSM_H 1

extern char *
latin1_to_gsm(const char *ls,
	      char *buf,
	      int bufsize);

extern char *
gsm_to_latin1(const char *gs,
	      char *buf,
	      int bufsize);

#endif
