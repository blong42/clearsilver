/*
 * Copyright (C) 2000 - Neotonic
 */

#ifndef __HTML_H_ 
#define __HTML_H_ 1

#include <stdarg.h>
#include "util/neo_err.h"
#include "util/neo_hdf.h"

NEOERR *convert_text_html_alloc (char *src, int slen, char **out);
NEOERR *html_escape_alloc (char *src, int slen, char **out);

#endif /* __HTML_H_ */
