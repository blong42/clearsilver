/*
 * Copyright (C) 2000 Neotonic - All rights reserved.
 */

#include <stdargs.h>
#include "neo_err.h"
#include "cgiwrap.h"

typedef struct _cgiwrapper
{
  int argc;
  char **argv;
  char **envp;

  READ_FUNC *read_cb;
  WRITEF_FUNC *writef_cb;
  WRITE_FUNC *write_cb;
  GETENV_FUNC *getenv_cb;
  PUTENV_FUNC *putenv_cb;
  ITERENV_FUNC *iterenv_cb;

  void *data;
} CGIWRAPPER;

static CGIWRAPPER GlobalWrapper;

static void cgiwrap_init (void)
{
  GlobalWrapper.argc = 0;
  GlobalWrapper.argv = NULL;
  GlobalWrapper.envp = NULL;
  GlobalWrapper.read_cb = NULL;
  GlobalWrapper.writef_cb = NULL;
  GlobalWrapper.write_cb = NULL;
  GlobalWrapper.getenv_cb = NULL;
  GlobalWrapper.putenv_cb = NULL;
  GlobalWrapper.iterenv_cb = NULL;
  GlobalWrapper.data = NULL;
}

NEOERR *cgiwrap_init_std (int argc, char **argv, char **envp)
{
  cgiwrap_init();
  GlobalWrapper.argc = argc;
  GlobalWrapper.argv = argv;
  GlobalWrapper.envp = envp;
  return STATUS_OK;
}

NEOERR *cgiwrap_init_emu (void *data, READ_FUNC *read_cb, 
    WRITEF_FUNC *writef_cb, WRITE_FUNC *write_cb, GETENV_FUNC *getenv_cb,
    PUTENV_FUNC *putenv_cb, ITERENV_FUNC *iterenv_cb)
{
  cgiwrap_init();
  GlobalWrapper.data = data;
  GlobalWrapper.read_cb = read_cb;
  GlobalWrapper.writef_cb = writef_cb;
  GlobalWrapper.write_cb = write_cb;
  GlobalWrapper.getenv_cb = getenv_cb;
  GlobalWrapper.putenv_cb = putenv_cb;
  GlobalWrapper.iterenv_cb = iterenv_cb;
  return STATUS_OK;
}

NEOERR *cgiwrap_getenv (char *k, char **v)
{
  if (GlobalWrapper.getenv_cb != NULL)
  {
    *v = GlobalWrapper.getenv_cb (k);
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
    if (GlobalWrapper.putenv_cb(k, v))
      return nerr_raise(NERR_NOMEM, "putenv_cb says nomem when %s=%s", k, v);
  }
  else
  {
    char buf[1024];
    snprintf (buf, sizeof(buf), "%s=%s", k, v);
    if (putenv (buf))
      return nerr_raise(NERR_NOMEM, "putenv says nomem when %s", buf);
  }
  return STATUS_OK;
}

NEOERR *cgiwrap_iterenv (int start, char **k, char **v)
{
  if (GlobalWrapper.iterenv_cb != NULL)
  {
    
  }
}

NEOERR *cgiwrap_writef (char *fmt, ...)
{
}

NEOERR *cgiwrap_write (char *buf, int buf_len);
NEOERR *cgiwrap_read (char *buf, int buf_len);
