
#include <unistd.h>
#include <limits.h>
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "util/neo_err.h"

int main (int argc, char **argv, char **envp)
{
  NEOERR *err;
  CGI *cgi;
  char *cs_file;
  char hdf_file[_POSIX_PATH_MAX];

  cgi_debug_init (argc,argv);
  cgiwrap_init_std (argc, argv, envp);

  err = cgi_init(&cgi, NULL);
  if (err != STATUS_OK)
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }

  cs_file = hdf_get_value (cgi->hdf, "CGI.PathTranslated", NULL);
  if (cs_file == NULL)
  {
    cgi_error (cgi, "No PATH_TRANSLATED var");
    return -1;
  }
  snprintf (hdf_file, sizeof(hdf_file), "%s.hdf", cs_file);
  err = hdf_read_file (cgi->hdf, hdf_file);
  if (err != STATUS_OK)
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }
  err = cgi_display (cgi, cs_file);
  if (err != STATUS_OK)
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }
  return 0;
}
