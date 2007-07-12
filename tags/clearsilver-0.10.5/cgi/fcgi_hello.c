/**
 * Copyright 2006 Mike Tsao. All rights reserved.
 *
 * Hello World using FastCGI and ClearSilver.
 */

#include "ClearSilver.h"
#include <string>
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>

static bool quit = false;

static int cs_printf(void *ctx, const char *s, va_list args) {
  return printf(s, args);
}

static int cs_write(void *ctx, const char *s, int n) {
  return fwrite(const_cast<char *>(s), n, 1, FCGI_stdout);
}

int main(int argc, char **argv, char **envp) {
  openlog(argv[0], 0, LOG_USER);
  syslog(LOG_INFO, "%s started.", argv[0]); 

  int hits = 0;
  while (FCGI_Accept() >= 0) {
    HDF *hdf = NULL;
    CGI *cgi = NULL;

    /* Note that we aren't doing any error handling here, we really should. */
    hdf_init(&hdf);

    // Takes ownership of HDF.
    cgi_init(&cgi, hdf);

    hits++;

    /* Initialize the standard cgiwrap environment.  FastCGI already wraps some
     * of the standard calls that cgiwrap wraps.  */
    cgiwrap_init_std(argc, argv, environ);

    /* Then, we install our own wrappers for some cgiwrap calls that aren't
     * already wrapped in the standard wrappers. */
    cgiwrap_init_emu(NULL, NULL, cs_printf, cs_write, NULL, NULL, NULL);

    hdf_read_file(cgi->hdf, "common.hdf");
    hdf_read_file(cgi->hdf, "hello_world.hdf");

    cgi_display(cgi, "hello_world.cs");

    // This destroys HDF.
    cgi_destroy(&cgi);
  }
  syslog(LOG_INFO, "%s ending.", argv[0]); 
  return 0;
}
