/*
** users.h - User database handling routines
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef USERS_H
#define USERS_H

typedef struct user
{
    char *name;
    char *pass;
    
    char *acl;
    
    char *pphone; 	/* Primary phone */
    char *cphone;	/* Current logged in phone */
    
    time_t expires;
} USER;


typedef struct ucred
{
    char *phone;
    char *name;
    char *acl;
    int level; /* 0 = unknown, 1 = known, 2 = logged in */
} UCRED;


extern int
users_load(const char *path);


extern int
users_login(UCRED *ucp,
	    const char *name,
	    const char *pass);

extern int
users_logout(UCRED *ucp);

extern void
users_free_creds(UCRED *ucp);

extern UCRED *
users_get_creds(const char *phone);

extern int
users_valid_command(UCRED *ucp,
		    const char *command);

extern char *
users_name2phone(const char *name);

extern int
users_autologout_start(int logout_time,
		       void (*logout_handler)(USER *up));

extern int
users_autologout_stop(void);

extern int
users_foreach(int (*fcp)(USER *up, void *xp), void *xp);

#endif
