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
#include <string.h>
#include <ctype.h>
#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "cs.h"

static NEOERR *output (void *ctx, char *s)
{
  printf ("%s", s);
  return STATUS_OK;
}

NEOERR *test_strfunc(const char *str, char **ret)
{
  char *s = strdup(str);
  int x = 0;

  if (s == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to duplicate string in test_strfunc");

  while (s[x]) {
    s[x] = tolower(s[x]);
    x++;
  }
  *ret = s;
  return STATUS_OK;
}

void usage(char *argv0)
{
  ne_warn("Usage: %s [-v] [-parse_must_fail] [-global_hdf <file.hdf>] "
          "<file.hdf> <file.cs>", argv0);
}

int hdf_init_load_file_or_err(HDF **hdf, char *filename)
{
  NEOERR *err;

  err = hdf_init(hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }
  err = hdf_read_file(*hdf, filename);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    hdf_destroy(hdf);
    return -1;
  }
  return 0;
}

int safe_strcmp(char *a, char *b)
{
  if (a == NULL && b != NULL) return 1;
  if (a != NULL && b == NULL) return -1;
  if (a == b) return 0;
  return strcmp(a, b);
}

int hdf_compare_node(HDF *hdf_a, HDF *hdf_b)
{
  if ((hdf_a->link != hdf_b->link) ||
      (hdf_a->alloc_value != hdf_b->alloc_value) ||
      (hdf_a->name_len != hdf_b->name_len) ||
      (safe_strcmp(hdf_a->name, hdf_b->name)) ||
      (safe_strcmp(hdf_a->value, hdf_b->value)))
  {
    return -1;
  }
  return 0;
}

int hdf_compare(HDF *hdf_a, HDF *hdf_b)
{
  while (hdf_a != NULL && hdf_b != NULL) {
    if (hdf_compare_node(hdf_a, hdf_b) == -1)
    {
      ne_warn("HDF nodes don't match!\n\t%s=%s\n\t%s=%s",
              hdf_a->name, hdf_a->value, hdf_b->name, hdf_b->value);
      return -1;
    }
    if (hdf_a->child != NULL && hdf_b->child == NULL) {
      ne_warn("HDF child doesn't match! %s child? %d vs %s child? %d",
              hdf_a->name, hdf_a->child ? 1 : 0,
              hdf_b->name, hdf_b->child ? 1 : 0);
      return -1;
    }
    if (hdf_a->child == NULL && hdf_b->child != NULL) {
      ne_warn("HDF child doesn't match! %s child? %d vs %s child? %d",
              hdf_a->name, hdf_a->child ? 1 : 0,
              hdf_b->name, hdf_b->child ? 1 : 0);
      return -1;
    }
    if (hdf_a->child)
    {
      int r = hdf_compare(hdf_a->child, hdf_b->child);
      if (r == -1) {
        ne_warn("HDF trees don't match!\n\t%s=%s\n\t%s=%s",
                hdf_a->name, hdf_a->value, hdf_b->name, hdf_b->value);
        return -1;
      }
    }
    if (hdf_a->next != NULL && hdf_b->next == NULL) {
      ne_warn("HDF next doesn't match! %s next? %d vs %s next? %d",
              hdf_a->name, hdf_a->next ? 1 : 0,
              hdf_b->name, hdf_b->next ? 1 : 0);
      return -1;
    }
    if (hdf_a->next == NULL && hdf_b->next != NULL) {
      ne_warn("HDF next doesn't match! %s next? %d vs %s next? %d",
              hdf_a->name, hdf_a->next ? 1 : 0,
              hdf_b->name, hdf_b->next ? 1 : 0);
      return -1;
    }
    hdf_a = hdf_a->next;
    hdf_b = hdf_b->next;
  }

  if (hdf_a == NULL && hdf_b != NULL) {
    ne_warn("HDF nodes don't match! NULL vs %s", hdf_b->name);
    return -1;
  }
  if (hdf_a != NULL && hdf_b == NULL) {
    ne_warn("HDF nodes don't match! %s vs NULL", hdf_a->name);
    return -1;
  }
  return 0;
}

int main (int argc, char *argv[])
{
  NEOERR *err;
  CSPARSE *parse;
  HDF *global_hdf = NULL;
  HDF *hdf;
  int verbose = 0;
  int parse_must_fail = 0;
  char *global_hdf_file = NULL;
  char *hdf_file, *cs_file;
  int arg_position = 1;

  while(arg_position < argc) {
    if (!strcmp(argv[arg_position], "-v"))
    {
      verbose = 1;
    }
    else if (!strcmp(argv[arg_position], "-parse_must_fail"))
    {
      parse_must_fail = 1;
    }
    else if (!strcmp(argv[arg_position], "-global_hdf"))
    {
      if (++arg_position >= argc) {
        usage(argv[0]);
        return -1;
      }
      global_hdf_file = argv[arg_position];
    }
    else
    {
      break;
    }
    arg_position++;
  }

  if (arg_position + 1 >= argc)
  {
    return -1;
  }

  hdf_file = argv[arg_position];
  cs_file = argv[arg_position + 1];

  if (hdf_init_load_file_or_err(&hdf, hdf_file) != 0)
  {
    return -1;
  }

  if (global_hdf_file)
  {
    if (hdf_init_load_file_or_err(&global_hdf, global_hdf_file) != 0)
    {
      return -1;
    }
  }

  printf ("Parsing %s\n", cs_file);
  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }
  parse->global_hdf = global_hdf;

  /* register a test strfunc */
  err = cs_register_strfunc(parse, "test_strfunc", test_strfunc);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_parse_file (parse, cs_file);
  if (err != STATUS_OK)
  {
    if ( !parse_must_fail)
    {
      err = nerr_pass(err);
      nerr_log_error(err);
      return -1;
    }
    else
    {
      return 0;
    }
  }

  err = cs_render(parse, NULL, output);
  if (err != STATUS_OK)
  {
    if ( !parse_must_fail)
    {
      err = nerr_pass(err);
      nerr_log_error(err);
      return -1;
    }
    else
    {
      return 0;
    }
  }

  if (verbose)
  {
    printf ("\n-----------------------\nCS DUMP\n");
    err = cs_dump(parse, NULL, output);
  }

  cs_destroy (&parse);

  if (verbose)
  {
    printf ("\n-----------------------\nHDF DUMP\n");
    hdf_dump (hdf, NULL);
    if (global_hdf)
    {
      printf ("\n-----------------------\nGLOBAL HDF DUMP\n");
      hdf_dump (global_hdf, NULL);
    }
  }
  hdf_destroy(&hdf);

  if (global_hdf)
  {
    /* validate that the global hdf didn't change */
    HDF *global_hdf_again;

    if (hdf_init_load_file_or_err(&global_hdf_again, global_hdf_file) != 0)
    {
      return -1;
    }
    if (hdf_compare(global_hdf, global_hdf_again) != 0)
    {
      ne_warn("HDF trees don't match");
      return -1;
    }
    hdf_destroy(&global_hdf_again);
  }
  hdf_destroy(&global_hdf);

  if (parse_must_fail)
  {
    return -1;
  }
  return 0;
}
