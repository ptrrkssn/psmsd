/*
 * psmsd.c - Peter's SMS Daemon
 *
 * Copyright (c) 2006-2020 Peter Eriksson <pen@lysator.liu.se>
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include <setjmp.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>

#if HAVE_DOORS
#include <door.h>
#endif

#if HAVE_LOADAVG
#include <sys/loadavg.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "common.h"
#include "serial.h"
#include "queue.h"
#include "gsm.h"
#include "argv.h"
#include "buffer.h"
#include "users.h"
#include "spawn.h"
#include "ptime.h"
#include "strmisc.h"


extern char version[];


typedef struct extcmd
{
    char *name;
    int level;
    char *user;
    char *path;	
    char *argv;	
} ECMD;


typedef struct xmitmsg
{
    char *cmd;
    char *data;
    void (*ack)(int rc, void *misc);
    void *misc;
} XMSG;


extern char version[];
char *argv0 = "psmsd";

int verbose = 0;
int debug = 0;

volatile int abort_threads = 0;
int autologout_time = 0;

char *serial_device = SERIAL_DEVICE;
int serial_speed = SERIAL_SPEED;

int serial_timeout = 30000;

FILE *ser_r_fp = NULL;
FILE *ser_w_fp = NULL;
FILE *tty_fp = NULL;

#if HAVE_DOORS
char *door_path = DOOR_PATH;
#endif

char *fifo_path = FIFO_PATH;
int tty_reader = 0;

QUEUE *q_xmit = NULL;

char *commands_path = NULL;
char *userauth_path = NULL;

pthread_mutex_t ecmd_mtx;
ECMD *ev = NULL;
int es = 0;
int ec = 0;


volatile XMSG *resp_msg = NULL;
pthread_mutex_t resp_mtx;
pthread_cond_t resp_cv;


void
error(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);

    if (!debug)
	vsyslog(LOG_ERR, msg, ap);
    else
    {
	fprintf(stderr, "%s: Error: ", argv0);
	vfprintf(stderr, msg, ap);
	putchar('\n');
    }
    
    va_end(ap);
    
    exit(1);
}


int
_send_sms(const char *phone,
	  const char *msg)
{
    XMSG *xp;
    char buf[1024];
    int len;
    

    if (debug)
	fprintf(stderr, "SEND_SMS: Phone=%s, Msg=%s\n", phone, msg);
	
    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CMGS=\"%s\"", phone);
    xp->cmd = s_dup(buf);

    buf[0] = '\0';
    latin1_to_gsm(msg, buf, sizeof(buf));
    
    xp->data = s_dup(buf);
    len = strlen(xp->data);
    if (len > MAX_SMS_MESSAGE)
	xp->data[MAX_SMS_MESSAGE] = 0;
    
    xp->ack = NULL;
    xp->misc = NULL;
    
    return queue_put(q_xmit, xp);
}


int
do_send(USER *up, void *xp)
{
    const char *msg = (const char *) xp;


    return _send_sms(up->cphone ? up->cphone : up->pphone, msg);
}


int
send_sms(const char *to,
	 const char *msg)
{
    char *phone;


    if (!to || !msg)
	return -1;
    
    if (strcmp(to, "*") == 0)
	return users_foreach(do_send, (void *) msg);

    if (*to == '+' || isdigit(*to))
	return _send_sms(to, msg);
    
    phone = users_name2phone(to);
    if (!phone)
	return -1;
    
    return _send_sms(phone, msg);
}




int
ecmd_load(const char *ecmdpath)
{
    FILE *fp;
    char buf[1024], *name, *user, *path, *argv;
    int level;


    if (debug)
	fprintf(stderr, "ECMD_LOAD: Start\n");

    pthread_mutex_lock(&ecmd_mtx);
    if (ev)
	free(ev);

    ec = 0;
    ev = malloc(sizeof(*ev)*(es = 128));
    if (!ev)
    {
	pthread_mutex_unlock(&ecmd_mtx);
	if (debug)
	    fprintf(stderr, "ECMD_LOAD: malloc failed: %s\n", strerror(errno));
	return -1;
    }
    
    fp = fopen(ecmdpath, "r");
    if (!fp)
    {
	pthread_mutex_unlock(&ecmd_mtx);
	if (debug)
	    fprintf(stderr, "ECMD_LOAD: fopen (%s) failed: %s\n", ecmdpath, strerror(errno));
	return -1;
    }
    
    while (fgets(buf, sizeof(buf), fp))
    {
	char *tmp, *endp;
	
	name = strtok_r(buf, " \t\r\n", &endp);
	if (!name || *name == '#')
	    continue;
	
	tmp = strtok_r(NULL, " \t\r\n", &endp);
	if (!tmp)
	    continue;
	if (sscanf(tmp, "%u", &level) != 1)
	{
	    if (strcmp(tmp, "*") == 0 || strcmp(tmp, "all") == 0)
		level = 0;
	    else if (strcmp(tmp, "phone") == 0)
		level = 1;
	    else if (strcmp(tmp, "login") == 0)
		level = 2;
	    else
		level = 3;
	}
	
	user = strtok_r(NULL, " \t\r\n", &endp);
	if (!user)
	    continue;

	path = strtok_r(NULL, " \t\r\n", &endp);
	if (!path)
	    continue;

	argv = strtok_r(NULL, "\r\n", &endp);
	if (!argv)
	    continue;

	while (isspace(*argv))
	    ++argv;
	
	if (ec == es)
	    ev = realloc(ev, sizeof(*ev)*(es += 128));

	if (debug > 1)
	    fprintf(stderr, "ECMD_LOAD: Name=%s, Level=%d, Path=%s, Argv=%s\n", name, level, path, argv);
	
	ev[ec].name = s_dup(name);
	ev[ec].level = level;
	ev[ec].user = s_dup(user);
	ev[ec].path = s_dup(path);
	ev[ec].argv  = s_dup(argv);
	++ec;
    }

    fclose(fp);

    pthread_mutex_unlock(&ecmd_mtx);
    
    if (debug)
	fprintf(stderr, "ECMD_LOAD: Stop\n");
    
    return ec;
}




int
ecmd_list(UCRED *ucp,
	  BUFFER *out)
{
    int i;

    pthread_mutex_lock(&ecmd_mtx);
    for (i = 0; i < ec; i++)
    {
	if (users_valid_command(ucp, ev[i].name) &&
	    ucp->level >= ev[i].level)
	{
	    buf_puts(out, ",");
	    buf_puts(out, ev[i].name);
	}
    }

    pthread_mutex_unlock(&ecmd_mtx);
    
    return ec;
}

struct ecmd_escapes {
    char **argv;
    const char *phone;
    const char *date;
    const char *user;
};
    

static char *
ecmd_esc_handler(const char *esc,
		 void *xtra)
{
    struct ecmd_escapes *ep = (struct ecmd_escapes *) xtra;
    int start, stop, rc;
    char c;
    const char *rv = NULL;

    
    if (strcmp(esc, "P") == 0 || strcmp(esc, "phone") == 0)
	rv = ep->phone;

    else if (strcmp(esc, "D") == 0 || strcmp(esc, "date") == 0)
	rv = ep->date;
    
    else if (strcmp(esc, "U") == 0 || strcmp(esc, "user") == 0)
	rv = ep->user;

    else if (strcmp(esc, "*") == 0)
	rv = argv_getm(ep->argv, 1, 0);

    else if (sscanf(esc, "%u-%u%c", &start, &stop, &c) == 2)
	rv = argv_getm(ep->argv, start, stop);

    else if (sscanf(esc, "-%u%c", &stop, &c) == 2)
	rv = argv_getm(ep->argv, 1, stop);

    else
    {
	rc = sscanf(esc, "%u%c", &start, &c);

	if (rc == 1)
	    rv = argv_get(ep->argv, start);
	else if (rc == 2 && c == '-')
	    rv = argv_getm(ep->argv, start, 0);
    }

    return s_dup(rv);
}


char *
ecmd_run(UCRED *ucp,
	 char **argv,
	 const char *date,
	 BUFFER *in,
	 BUFFER *out)
{
    struct ecmd_escapes edata;
    int i, pid, rc;
    FILE *fp_in = NULL, *fp_out = NULL;
    char **cmd_argv = NULL;
    char *path = NULL;
    char *user = NULL;
    int uid = 60001, gid = 60001;
    struct passwd pb, *pp;
    char buf[256];
	
    
    
    if (!argv || !argv[0])
	return NULL;

    edata.argv = argv;
    edata.user = ucp->name;
    edata.phone = ucp->phone;
    edata.date = date;

    if (debug)
    {
	fprintf(stderr, "ECMD_RUN: Argv=");
	for (i = 0; argv[i]; ++i)
	    fprintf(stderr, "%s ", argv[i]);
	putc('\n', stderr);
    }
    
    pthread_mutex_lock(&ecmd_mtx);
    for (i = 0; i < ec && strcasecmp(argv[0], ev[i].name) != 0; i++)
	;
    if (i >= ec)
    {
	pthread_mutex_unlock(&ecmd_mtx);
	return NULL;
    }

    if (!(users_valid_command(ucp, ev[i].name) && ev[i].level <= ucp->level))
    {
	pthread_mutex_unlock(&ecmd_mtx);
	return NULL;
    }
	
    cmd_argv = argv_create(ev[i].argv, ecmd_esc_handler, (void *) &edata);
    path = s_dup(ev[i].path);
    user = s_dup(ev[i].user);
    
    pthread_mutex_unlock(&ecmd_mtx);


    if (in)
    {
	fp_in = tmpfile();
	if (!fp_in)
	    goto Fail;
	
	buf_save(in, fp_in);
	fflush(fp_in);
	rewind(fp_in);
    }

    fp_out = tmpfile();
    if (!fp_out)
	goto Fail;

    pp = NULL;
    if (ucp->name && strcmp(user, "=") == 0)
	getpwnam_r(ucp->name, &pb, buf, sizeof(buf), &pp);
    else
	getpwnam_r(user, &pb, buf, sizeof(buf), &pp);
    if (pp)
    {
	uid = pp->pw_uid;
	gid = pp->pw_gid;
    }
    
    if (debug)
    {
	fprintf(stderr, "ECMD_RUN: Spawn: Uid=%d, Gid=%d, Path=%s, Argv=", uid, gid, path);
	for (i = 0; cmd_argv[i]; ++i)
	    fprintf(stderr, "%s ", cmd_argv[i]);
	putc('\n', stderr);
    }
    
    pid = spawn(path, cmd_argv, uid, gid, fp_in, fp_out, NULL);
    if (pid < 0)
	goto Fail;	

    while (waitpid(pid, &rc, 0) < 0 && errno == EINTR)
	;
    
    fflush(fp_out);
    rewind(fp_out);

    buf_load(out, fp_out);
    fclose(fp_in);
    fclose(fp_out);
    if (user)
	free(user);
    if (path)
	free(path);

    if (debug)
	fprintf(stderr, "ECMD_RUN: Command output: %s\n", buf_getall(out));
    
    return buf_getall(out);
    
  Fail:
    if (debug)
	fprintf(stderr, "ECMD_RUN: Failed, returning NULL\n");
    
    if (fp_in)
	fclose(fp_in);
    if (fp_out)
	fclose(fp_out);
    if (user)
	free(user);
    if (path)
	free(path);
    return NULL;
}


void *
ser_xmit_thread(void *tap)
{
    XMSG *p;


    if (debug)
	fprintf(stderr, "SER_XMIT_THREAD: Starting\n");
    
    while ((p = (XMSG *) queue_get(q_xmit)) != NULL)
    {
	pthread_mutex_lock(&resp_mtx);
	while (resp_msg != NULL)
	    pthread_cond_wait(&resp_cv, &resp_mtx);
	resp_msg = p;
	pthread_mutex_unlock(&resp_mtx);
	pthread_cond_signal(&resp_cv);

	if (debug > 1)
	    fprintf(stderr, "XMIT: MSG: %s, DATA: %s\n", p->cmd, p->data ? p->data : "<null>");

	fprintf(ser_w_fp, "AT%s\r", p->cmd);
	fflush(ser_w_fp);

	if (p->data)
	{
	    /* XXX: Should wait for ">" */
	    sleep(1);
	    fputs(p->data, ser_w_fp);
	    putc(0x1A, ser_w_fp);
	    fflush(ser_w_fp);
	}

    }

    if (debug)
	fprintf(stderr, "SER_XMIT_THREAD: Stopping\n");
    return NULL;
}


