/*
** queue.h - Queue management routines
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

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>


typedef struct qentry
{
    void *p;

    struct qentry *next;
} QENTRY;


typedef struct queue
{
    pthread_mutex_t mtx;
    pthread_cond_t  cv;

    QENTRY *head;
    QENTRY *tail;
} QUEUE;


extern QUEUE *
queue_create(void);

extern int
queue_put(QUEUE *qp, void *p);

extern void *
queue_get(QUEUE *qp);

extern void
queue_destroy(QUEUE *qp);

#endif
