/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the FSF's
 * Library Gnu Public License (LGPL).
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __CGI_H_ 
#define __CGI_H_ 1

#include <stdarg.h>
#include "util/neo_err.h"
#include "util/neo_hdf.h"

extern int CGIFinished;

typedef struct _cgi
{
  double time_start;
  double time_end;
  void *data;
  HDF *hdf;
} CGI;

NEOERR *cgi_init (CGI **cgi, char *hdf_file);
void cgi_destroy (CGI **cgi);
NEOERR *cgi_parse (CGI *cgi);
NEOERR *cgi_display (CGI *cgi, char *cs_file);
void cgi_neo_error (CGI *cgi, NEOERR *err);
void cgi_error (CGI *cgi, char *fmt, ...);
void cgi_debug_init (int argc, char **argv);
NEOERR *cgi_url_escape (char *buf, char **esc);
void cgi_redirect (CGI *cgi, char *fmt, ...);
void cgi_redirect_uri (CGI *cgi, char *fmt, ...);
void cgi_vredirect (CGI *cgi, int uri, char *fmt, va_list ap);

char *cgi_cookie_authority (CGI *cgi, char *host);
NEOERR *cgi_cookie_set (CGI *cgi, char *name, char *value, char *path, 
    char *domain, char *time_str, int persistant);
NEOERR *cgi_cookie_clear (CGI *cgi, char *name, char *domain, char *path);

#endif /* __CGI_H_ */
