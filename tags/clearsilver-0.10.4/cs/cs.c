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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cs.h"
#include "util/neo_misc.h"
#include "util/neo_hdf.h"

static NEOERR *output (void *ctx, char *s)
{
  printf ("%s", s);
  return STATUS_OK;
}

int main (int argc, char *argv[])
{
  NEOERR *err;
  CSPARSE *parse;
  HDF *hdf;
  int verbose = 0;
  char *hdf_file, *cs_file;
  char c;

  extern char *optarg;

  err = hdf_init(&hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_init (&parse, hdf);
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }

  while ((c = getopt(argc, argv, "Hvh:c:")) != EOF )

    switch (c) {
    case 'h':
      hdf_file=optarg;
      err = hdf_read_file(hdf, hdf_file);
      if (err != STATUS_OK) {
	nerr_log_error(err);
	return -1;
      }
      break;
    case 'c':
      cs_file=optarg;
      if ( verbose )
	printf ("Parsing %s\n", cs_file);

      err = cs_parse_file (parse, cs_file);
      if (err != STATUS_OK) {
	err = nerr_pass(err);
	nerr_log_error(err);
	return -1;
      }
      break;
    case 'v':
      verbose=1;
      break;
    case 'H':
      fprintf(stderr, "Usage: %s [-v] [-h <file.hdf>] [-c <file.cs>]\n", argv[0]);
      fprintf(stderr, "     -h <file.hdf> load hdf file file.hdf (multiple allowed)\n");
      fprintf(stderr, "     -c <file.cs>  load cs file file.cs (multiple allowed)\n");
      fprintf(stderr, "     -v            verbose output\n");
      return -1;
      break;
    }


  err = cs_render(parse, NULL, output);
  if (err != STATUS_OK) {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  if (verbose) {
    printf ("\n-----------------------\nCS DUMP\n");
    err = cs_dump(parse, NULL, output);
  }

  cs_destroy (&parse);

  if (verbose) {
    printf ("\n-----------------------\nHDF DUMP\n");
    hdf_dump (hdf, NULL);
  }

  return 0;
}