int
read_sms(int id)
{
    XMSG *xp;
    char buf[1024];
    

    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CMGR=%u", id);
    xp->cmd = s_dup(buf);
    xp->data = NULL;
    xp->ack = NULL;
    xp->misc = NULL;

    return queue_put(q_xmit, xp);
}

int
list_sms(char *type)
{
    XMSG *xp;
    char buf[1024];
    

    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CMGL=\"%s\"", type);
    xp->cmd = s_dup(buf);
    xp->data = NULL;
    xp->ack = NULL;
    xp->misc = NULL;

    return queue_put(q_xmit, xp);
}


int
select_charset(char *type)
{
    XMSG *xp;
    char buf[1024];
    

    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CSCS=\"%s\"", type);
    xp->cmd = s_dup(buf);
    xp->data = NULL;
    xp->ack = NULL;
    xp->misc = NULL;

    return queue_put(q_xmit, xp);
}
int
send_pin(char *pin)
{
    XMSG *xp;
    char buf[1024];
    

    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CPIN=%s", pin);
    xp->cmd = s_dup(buf);
    xp->data = NULL;
    xp->ack = NULL;
    xp->misc = NULL;

    return queue_put(q_xmit, xp);
}

int
delete_sms(int id, int mode)
{
    XMSG *xp;
    char buf[1024];
    

    xp = malloc(sizeof(*xp));
    if (!xp)
	return -1;

    snprintf(buf, sizeof(buf), "+CMGD=%u,%u", id, mode);
    xp->cmd = s_dup(buf);
    xp->data = NULL;
    xp->ack = NULL;
    xp->misc = NULL;

    return queue_put(q_xmit, xp);
}

