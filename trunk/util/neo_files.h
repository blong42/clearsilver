/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_FILES_H_
#define __NEO_FILES_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include <sys/types.h>
#include "util/ulist.h"



typedef int (* MATCH_FUNC)(void *rock, char *filename);

NEOERR *ne_mkdirs (char *path, mode_t mode);
NEOERR *ne_load_file (char *path, char **str);
NEOERR *ne_load_file_len (char *path, char **str, int *len);
NEOERR *ne_save_file (char *path, char *str);
NEOERR *ne_remove_dir (char *path);
NEOERR *ne_listdir(char *path, ULIST **files);
NEOERR *ne_listdir_match(char *path, ULIST **files, char *match);
NEOERR *ne_listdir_fmatch(char *path, ULIST **files, MATCH_FUNC fmatch, void *rock);

__END_DECLS

#endif /* __NEO_FILES_H_ */
