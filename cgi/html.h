/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __HTML_H_ 
#define __HTML_H_ 1

#include <stdarg.h>
#include "util/neo_err.h"
#include "util/neo_hdf.h"

__BEGIN_DECLS

typedef struct _text_html_opts {
    char *bounce_url;
    char *url_class;
    char *url_target;
    char *mailto_class;
    int long_lines;
    int space_convert;
    int newlines_convert;
    int longline_width;
    int check_ascii_art;
    char *link_name;
} HTML_CONVERT_OPTS;

NEOERR *convert_text_html_alloc (unsigned char *src, int slen, unsigned char **out);
NEOERR *convert_text_html_alloc_options (unsigned char *src, int slen, unsigned char **out, HTML_CONVERT_OPTS *opts);
NEOERR *html_escape_alloc (unsigned char *src, int slen, unsigned char **out);
NEOERR *html_strip_alloc(unsigned char *src, int slen, unsigned char **out);

__END_DECLS

#endif /* __HTML_H_ */