static int
cmd_users(USER *up, void *xp)
{
    BUFFER *bp = (BUFFER *) xp;

    if (up->cphone)
    {
	buf_puts(bp, up->name);
	buf_putc(bp, ' ');
	buf_puts(bp, up->cphone);
	buf_putc(bp, '\n');
    }
    return 0;
}

#if !HAVE_LOADAVG
int
getloadavg(double lav[],
           int nlav) {
  errno = ENOSYS;
  return -1;
}
#endif

int
run_message(const char *msg,
	    const char *phone,
	    const char *date)
{
    char tmpbuf[1024], *cp, **argv, **sargv;
    int i, len;
    BUFFER in, out;
    UCRED *ucp;
    
    
    ucp = users_get_creds(phone);
    if (!ucp)
	return -1;
    
    buf_init(&in);
    buf_init(&out);

    cp = s_dup(msg);
    i = strcspn(cp, "\r\n");
    if (i >= 0)
	cp[i++] = '\0';

    while (cp[i] && isspace(cp[i]))
	++i;
    
    if (!debug)
	syslog(LOG_INFO, "Date=%s, Phone=%s, User=%s, Level=%d: %s",
	       date, ucp->phone, ucp->name ? ucp->name : "<unknown>", ucp->level, cp);
    else
	fprintf(stderr, "RUN_MSG: Date=%s, Phone=%s, User=%s, Level=%d: %s\n",
		date, ucp->phone, ucp->name ? ucp->name : "<unknown>", ucp->level, cp);
    
    buf_puts(&in, cp+i);
    sargv = argv = argv_create(cp, NULL, NULL);
    free(cp);

    if (!argv || !argv[0])
    {
	argv_destroy(argv);
	buf_clear(&in);
	buf_clear(&out);
	users_free_creds(ucp);
	return -1;
    }

    if (argv[0] && (len = strlen(argv[0])) > 2 &&
	argv[0][0] == '[' && argv[0][len-1] == ']' &&
	ucp->name)
    {
	argv[0][len-1] = '\0';
	if (!users_login(ucp, ucp->name, argv[0]+1))
	{
	    buf_puts(&out, "Invalid password");
	    goto End;
	}

	++argv;
    }
    
    if (strcasecmp(argv[0], "Help") == 0)
    {
	buf_puts(&out, "Help,Whoami,Login");
	if (ucp->level > 0)
	    buf_puts(&out, ",LoadAvg,Users");
	if (ucp->level > 1)
	    buf_puts(&out, ",Logout");
	ecmd_list(ucp, &out);
	goto End;
    }
    
    if (strcasecmp(argv[0], "Whoami") == 0)
    {
	buf_puts(&out, ucp->phone);
	if (ucp->level > 0)
	{
	    buf_puts(&out, " ");
	    if (ucp->level < 2)
		buf_puts(&out, "(");
	    buf_puts(&out, ucp->name);
	    if (ucp->level < 2)
		buf_puts(&out, ")");
	}
	goto End;
    }

    if (strcasecmp(argv[0], "Login") == 0)
    {
	if (!argv[1] || !argv[2] || !users_login(ucp, argv[1], argv[2]))
	{
	    if (ucp->name)
		buf_puts(&out, "Login denied!");
	}
	else
	    buf_puts(&out, "Login OK");
	goto End;
    }

    if (ucp->level > 1 && strcasecmp(argv[0], "Logout") == 0)
    {
	if (users_logout(ucp))
	    buf_puts(&out, "Logout OK");
	else
	{
	    if (ucp->name)
		buf_puts(&out, "Logout denied!");
	}
	goto End;
    }

    if (ucp->level > 0 && strcasecmp(argv[0], "LoadAvg") == 0)
    {
	double loadavg[3];
	
	if (getloadavg(loadavg, 3) < 0)
          buf_puts(&out, "No load averages");
        else {
          snprintf(tmpbuf, sizeof(tmpbuf), "%.2f/%.2f/%.2f",
                   loadavg[0], loadavg[1], loadavg[2]);
          buf_puts(&out, tmpbuf);
        }
        goto End;
    }

    if (ucp->level > 0 && strcasecmp(argv[0], "Users") == 0)
    {
	users_foreach(cmd_users, (void *) &out);
	goto End;
    }

    if (ucp->level > 0 && ecmd_run(ucp, argv, date, &in, &out) != NULL)
	goto End;


    /* Only return an error if sent from a valid user or known phone */
    if (ucp->level > 0)
    {
	buf_puts(&out, "What?\r(");
	buf_puts(&out, msg);
	buf_puts(&out, ")");
    }

  End:
    cp = buf_getall(&out);
    if (cp && *cp)
	send_sms(phone, cp);
    
    buf_clear(&in);
    buf_clear(&out);
    users_free_creds(ucp);
    argv_destroy(sargv);
    
    return 0;
}


