
/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2002 by Brandon Long
 */

#ifndef __NEO_FILTER_H_
#define __NEO_FILTER_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include <stdio.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"

NEOERR *filter_wait(pid_t pid, int options, int *exitcode);
NEOERR *filter_create_fd(char *cmd, int *fdin, int *fdout, int *fderr, pid_t *pid);
NEOERR *filter_create_fp(char *cmd, FILE **in, FILE **out, FILE **err, pid_t *pid);

__END_DECLS

#endif /* __NEO_FILTER_H_ */
