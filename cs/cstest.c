

#include <stdio.h>
#include "cs.h"
#include "neo_misc.h"
#include "neo_hdf.h"

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

  if (argc < 3)
  {
    ne_warn ("Usage: cstest <file.hdf> <file.cs>");
    return -1;
  }

  err = hdf_init(&hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }
  err = hdf_read_file(hdf, argv[1]);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  ne_warn ("Parsing %s", argv[2]);
  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_parse_file (parse, argv[2]);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  err = cs_dump(parse);

  err = cs_render(parse, NULL, output);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  cs_destroy (&parse);

  return 0;
}