static jmp_buf sigusr1_env;

void
sigusr1_handler(int sig)
{
    longjmp(sigusr1_env, 1);
}

void *
ser_recv_thread(void *tap)
{
    char buf[1024], status[64], phone[128], date[128];
    int i, id;
    int delete_read_msgs = 0;
    

    if (debug)
	fprintf(stderr, "SER_RECV_THREAD: Starting\n");

    setjmp(sigusr1_env);
    signal(SIGUSR1, sigusr1_handler);
    
    while (!abort_threads && fgets(buf, sizeof(buf), ser_r_fp) != NULL)
    {
	for (i = strlen(buf); i > 0 && isspace(buf[i-1]); i--)
	    ;

	buf[i] = '\0';
	
	if (debug > 1)
	    fprintf(stderr, "RECV: %s\n", buf);

	if (sscanf(buf, "+CMTI: \"SM\",%u", &id) == 1)
	{
	    if (debug)
		fprintf(stderr, "NEW INCOMING SMS #%u\n", id);

	    read_sms(id);
	}
	
	else if (sscanf(buf, "+CMGL: %u,\"%20[^\"]\",\"%80[^\"]\",,\"%80[^\"]\"",
			&id, status, phone, date) == 4)
	{
	    if (debug)
		fprintf(stderr, "SMS #%u FROM %s AT %s STATUS %s\n",
			id, phone, date, status);
	    
	    if (fgets(buf, sizeof(buf), ser_r_fp) != NULL)
	    {
		char obuf[1024];

		gsm_to_latin1(buf, obuf, sizeof(obuf));
		if (debug)
		    fprintf(stderr, "MESSAGE: %s\n", obuf);

		run_message(obuf, phone, date);
		delete_read_msgs = 1;
	    }
	}

	else if (sscanf(buf, "+CMGR: \"%20[^\"]\",\"%80[^\"]\",,\"%80[^\"]\"",
			status, phone, date) == 3)
	{
	    if (debug)
		fprintf(stderr, "SMS FROM %s AT %s STATUS %s\n",
			phone, date, status);
	    
	    if (fgets(buf, sizeof(buf), ser_r_fp) != NULL)
	    {
		char obuf[1024];

		gsm_to_latin1(buf, obuf, sizeof(obuf));
		if (debug)
		    fprintf(stderr, "MESSAGE: %s\n", obuf);

		run_message(obuf, phone, date);
		delete_read_msgs = 1;
	    }
	}

	else if (strcmp(buf, "OK") == 0 ||
		 strcmp(buf, "ERROR") == 0)
	{
	    int rc = (strcmp(buf, "OK") != 0);

	    if (debug)
		fprintf(stderr, "ACKNOWLEDGE OF TYPE: %s (rc=%d)\n", buf, rc);
	    
	    if (delete_read_msgs)
	    {
		if (debug)
		    fprintf(stderr, "DELETING READ MESSAGES\n");

		delete_sms(1,2);
		delete_read_msgs = 0;
	    }
	    
	    pthread_mutex_lock(&resp_mtx);
	    while (resp_msg == NULL)
		pthread_cond_wait(&resp_cv, &resp_mtx);
	    
	    if (resp_msg->ack)
	    {
		resp_msg->ack(rc, resp_msg->misc);
	    }
	    if (resp_msg->data)
		free(resp_msg->data);
	    if (resp_msg->cmd)
		free(resp_msg->data);
	    free((void *) resp_msg);
	    resp_msg = NULL;
	    
	    pthread_mutex_unlock(&resp_mtx);
	    pthread_cond_signal(&resp_cv);
	}
	
	else if (*buf)
	    if (debug)
		fprintf(stderr, "IGNORING: %s\n", buf);
    }

    if (debug)
	fprintf(stderr, "SER_RECV_THREAD: Stopping (error: %s)\n", strerror(errno));
    return NULL;
}

