/*
 * Copyright 2001-2004 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
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
#include "util/neo_err.h"

__BEGIN_DECLS

typedef int (*READ_FUNC)(void *, char *, int);
typedef int (*WRITEF_FUNC)(void *, const char *, va_list);
typedef int (*WRITE_FUNC)(void *, const char *, int);
typedef char *(*GETENV_FUNC)(void *, const char *);
typedef int (*PUTENV_FUNC)(void *, const char *, const char *);
typedef int (*ITERENV_FUNC)(void *, int, char **, char **);

/* 
 * Function: cgiwrap_init_std - Initialize cgiwrap with default functions
 * Description: cgiwrap_init_std will initialize the cgiwrap subsystem 
 *              to use the default CGI functions, ie
 *              getenv/putenv/stdio.  In reality, all this is doing is
 *              setting up the data for the cgiwrap_iterenv() function.
 * Input: the arguments to main, namely argc/argv/envp
 * Output: None
 * Returns: None
 */
void cgiwrap_init_std (int argc, char **argv, char **envp);

/* 
 * Function: cgiwrap_init_emu - initialize cgiwrap for emulated use
 * Description: cgiwrap_init_emu sets up the cgiwrap subsystem for use
 *              in an emulated environment where you are providing
 *              routines to use in place of the standard routines, ie
 *              when used to interface with a server or scripting
 *              language.
 *              See cgi/cgiwrap.h for the exact definitions of the
 *              callback functions.
 * Input: data - user data to be passed to the specified callbacks
 *        read_cb - a cb to replace fread(stdin)
 *        writef_cb - a cb to repalce fprintf(stdout)
 *        write_cb - a cb to replace fwrite(stdout)
 *        getenv_cb - a cb to replace getenv
 *        putenv_cb - a cb to replace putenv
 *        iterenv_cb - a cb to replace the default environment iteration
 *                     function (which just wraps walking the envp array)
 * Output: None
 * Returns: None
 */
void cgiwrap_init_emu (void *data, READ_FUNC read_cb, 
    WRITEF_FUNC writef_cb, WRITE_FUNC write_cb, GETENV_FUNC getenv_cb,
    PUTENV_FUNC putenv_cb, ITERENV_FUNC iterenv_cb);

/* 
 * Function: cgiwrap_getenv - the wrapper for getenv
 * Description: cgiwrap_getenv wraps the getenv function for access to
 *              environment variables, which are used to pass data to
 *              CGI scripts.  This version differs from the system
 *              getenv in that it makes a copy of the value it returns,
 *              which gets around problems when wrapping this routine in
 *              garbage collected/reference counted languages by
 *              moving the ownership of the data to the calling
 *              function.
 * Input: k - the environment variable to lookup
 * Output: v - a newly allocated copy of the value of that variable, or
 *             NULL if not found.
 * Returns: NERR_NOMEM if there isn't memory available to allocate the result
 */
NEOERR *cgiwrap_getenv (const char *k, char **v);

/* 
 * Function: cgiwrap_putenv - wrap the putenv call
 * Description: cgiwrap_putenv wraps the putenv call.  This is mostly
 *              used by the cgi_debug_init function to create an
 *              artificial environment.  This version differs from the
 *              system version by having separate arguments for the
 *              variable name and value, which makes life easier for the
 *              caller (usually), and keeps most wrapping callbacks from
 *              having to implement a parser to separate them.
 * Input: k - the env var name
 *        v - the new value for env var k
 * Output: None
 * Returns: NERR_NOMEM
 */
NEOERR *cgiwrap_putenv (const char *k, const char *v);

/* 
 * Function: cgiwrap_iterenv - iterater for env vars
 * Description: cgiwrap_iterenv allows a program to iterate over all the
 *              environment variables.  This is probably mostly used by
 *              the default debug output.
 * Input: n - variable to return.  This should start at 0 and increment
 *            until you receive a NULL return value.
 * Output: k - a malloc'd copy of the variable name
 *         v - a malloc'd copy of the variable value
 * Returns: NERR_NOMEM
 */
NEOERR *cgiwrap_iterenv (int n, char **k, char **v);

/* 
 * Function: cgiwrap_writef - a wrapper for printf
 * Description: cgiwrap_writef is the formatted output command that
 *              replaces printf or fprintf(stdout) in a standard CGI
 * Input: fmt - standard printf fmt string and args
 * Output: None
 * Returns: NERR_SYSTEM
 */
NEOERR *cgiwrap_writef (const char *fmt, ...)
                        ATTRIBUTE_PRINTF(1,2);

/* 
 * Function: cgiwrap_writevf - a wrapper for vprintf
 * Description: cgiwrap_writevf is the formatted output command that
 *              replaces vprintf or fvprintf(stdout) in a standard CGI
 *              It is also used by cgiwrap_writef (the actual wrapped
 *              function is a v type function)
 * Input: fmt - standard printf fmt string
 *        ap - stdarg argument pointer
 * Output: None
 * Returns: NERR_SYSTEM
 */
NEOERR *cgiwrap_writevf (const char *fmt, va_list ap);

/* 
 * Function: cgiwrap_write - wrapper for the fwrite(stdout)
 * Description: cgiwrap_write is the block data output function for
 *              cgiwrap that replaces fwrite(stdout) in regular CGIs
 * Input: buf - a character buffer
 *        buf_len - the length of the buffer in buf
 * Output: None
 * Returns: NERR_IO
 */
NEOERR *cgiwrap_write (const char *buf, int buf_len);

/* 
 * Function: cgiwrap_read - cgiwrap input function
 * Description: cgiwrap_read is used to read incoming data from the
 *              client, usually from a POST or PUT HTTP request.  It
 *              wraps the part of fread(stdin).
 * Input: buf - a pre-allocated buffer to read the data into
 *        buf_len - the size of the pre-allocated buffer
 * Output: read_len - the number of bytes read into buf
 * Returns: None
 */
void cgiwrap_read (char *buf, int buf_len, int *read_len);

__END_DECLS

#endif /* __CGIWRAP_H_ */
