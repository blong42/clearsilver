/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's
 * Library Gnu Public License (LGPL).
 *
 * Copyright (C) 2001 by Brandon Long
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/neo_err.h"
#include "cgi/cgiwrap.h"

typedef struct _cgiwrapper
{
  int argc;
  char **argv;
  char **envp;
  int env_count;

  READ_FUNC read_cb;
  WRITEF_FUNC writef_cb;
  WRITE_FUNC write_cb;
  GETENV_FUNC getenv_cb;
  PUTENV_FUNC putenv_cb;
  ITERENV_FUNC iterenv_cb;

  void *data;
} CGIWRAPPER;

static CGIWRAPPER GlobalWrapper;

static void cgiwrap_init (void)
{
  GlobalWrapper.argc = 0;
  GlobalWrapper.argv = NULL;
  GlobalWrapper.envp = NULL;
  GlobalWrapper.env_count = 0;
  GlobalWrapper.read_cb = NULL;
  GlobalWrapper.writef_cb = NULL;
  GlobalWrapper.write_cb = NULL;
  GlobalWrapper.getenv_cb = NULL;
  GlobalWrapper.putenv_cb = NULL;
  GlobalWrapper.iterenv_cb = NULL;
  GlobalWrapper.data = NULL;
}

void cgiwrap_init_std (int argc, char **argv, char **envp)
{
  cgiwrap_init();
  GlobalWrapper.argc = argc;
  GlobalWrapper.argv = argv;
  GlobalWrapper.envp = envp;
  while (envp[GlobalWrapper.env_count] != NULL) GlobalWrapper.env_count++;
}

void cgiwrap_init_emu (void *data, READ_FUNC read_cb, 
    WRITEF_FUNC writef_cb, WRITE_FUNC write_cb, GETENV_FUNC getenv_cb,
    PUTENV_FUNC putenv_cb, ITERENV_FUNC iterenv_cb)
{
  cgiwrap_init();
  GlobalWrapper.data = data;
  GlobalWrapper.read_cb = read_cb;
  GlobalWrapper.writef_cb = writef_cb;
  GlobalWrapper.write_cb = write_cb;
  GlobalWrapper.getenv_cb = getenv_cb;
  GlobalWrapper.putenv_cb = putenv_cb;
  GlobalWrapper.iterenv_cb = iterenv_cb;
}

NEOERR *cgiwrap_getenv (char *k, char **v)
{
  if (GlobalWrapper.getenv_cb != NULL)
  {
    *v = GlobalWrapper.getenv_cb (GlobalWrapper.data, k);
  }
  else
  {
    char *s = getenv(k);

    if (s != NULL)
    {
      *v = strdup(s);
      if (*v == NULL)
      {
	return nerr_raise (NERR_NOMEM, "Unable to duplicate env var %s=%s", 
	    k, s);
      }
    }
    else
    {
      *v = NULL;
    }
  }
  return STATUS_OK;
}

NEOERR *cgiwrap_putenv (char *k, char *v)
{
  if (GlobalWrapper.putenv_cb != NULL)
  {
    if (GlobalWrapper.putenv_cb(GlobalWrapper.data, k, v))
      return nerr_raise(NERR_NOMEM, "putenv_cb says nomem when %s=%s", k, v);
  }
  else
  {
    char *buf;
    int l;
    l = strlen(k) + strlen(v) + 2;
    buf = (char *) malloc(sizeof(char) * l);
    if (buf == NULL)
      return nerr_raise(NERR_NOMEM, "Unable to allocate memory for putenv %s=%s", k, v);
    snprintf (buf, l, "%s=%s", k, v);
    if (putenv (buf))
      return nerr_raise(NERR_NOMEM, "putenv says nomem when %s", buf);
  }
  return STATUS_OK;
}

NEOERR *cgiwrap_iterenv (int num, char **k, char **v)
{
  *k = NULL;
  *v = NULL;
  if (GlobalWrapper.iterenv_cb != NULL)
  {
    int r;

    r = GlobalWrapper.iterenv_cb(GlobalWrapper.data, num, k, v);
    if (r)
      return nerr_raise(NERR_SYSTEM, "iterenv_cb returned %d", r);
  }
  else if (GlobalWrapper.envp != NULL && num < GlobalWrapper.env_count)
  {
    char *c, *s = GlobalWrapper.envp[num];

    c = strchr (s, '=');
    if (c == NULL) return STATUS_OK;
    *c = '\0';
    *k = strdup(s);
    *c = '=';
    if (*k == NULL)
      return nerr_raise(NERR_NOMEM, "iterenv says nomem for %s", s);
    *v = strdup(c+1);
    if (*v == NULL)
    {
      free(*k);
      *k = NULL;
      return nerr_raise(NERR_NOMEM, "iterenv says nomem for %s", s);
    }
  }
  return STATUS_OK;
}

NEOERR *cgiwrap_writef (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  cgiwrap_writevf (fmt, ap);
  va_end (ap);
  return STATUS_OK;
}

NEOERR *cgiwrap_writevf (char *fmt, va_list ap) 
{
  int r;

  if (GlobalWrapper.writef_cb != NULL)
  {
    r = GlobalWrapper.writef_cb (GlobalWrapper.data, fmt, ap);
    if (r) 
      return nerr_raise (NERR_SYSTEM, "writef_cb returned %d", r);
  }
  else
  {
    vprintf (fmt, ap);
    /* vfprintf(stderr, fmt, ap); */
  }
  return STATUS_OK;
}

NEOERR *cgiwrap_write (char *buf, int buf_len)
{
  int r;

  if (GlobalWrapper.write_cb != NULL)
  {
    r = GlobalWrapper.write_cb (GlobalWrapper.data, buf, buf_len);
    if (r != buf_len)
      return nerr_raise (NERR_IO, "write_cb returned %d<%d", r, buf_len);
  }
  else
  {
    /* r = fwrite(buf, sizeof(char), buf_len, stderr);  */
    r = fwrite(buf, sizeof(char), buf_len, stdout);
    if (r != buf_len)
      return nerr_raise (NERR_IO, "fwrite returned %d<%d", r, buf_len);
  }
  return STATUS_OK;
}

void cgiwrap_read (char *buf, int buf_len, int *read_len)
{
  if (GlobalWrapper.read_cb != NULL)
  {
    *read_len = GlobalWrapper.read_cb (GlobalWrapper.data, buf, buf_len);
  }
  else
  {
    *read_len = fread (buf, sizeof(char), buf_len, stdin);
  }
}