void *
tty_read_thread(void *tap)
{
    char buf[256];
    

    if (debug)
	fprintf(stderr, "TTY_READ_THREAD: Starting\n");
    
    while (fgets(buf, sizeof(buf), tty_fp) != NULL)
    {
	char *phone, *cp, *endp;
	
	if (debug > 1)
	    fprintf(stderr, "TTY RECV: %s\n", buf);

	phone = strtok_r(buf, " \t\r\n", &endp);
	if (!phone)
	    continue;

	cp = strtok_r(NULL, "\n\r", &endp);
	if (!cp)
	    continue;

	while (isspace(*cp))
	    ++cp;

	if (!*cp)
	    continue;
	
	send_sms(phone, cp);
    }

    if (debug)
	fprintf(stderr, "TTY_READ_THREAD: Stopping\n");
    
    queue_put(q_xmit, NULL);
    return NULL;
}


void *
fifo_read_thread(void *tap)
{
    char buf[256];
    FILE *fp;
    
    

    if (debug)
	fprintf(stderr, "FIFO_READ_THREAD: Starting (fifo=%s)\n", (char *) tap);

    while ((fp = fopen((char *) tap, "r")) != NULL)
    {
	if (debug > 1)
	    fprintf(stderr, "FIFO_READ_THREAD: Fifo opened\n");
	
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
	    char *phone, *cp, *endp;
	    
	    if (debug > 1)
		fprintf(stderr, "FIFO RECV: %s\n", buf);
	    
	    phone = strtok_r(buf, " \t\r\n", &endp);
	    if (!phone)
		continue;
	    
	    cp = strtok_r(NULL, "\n\r", &endp);
	    if (!cp)
		continue;
	    
	    while (isspace(*cp))
		++cp;
	    
	    if (!*cp)
		continue;
	    
	    send_sms(phone, cp);
	}

	fclose(fp);
	if (debug > 1)
	    fprintf(stderr, "FIFO_READ_THREAD: Fifo closed\n");
	
    }

    if (debug)
	fprintf(stderr, "FIFO_READ_THREAD: Stopping\n");
    
    queue_put(q_xmit, NULL);
    return NULL;
}


