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

/* static.cgi
 * This is a really simple example of how you can map URL requests to a set of
 * hdf and cs files.
 */

#include "ClearSilver.h"

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv, char **envp)
{
  NEOERR *err;
  CGI *cgi;
  char *cs_file;
  char hdf_file[_POSIX_PATH_MAX];
  char *p;

  /* CGI works by passing information from the server to the CGI program via
   * environment variables and stdin.  cgi_debug_init looks for a file as the
   * first argument, and loads it.  That file contains key=value pairs which
   * cgi_debug_init will load into the environment, allowing you to test your
   * program via the command line. */
  cgi_debug_init(argc, argv);

  /* The ClearSilver cgi toolkit accesses the CGI environment through a
   * wrapper.  This allows the program to be used in other environments and
   * fake the CGI environment, such as FastCGI, mod_python, PyApache, or even
   * just from Python to access the python objects instead of the libc API.
   * cgiwrap_init_std just sets up for the default CGI environment using the
   * libc api. */
  cgiwrap_init_std(argc, argv, envp);

  /* cgi_init creates a CGI struct, and parses the CGI environment variables. 
   * It creates an HDF structure as well.  */
  err = cgi_init(&cgi, NULL);
  if (err != STATUS_OK)
  {
    /* cgi_neo_error renders a NEOERR as an error CGI result */
    cgi_neo_error(cgi, err);
    /* nerr_log_error logs the error to stderr and cleans up */
    nerr_log_error(err);
    return -1;
  }

  /* CGI.PathTranslated is a CGI env var which maps the URL with the
   * DocumentRoot to give you the location of the referenced file on disk */
  cs_file = hdf_get_value(cgi->hdf, "CGI.PathTranslated", NULL);
  if (cs_file == NULL)
  {
    /* cgi_error returns a simple error page */
    cgi_error(cgi, "No PATH_TRANSLATED var");
    return -1;
  }

  /* The hdf.loadpaths variables specify where HDF and ClearSilver look for
   * files on the file system.  We start setting that up here based on
   * the directory of the file referenced */
  p = strrchr (cs_file, '/');
  if (p)
  {
    *p = '\0';
    err = hdf_set_value(cgi->hdf, "hdf.loadpaths.0", cs_file);
    chdir(cs_file);
    *p = '/';
    if (err)
    {
      cgi_neo_error(cgi, err);
      nerr_log_error(err);
      return -1;
    }
  }
  /* Next, we look for a shared HDF static dataset in common.hdf */
  err = hdf_read_file(cgi->hdf, "common.hdf");
  if (err && !nerr_handle(&err, NERR_NOT_FOUND))
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }
  /* Next, we look for an HDF file for this specific page.  We first look
   * for passedfile.html.hdf, then we check for a file by removing an extension
   * from the file, so something like passedfile.html we'll look for
   * passedfile.hdf */
  snprintf (hdf_file, sizeof(hdf_file), "%s.hdf", cs_file);
  err = hdf_read_file (cgi->hdf, hdf_file);
  if (err && !nerr_handle(&err, NERR_NOT_FOUND))
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }
  p = strrchr (cs_file, '.');
  if (p)
  {
    *p = '\0';
    snprintf (hdf_file, sizeof(hdf_file), "%s.hdf", cs_file);
    *p = '.';
    err = hdf_read_file (cgi->hdf, hdf_file);
    if (err && !nerr_handle(&err, NERR_NOT_FOUND))
    {
      cgi_neo_error(cgi, err);
      nerr_log_error(err);
      return -1;
    }
  }
  /* Lastly, we need to render a template.  The template is either the
   * file that was passed to us, or its specificed by CGI.StaticContent
   * in one of the HDF files we loaded above. */
  cs_file = hdf_get_value (cgi->hdf, "CGI.StaticContent", cs_file);
  err = cgi_display (cgi, cs_file);
  if (err != STATUS_OK)
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    return -1;
  }
  return 0;
}
