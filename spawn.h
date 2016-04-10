/*
** spawn.c - Spawn subprocesses
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
*/

#ifndef SPAWN_H
#define SPAWN_H 1

extern int
spawn(const char *path,
      char * const *argv,
      int uid, int gid,
      FILE *fpin,
      FILE *fpout,
      FILE *fperr);

#endif
