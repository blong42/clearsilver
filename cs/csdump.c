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

#include "cs_config.h"
#include <stdio.h>
#include "cs.h"
#include "util/neo_misc.h"
#include "util/neo_hdf.h"

/*
static NEOERR *output (void *ctx, char *s)
{
  printf ("%s", s);
  return STATUS_OK;
}
*/

int main (int argc, char *argv[])
{
  NEOERR *err;
  CSPARSE *parse;
  HDF *hdf;

  if (argc < 3)
  {
    ne_warn ("Usage: csdump <file.cs> <output.c>");
    return -1;
  }

  err = hdf_init(&hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  ne_warn ("Parsing %s", argv[1]);
  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_parse_file (parse, argv[1]);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  err = cs_dump_c(parse, argv[2]);

  cs_destroy (&parse);

  return 0;
}
