/*
** users.c - User database handling routines
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

#include "users.h"
#include "strmisc.h"

extern int debug;

static pthread_mutex_t mtx;
static USER *uv = NULL;
static int us = 0;
static int uc = 0;

static int autologout_time = 0;
static pthread_t autologout_tid;


static void
sigusr1_handler(int sig)
{
}


static void *
autologout_thread(void *misc)
{
    int i, len;
    time_t now, next;
    void (*logout_handler)(USER *up);


    logout_handler = (void (*)(USER *up)) misc;

    if (debug)
	fprintf(stderr, "AUTOLOGOUT_THREAD: Start\n");

    next = 0;

    signal(SIGUSR1, sigusr1_handler);
    
    while (autologout_time > 0)
    {
	if (debug > 1)
	    fprintf(stderr, "AUTOLOGOUT_THREAD: Checking\n");
    
	time(&now);
	
	pthread_mutex_lock(&mtx);
	for (i = 0; i < uc; i++)
	{
	    if (uv[i].expires)
	    {
		if (now >= uv[i].expires)
		{
		    if (uv[i].cphone)
		    {
			if (debug)
			    fprintf(stderr, "AUTOLOGOUT_THREAD: Terminating %s\n",
				    uv[i].cphone);
			
			logout_handler(&uv[i]);
			free(uv[i].cphone);
			uv[i].cphone = NULL;
			uv[i].expires = 0;
		    }
		}
		else
		{
		    if (!next || uv[i].expires < next)
			next = uv[i].expires;
		}
	    }
	}
	pthread_mutex_unlock(&mtx);

	len = next-now;
	if (len <= 0)
	    len = autologout_time;
	
	if (debug > 1)
	    fprintf(stderr, "AUTOLOGOUT_THREAD: Sleeping %d seconds\n",
		    len);
	
	sleep(len);
    }
    
    if (debug)
	fprintf(stderr, "AUTOLOGOUT_THREAD: Stop\n");
    
    return NULL;
}


int
users_load(const char *path)
{
    FILE *fp;
    char buf[1024], *name, *pass, *phone, *acl;


    if (debug)
	fprintf(stderr, "USERS_LOAD: Start\n");
    
    pthread_mutex_lock(&mtx);
    if (uv)
	free(uv);

    uc = 0;
    uv = malloc(sizeof(*uv)*(us = 128));
    if (!uv)
    {
	pthread_mutex_unlock(&mtx);
	return -1;
    }
    
    fp = fopen(path, "r");
    if (!fp)
    {
	pthread_mutex_unlock(&mtx);
	return -1;
    }
    
    while (fgets(buf, sizeof(buf), fp))
    {
	name = strtok(buf, " \t\r\n");
	if (!name || *name == '#')
	    continue;

	phone = strtok(NULL, " \t\n\r");
	if (!phone)
	    continue;
	
	pass = strtok(NULL, " \t\n\r");
	if (!pass)
	    continue;
	
	acl = strtok(NULL, "\n\r");

	if (acl)
	    while (isspace(*acl))
	    ++acl;
	
	if (uc == us)
	    uv = realloc(uv, sizeof(*uv)*(us += 128));

	if (debug > 1)
	    fprintf(stderr,
		    "USERS_LOAD: Name=%s, Phone=%s, Pass=%s, Acl=%s\n",
		    name, phone, pass, acl ? acl : "<none>");
	
	uv[uc].name = s_dup(name);
	uv[uc].pphone = s_dup(phone);
	uv[uc].pass = s_dup(pass);
	uv[uc].acl = s_dup(acl);
	
	uv[uc].cphone = NULL;
	uv[uc].expires = 0;
	
	++uc;
    }

    fclose(fp);

    pthread_mutex_unlock(&mtx);
    
    if (debug)
	fprintf(stderr, "USERS_LOAD: Stop\n");
    
    return uc;
}

int
users_login(UCRED *ucp,
	    const char *name,
	    const char *pass)
{
    int i, j, nm = 0;
    time_t now;
    
    
    if (debug)
	fprintf(stderr, "USERS_LOGIN: Name=%s, Pass=%s\n",
		(name ? name : "<null>"),
		(pass ? pass : "<null>"));
    
    if (!name)
	name = ucp->name;
    
    if (!name || !pass)
	return -1;
    
    time(&now);
    
    pthread_mutex_lock(&mtx);

    /* Scan the list of users currently logged in phones for a phone match */
    for (j = 0; j < uc; j++)
	if (uv[j].cphone && strcmp(uv[j].cphone, ucp->phone) == 0)
	    break;

    /* Scan the list of users for a user match */
    for (i = 0; i < uc && strcasecmp(name, uv[i].name) != 0; i++)
	;
    if (i >= uc)
    {
	pthread_mutex_unlock(&mtx);
	return -1;
    }
    
    if (strcasecmp(pass, uv[i].pass) == 0)
    {
	if (j < uc)
	{
	    /* Clear old logged in for this phone (possibly for someone else) */
	    if (uv[j].cphone)
	    {
		free(uv[j].cphone);
		uv[j].cphone = NULL;
	    }
	    uv[j].expires = 0;
	}
	
	/* Clear old logged in phone for this user */
	if (uv[i].cphone)
	{
	    free(uv[i].cphone);
	    uv[i].cphone = NULL;
	}
	
	uv[i].cphone = s_dup(ucp->phone);
	
	if (autologout_time)
	    uv[i].expires = now+autologout_time;
	else
	    uv[i].expires = 0;

	ucp->name = s_dup(name);
	ucp->level = 2;
	nm++;
    }

    pthread_mutex_unlock(&mtx);
    return nm;
}

