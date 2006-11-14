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

#ifndef __NEO_FILTER_H_
#define __NEO_FILTER_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include <stdio.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"

/*
 * Function: filter_wait - wrap waitpid to decode the exitcode and why
 *           your filter quit
 * Description: filter_wait wraps the waitpid call and raises an error
 *              (with description) if the call failed.  Note that if the
 *              ask for the exitcode and the process exited with a code
 *              other than zero, we don't raise an error.  If you don't
 *              ask for the exitcode, and it is non-zero, we raise an
 *              error
 * Input: pid -> the process identifier to wait for
 *        options -> the options to pass to waitpid (see wait(2))
 * Output: exitcode -> the exitcode if the process existed normally
 * Returns: NERR_SYSTEM, NERR_ASSERT
 */
NEOERR *filter_wait(pid_t pid, int options, int *exitcode);

/*
 * Function: filter_create_fd - Create a sub process and return the
 * requested pipes
 * Description: filter_create_fd and filter_create_fp are what popen
 *              should have been: a mechanism to create sub processes
 *              and have pipes to all their input/output.  The concept
 *              was taken from mutt, though python has something similar
 *              with popen3/popen4.  You control which pipes the
 *              function returns by the fdin/fdout/fderr arguments.  A
 *              NULL value means "don't create a pipe", a pointer to an
 *              int will cause the pipes to be created and the value
 *              of the file descriptor stored in the int.  You will have
 *              to close(2) the file descriptors yourself.
 * Input: cmd -> the sub command to execute.  Will be executed with
 *               /bin/sh -c
 *        fdin -> pointer to return the stdin pipe, or NULL if you don't
 *                want the stdin pipe
 *        fdout -> pointer to return the stdout pipe, or NULL if you don't
 *                 want the stdout pipe
 *        fderr -> pointer to return the stderr pipe, or NULL if you don't
 *                 want the stderr pipe
 * Output: fdin -> the stdin file descriptor of the sub process
 *         fdout -> the stdout file descriptor of the sub process
 *         fderr -> the stderr file descriptor of the sub process
 *         pid -> the pid of the sub process
 * Returns: NERR_SYSTEM
 */
NEOERR *filter_create_fd(const char *cmd, int *fdin, int *fdout, int *fderr, 
                         pid_t *pid);

/*
 * Function: filter_create_fp - similar to filter_create_fd except with
 *           buffered FILE* 
 * Description: filter_create_fp is identical to filter_create_fd,
 *              except each of the pipes is wrapped in a buffered stdio FILE 
 * Input: cmd -> the sub command to execute.  Will be executed with
 *               /bin/sh -c
 *        in -> pointer to return the stdin pipe, or NULL if you don't
 *              want the stdin pipe
 *        out -> pointer to return the stdout pipe, or NULL if you don't
 *               want the stdout pipe
 *        err -> pointer to return the stderr pipe, or NULL if you don't
 *                 want the stderr pipe
 * Output: in -> the stdin FILE of the sub process
 *         out -> the stdout FILE of the sub process
 *         err -> the stderr FILE of the sub process
 *         pid -> the pid of the sub process
 * Returns: NERR_SYSTEM, NERR_IO
 */
NEOERR *filter_create_fp(const char *cmd, FILE **in, FILE **out, FILE **err, 
                         pid_t *pid);

__END_DECLS

#endif /* __NEO_FILTER_H_ */