#if !HAVE_CLOSEFROM
void
closefrom(int fd) {
  while (fd < 256)
    close(fd++);
}
#endif

#if HAVE_DOORS
static void
door_servproc(void *cookie,
	      char *dataptr,
	      size_t datasize,
	      door_desc_t *descptr,
	      uint_t ndesc)
{
    door_cred_t cb;
    int rc = -1;
    DOORSMS *dsp;


    if (debug)
	fprintf(stderr, "DOOR: servproc: Start\n");
    
    door_cred(&cb);

    if (debug)
	fprintf(stderr, "DOOR: credentials: euid=%d, egid=%d, ruid=%d, rgid=%d\n",
		(int) cb.dc_euid, (int) cb.dc_egid, (int) cb.dc_ruid, (int) cb.dc_rgid);
    
    if (dataptr == DOOR_UNREF_DATA)
    {
	if (debug)
	    fprintf(stderr, "DOOR: servproc: door unreferenced\n");

	rc = -1;
	door_return((char *) &rc, sizeof(rc), NULL, 0);
	return;
    }

    if (datasize != sizeof(*dsp))
    {
	if (debug)
	    fprintf(stderr, "DOOR: servproc: invalid request structure\n");

	rc = -2;
	door_return((char *) &rc, sizeof(rc), NULL, 0);
	return;
    }

    dsp = (DOORSMS *) dataptr;
    if (debug)
	fprintf(stderr, "DOOR: servproc: sending SMS to %s: %s\n", dsp->phone, dsp->message);
    
    rc = send_sms(dsp->phone, dsp->message);

    if (debug)
	fprintf(stderr, "DOOR: servproc: send_sms returned: %d\n", rc);
    
    door_return((char *) &rc, sizeof(rc), NULL, 0);
}



