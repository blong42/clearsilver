/*
 * Copyright (C) 2000 - Neotonic
 */

#ifndef __CGI_H_ 
#define __CGI_H_ 1

#include "util/neo_err.h"
#include "util/neo_hdf.h"

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

#endif /* __CGI_H_ */
