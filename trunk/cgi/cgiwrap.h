/*
 * Copyright (C) 2000 - Neotonic
 */

/*
 * cgiwrap.h
 * The purpose of the cgiwrap is to abstract the CGI interface to allow
 * for other than the default implementation.  The default
 * implementation is of course based on environment variables and stdio,
 * but this can be used with server modules which can substitute their
 * own implementation of these functions.
 */


#ifndef __CGIWRAP_H_
#define __CGIWRAP_H_ 1

#include <stdarg.h>
#include "neo_err.h"

typedef int (*READ_FUNC)(void *, char *, int);
typedef int (*WRITEF_FUNC)(void *, char *, va_list);
typedef int (*WRITE_FUNC)(void *, char *, int);
typedef char *(*GETENV_FUNC)(void *, char *);
typedef int (*PUTENV_FUNC)(void *, char *, char *);
typedef int (*ITERENV_FUNC)(void *, int, char **, char **);

NEOERR *cgiwrap_init_std (int argc, char **argv, char **envp);
NEOERR *cgiwrap_init_emu (void *data, READ_FUNC read_cb, 
    WRITEF_FUNC writef_cb, WRITE_FUNC write_cb, GETENV_FUNC getenv_cb,
    PUTENV_FUNC putenv_cb, ITERENV_FUNC iterenv_cb);
NEOERR *cgiwrap_getenv (char *k, char **v);
NEOERR *cgiwrap_putenv (char *k, char *v);
NEOERR *cgiwrap_iterenv (int start, char **k, char **v);
NEOERR *cgiwrap_writef (char *fmt, ...);
NEOERR *cgiwrap_writevf (char *fmt, va_list ap);
NEOERR *cgiwrap_write (char *buf, int buf_len);
NEOERR *cgiwrap_read (char *buf, int buf_len, int *read_len);

#endif /* __CGIWRAP_H_ */