int
door_start_server(const char *path)
{
    int rc, fd;

    
    while ((rc = unlink(path)) < 0 && errno == EINTR)
	;
    if (rc < 0 && errno != ENOENT)
	error("unlink(\"%s\") failed: %s\n", path, strerror(errno));
    
    while ((fd = open(path, O_CREAT|O_RDWR, 0644)) < 0 && errno == EINTR)
	;
    if (fd < 0)
	error("open(\"%s\") failed: %s\n", path, strerror(errno));
    
    while ((rc = close(fd)) < 0 && errno == EINTR)
	;
    if (rc < 0)
	error("close(%d) failed: %s\n", fd, strerror(errno));

    fd = door_create(door_servproc, NULL, DOOR_UNREF);
    if (fd < 0)
	error("door_create() failed: %s\n", strerror(errno));

    if (fattach(fd, path) < 0)
	error("fattach(\"%s\") failed: %s\n", path, strerror(errno)); 
    
    return 0;
}
#endif


static void
autologout_handler(USER *up)
{
    send_sms(up->cphone, "Autologout\r(Inactivity)");
}

void
daemonize(void)
{
    pid_t pid;
    int fd;


    pid = fork();
    if (pid < 0)
	exit(EXIT_FAILURE);
    else if (pid > 0)
	exit(EXIT_SUCCESS);

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
    setsid();

    while (chdir("/") < 0 && errno == EINTR)
	;
    umask(022);

    closefrom(0);
    while ((fd = open("/dev/null", O_RDWR)) < 0 && errno == EINTR)
	;
    
    if (fd != 0)
	dup2(fd, 0);
    if (fd != 1)
	dup2(fd, 1);
    if (!debug && fd != 2)
	dup2(fd, 2);
    if (fd > 2)
	close(fd);
}

void
usage(FILE *fp, char *argv0)
{
    fprintf(fp, "Usage: %s [<options>] <serial device>\n",
	    argv0);
    fprintf(fp, "Options:\n");
    fprintf(fp, "  -h                    Display this information\n");
    fprintf(fp, "  -V                    Print version and exit\n");
    fprintf(fp, "  -C<commands-path>     Path to commands definition file\n");
    fprintf(fp, "  -U<users-path>        Path to users definition file\n");
    fprintf(fp, "  -T<autologout-time>   Set autologout timeout\n");
    fprintf(fp, "  -d[<level>]           Set debug level\n");
    fprintf(fp, "  -v[<level>]           Set verbosity level\n");
    fprintf(fp, "  -t                    Enable TTY reader\n");
    fprintf(fp, "  -p<pin>               SIM card PIN code\n");
    fprintf(fp, "  -F<fifo-path>         Path to fifo\n");
#if HAVE_DOORS
    fprintf(fp, "  -D<door-path>         Path to door\n");
#endif
}

void
p_header(void)
{
    printf("[psmsd, version %s - Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>]\n", VERSION);
}


