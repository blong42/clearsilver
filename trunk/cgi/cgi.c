
#include <unistd.h>
#include <string.h>

#include "neo_err.h"
#include "neo_hdf.h"

typedef struct _cgi
{
  void *data;
} CGI;

NEOERR *cgi_parse (CGI *cgi)
{
  char *s;

  s = cgiwrap_getenv ("QUERY_STRING");
  hdf_set_buf (cgi->hdf, "CGI.QUERY_STRING", s);

  return STATUS_OK;
}