int
users_logout(UCRED *ucp)
{
    int i;

    
    if (debug)
	fprintf(stderr, "USERS_LOGOUT\n");

    pthread_mutex_lock(&mtx);

    /* Locate the user for the current phone */
    for (i = 0; i < uc && !(uv[i].cphone && strcmp(ucp->phone, uv[i].cphone) == 0); i++)
	;
    
    if (i >= uc)
    {
	pthread_mutex_unlock(&mtx);
	return 0;
    }
    
    free(uv[i].cphone);
    uv[i].cphone = NULL;
    uv[i].expires = 0;

    pthread_mutex_unlock(&mtx);
    return 1;
}


void
users_free_creds(UCRED *ucp)
{
    if (debug)
	fprintf(stderr, "USERS_FREE_CREDS\n");
    
    if (ucp)
    {
	if (ucp->acl)
	    free(ucp->acl);
	if (ucp->name)
	    free(ucp->name);
	if (ucp->phone)
	    free(ucp->phone);
	free(ucp);
    }
}

UCRED *
users_get_creds(const char *phone)
{
    UCRED *ucp;
    int i;
    time_t now;

    
    time(&now);

    ucp = malloc(sizeof(*ucp));
    if (!ucp)
	return NULL;
    
    memset(ucp, 0, sizeof(*ucp));
    ucp->phone = s_dup(phone);
    ucp->name = NULL;
    ucp->acl = NULL;
    ucp->level = 0;
    
    pthread_mutex_lock(&mtx);

    /* Check list of "logged in" phone numbers */
    for (i = 0; i < uc; i++)
	if (uv[i].cphone && strcmp(uv[i].cphone, phone) == 0)
	{
	    ucp->name = s_dup(uv[i].name);
	    ucp->acl = s_dup(uv[i].acl);
	    ucp->level = 2;
	    if (autologout_time)
		uv[i].expires = now+autologout_time;
	    break;
	}

    if (i >= uc)
    {
	/* Check list of "home" phone numbers */
	for (i = 0; i < uc; i++)
	    if (uv[i].pphone && strcmp(uv[i].pphone, phone) == 0)
	    {
		ucp->name = s_dup(uv[i].name);
		ucp->acl = s_dup(uv[i].acl);
		ucp->level = 1;
		if (autologout_time)
		    uv[i].expires = now+autologout_time;
		break;
	    }
    }
    
    pthread_mutex_unlock(&mtx);

    if (debug)
	fprintf(stderr, "USERS_GET_CREDS: Phone=%s, Name=%s, Level=%d ACL=%s\n",
		ucp->phone,
		(ucp->name ? ucp->name : "<null>"),
		ucp->level,
		(ucp->acl ? ucp->acl : "<null>"));
    
    return ucp;
}


/* returns phone number of user */
char *
users_name2phone(const char *name)
{
    int i;
    char *phone = NULL;
    time_t now;
    

    time(&now);
    
    pthread_mutex_lock(&mtx);

    for (i = 0; i < uc; i++)
	if (strcmp(uv[i].name, name) == 0)
	{
	    /* Temporarily "logged in" phone number? */
	    if (uv[i].cphone)
		phone = s_dup(uv[i].cphone);
	    else
		phone = s_dup(uv[i].pphone);
	    break;
	}
    
    pthread_mutex_unlock(&mtx);
    
    return phone;
}


/* verify user access for a command */
int
users_valid_command(UCRED *ucp,
		    const char *command)
{
    int rc = 0;
    time_t now;
    char *acl = NULL;
    char *cp;
    

    time(&now);
    
    if (!ucp->acl)
	goto End;

    if (strcmp(ucp->acl, "*") == 0)
	rc = 1;
    else
    {
	acl = s_dup(ucp->acl);
	cp = strtok(acl, "|");
	while (cp)
	{
	    if (strcasecmp(cp, command) == 0)
	    {
		rc = 1;
		break;
	    }
	    
	    cp = strtok(NULL, "|");
	}
	free(acl);
    }
  End:
    if (debug)
	fprintf(stderr, "USERS_VALID_COMMAND: Command=%s -> %d\n",
		command ? command : "<null>", rc);
    
    return rc;
}


int
users_autologout_start(int at,
		       void (*handler)(USER *up))
{
    autologout_time = at;
    
    pthread_create(&autologout_tid, NULL, autologout_thread, (void *) handler);
    return 0;
}

int
users_autologout_stop(void)
{
    void *status;

    /* XXX: Write this better */

    if (autologout_time > 0)
    {
	autologout_time = 0;
	pthread_kill(autologout_tid, SIGUSR1);
	
	pthread_join(autologout_tid, &status);
    }

    return 0;
}


int
users_foreach(int (*fcp)(USER *up, void *xp), void *xp)
{
    int rc = 0, i;


    pthread_mutex_lock(&mtx);
    for (i = 0; i < uc; i++)
    {
	rc = (*fcp)(&uv[i], xp);
	if (rc)
	    break;
    }
    pthread_mutex_unlock(&mtx);
    
    return 0;
}
