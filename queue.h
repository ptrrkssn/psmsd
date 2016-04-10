/*
** queue.h - Queue management routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
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
