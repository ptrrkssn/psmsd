/*
** doorsms.h - Definition of SMS message sent via the 'doors' system
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef DOORSMS_H
#define DOORSMS_H 1

typedef struct doorsms
{
    char phone[64];
    char message[192];
} DOORSMS;

#endif
