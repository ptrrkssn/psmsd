/*
 * queue.c - Queue management routines
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

#include "queue.h"


QUEUE *
queue_create(void)
{
    QUEUE *qp;

    qp = malloc(sizeof(*qp));
    if (!qp)
	return NULL;

    pthread_mutex_init(&qp->mtx, NULL);
    pthread_cond_init(&qp->cv, NULL);

    qp->head = qp->tail = NULL;
    return qp;
}


int
queue_put(QUEUE *qp, void *p)
{
    QENTRY *qep;


    if (!qp)
	return -1;
    
    qep = malloc(sizeof(*qep));
    if (!qep)
	return -1;

    qep->p = p;
    qep->next = NULL;
    
    pthread_mutex_lock(&qp->mtx);
    
    if (qp->tail)
    {
	qp->tail->next = qep;
	qp->tail = qep;
    }
    else
	qp->head = qp->tail = qep;

    pthread_cond_signal(&qp->cv);
    pthread_mutex_unlock(&qp->mtx);
    return 0;
}


void *
queue_get(QUEUE *qp)
{
    void *p;
    QENTRY *qep;
    

    if (!qp)
	return NULL;
    
    pthread_mutex_lock(&qp->mtx);
    while ((qep = qp->head) == NULL)
	pthread_cond_wait(&qp->cv, &qp->mtx);

    p = qep->p;
    qp->head = qep->next;
    if (qp->tail == qep)
	qp->tail = NULL;
    pthread_mutex_unlock(&qp->mtx);
    free(qep);

    return p;
}


void
queue_destroy(QUEUE *qp)
{
    QENTRY *qc, *qn;

    for (qc = qp->head; qc; qc = qn)
    {
	qn = qc->next;
	free(qc);
    }
    free(qp);
}