int
main(int argc,
     char *argv[])
{
    pthread_t t_recv, t_xmit, t_tty, t_fifo;
    sigset_t srvsigset;
    int sig, rc, fd, i;
    char *pin = NULL;
    double t;
    

    argv0 = argv[0];
    
    for (i = 1; i < argc && argv[i][0] == '-'; i++)
	switch (argv[i][1])
	{
	  case 'V':
	    p_header();
	    exit(0);
	    
	  case 'C':
	    if (!argv[i][2])
		error("Missing path argument for -C");
	    
	    commands_path = s_dup(argv[i]+2);
	    break;
	    
	  case 'U':
	    if (!argv[i][2])
		error("Missing path argument for -U");
	    
	    userauth_path = s_dup(argv[i]+2);
	    break;
	    
	  case 'T':
	    rc = time_get(argv[i]+2, &t);
	    if (rc > 0)
	    {
		if (t < 1 || t > 0)
		    autologout_time = 1;
		else
		    autologout_time = t;
	    }
	    else if (rc == 0)
		autologout_time = 60*10;
	    else
		error("Invalid time specification for -T");
	    break;
	    
	  case 'd':
	    if (argv[i][2])
	    {
		if (sscanf(argv[i]+2, "%d", &debug) != 1)
		    error("Invalid argument for -d");
	    }
	    else
		++debug;
	    break;
	    
	  case 'v':
	    if (argv[i][2])
	    {
		if (sscanf(argv[i]+2, "%d", &verbose) != 1)
		    error("Invalid argument for -v");
	    }
	    else
		++verbose;
	    break;

	  case 't':
	    tty_reader++;
	    break;

	  case 'p':
	    pin = s_dup(argv[i]+2);
	    break;
	    
	  case 'F':
	    if (argv[i][2])
		fifo_path = s_dup(argv[i]+2);
	    else
		fifo_path = FIFO_PATH;
	    break;
	    
#if HAVE_DOORS	    
	  case 'D':
	    if (argv[i][2])
		door_path = s_dup(argv[i]+2);
	    else
		door_path = DOOR_PATH;
	    break;
#endif
	    
	  case 'h':
	    usage(stdout, argv[0]);
	    exit(0);
	    
	  default:
	    fprintf(stderr, "%s: Invalid switch: %s\n",
		    argv[0], argv[i]);
	    exit(1);
	}

    if (verbose)
	p_header();

    if (i < argc)
	serial_device = argv[i++];
    
    if (access(serial_device, R_OK|W_OK) < 0) {
	fprintf(stderr, "%s: %s: %s\n", argv[0], serial_device, strerror(errno));
	exit(1);
    }

    if (fifo_path) {
	(void) mkfifo(fifo_path, 0660);
	/* XXX: Check for errors */
    }
    
    if (!debug)
	daemonize();
    
    openlog(argv[0], LOG_NDELAY|LOG_NOWAIT|(verbose ? LOG_CONS : 0), LOG_LOCAL3);
    syslog(LOG_INFO, "Version %s started", VERSION);
    
    fd = serial_open(serial_device, serial_speed, serial_timeout);
    if (fd < 0)
	error("Open of serial device: %s: %s", serial_device, strerror(errno));

    ser_r_fp = fdopen(fd, "r");
    ser_w_fp = fdopen(fd, "w");

    putc(27, ser_w_fp);
    fflush(ser_w_fp);
    sleep(1);
			   
    sigemptyset(&srvsigset);
    sigaddset(&srvsigset, SIGINT);
    sigaddset(&srvsigset, SIGHUP);
    sigaddset(&srvsigset, SIGTERM);
    sigaddset(&srvsigset, SIGPIPE);
#ifdef SIGTTOU
    sigaddset(&srvsigset, SIGTTOU);
#endif
    
    pthread_sigmask(SIG_BLOCK, &srvsigset, NULL);

    pthread_mutex_init(&resp_mtx, NULL);
    pthread_cond_init(&resp_cv, NULL);
    
    pthread_mutex_init(&ecmd_mtx, NULL);
    
    if (commands_path)
	ecmd_load(commands_path);
    
    if (userauth_path)
	users_load(userauth_path);
    
    q_xmit = queue_create();
    
    if (debug)
	fprintf(stderr, "MAIN: Starting threads:\n");
    
    pthread_create(&t_recv, NULL, ser_recv_thread, NULL);
    pthread_create(&t_xmit, NULL, ser_xmit_thread, NULL);

    if (autologout_time > 0)
	users_autologout_start(autologout_time, autologout_handler);
    
    if (pin)
	send_pin(pin);

    select_charset("HEX");
    
    list_sms("ALL");
    
#if HAVE_DOORS
    if (door_path)
	door_start_server(door_path);
#endif
    
    if (fifo_path)
	pthread_create(&t_fifo,  NULL, fifo_read_thread, (void *) fifo_path);

    if (tty_reader)
	pthread_create(&t_tty,  NULL, tty_read_thread, NULL);

    if (debug)
	fprintf(stderr, "MAIN: Waiting for signals...\n");
    
    while ((rc = sigwait(&srvsigset, &sig)) == 0)
    {
	if (debug)
	    fprintf(stderr, "MAIN: Got signal: %d\n", sig);
	
	switch (sig)
	{
	  case SIGHUP:
	    /* Close and reopen config & log files */
	    if (commands_path)
		ecmd_load(commands_path);
	    if (userauth_path)
		users_load(userauth_path);
	    break;
	    
	  case SIGTERM:
	    /* Terminate gracefully - close server socket, but wait for
	       other active threads to finish */

	    abort_threads = 1;
	    
	    /* Tell SER_XMIT to terminate */
	    queue_put(q_xmit, NULL);

	    /* Tell SER_RECV to terminate */
	    pthread_kill(t_recv, SIGUSR1);
	    
	    /* XXX: Kill autologout_thread and tty_read_thread - if active */
	    if (debug)
		fprintf(stderr, "Stopping autologout thread...\n");
	    users_autologout_stop();
	    if (debug)
		fprintf(stderr, "Stopping tty reader thread...\n");
	    close(0);
	    if (debug)
		fprintf(stderr, "Terminating main thread...\n");
	    pthread_exit(NULL);

	  case SIGPIPE:
	  case SIGTTOU:
	    /* Ignore */
	    break;

	  case SIGINT:
	    queue_put(q_xmit, NULL);
	    exit(1);
	    
	  default:
	    fprintf(stderr, "MAIN: Unhandled signal (%d) received", sig);
	}
    }

    return 1;
}
