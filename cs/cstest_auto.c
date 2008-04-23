/*
 * Copyright 2007
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
#include <unistd.h>
#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "cs.h"

static NEOERR *output (void *ctx, char *s)
{
  printf ("%s", s);
  return STATUS_OK;
}

static NEOERR *get_result (void *ctx, char *s)
{
  if (ctx)
  {
    string_append((STRING*)ctx, s);
  }

  return STATUS_OK;
}

char* test_single(char *content_type, HDF *hdf, char *body)
{
  STRING my_result;
  char cmd_start[] = "<?cs content-type: \"";
  char cmd_end[] = "\" ?>";
  NEOERR *err;
  CSPARSE *parse;
  char *buf;

  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return NULL;
  }

  /* cs_parse_string modifies input buffer */
  buf = (char*) malloc(strlen(cmd_start) + strlen(content_type) + 
                       strlen(cmd_end) + strlen(body) + 1);
  if (!buf)
  {
    return NULL;
  }
  sprintf(buf, "%s%s%s%s", cmd_start, content_type, cmd_end, body);

  err = cs_parse_string (parse, buf, strlen(buf)); 
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return NULL;
  }
  
  string_init(&my_result);
  err = cs_render(parse, &my_result, get_result);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return NULL;
  }
  
  cs_destroy (&parse);

  return my_result.buf;
}

struct _test_cases {
  char **content_types;
  int num_types;
  char *text;
  char *result;
};

int test_content_type()
{
  char *html_content_types[] =
    {"text/html", "text/xml", "text/plain", "application/xhtml+xml"};
  char *js_content_types[] =
    {"application/javascript", "application/json", "text/javascript"};

  /* Note: css types not supported yet
  char *css_content_types[] =
    {"text/css"};
  char css_var[] = "scriptalert1script";
  */

  char var[] = "<script>alert(1)</script>";
  char html_var[] = "\"&lt;script&gt;alert(1)&lt;/script&gt;\"";
  char js_var[] = "\"\\x3Cscript\\x3Ealert(1)\\x3C\\x2Fscript\\x3E\"";
  char body[] = "\"<?cs var: BadVar ?>\"";

  struct _test_cases test_cases[3];
  int num_cases = 0;

  NEOERR *err;
  HDF *hdf;
  char *result;
  int equal = 0;
  int i, j;

  test_cases[0].content_types = html_content_types;
  test_cases[0].num_types = sizeof(html_content_types)/sizeof(char*);
  test_cases[0].text = body;
  test_cases[0].result = html_var;

  test_cases[1].content_types = js_content_types;
  test_cases[1].num_types = sizeof(js_content_types)/sizeof(char*);
  test_cases[1].text = body;
  test_cases[1].result = js_var;
  num_cases = 2;


  err = hdf_init(&hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_int_value(hdf, "Config.AutoEscape", 1);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_value(hdf, "BadVar", var);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  for (i = 0; i < num_cases; i++)
    for (j = 0; j < test_cases[i].num_types; j++)
    {
      result = test_single(test_cases[i].content_types[j], 
                           hdf, test_cases[i].text); 
      
      equal = (strcmp(result, test_cases[i].result) == 0);
        
      if (result)
        free(result);
      
      if (!equal) 
      {
        printf("Match failed!! %s != %s\n", result, test_cases[i].result);
        return -1;
      }
    }

  hdf_destroy(&hdf);

  return 0;
}

int main (int argc, char *argv[])
{
  NEOERR *err;
  CSPARSE *parse;
  HDF *hdf;
  int html = 0;
  char *hdf_file = NULL;
  char *cs_file = NULL;
  int do_logging = 0;
  int c;
  char ctrl[] = {0x1, 'h', 'i', 0x3, 0x8, 'd', 0x1f, 0x7f, 'e', 0x00};

  while ((c = getopt(argc, argv, "h:c:lt")) != EOF ) {
    switch (c) {
      case 'h':
        hdf_file=optarg;
        break;
      case 'c':
        cs_file=optarg;
        break;
      case 'l':
        do_logging = 1;
        break;
      case 't':
        return test_content_type();
    }
  }

  html = 1;

  if (!cs_file || !hdf_file)
  {
    printf("Usage: %s -h <file.hdf> -c <file.cs> [-l] [-t]\n", argv[0]);
    printf("      -h <file.hdf> load hdf file file.hdf\n");
    printf("      -c <file.cs> load cs file file.cs\n");
    printf("      -t run content type tests\n");
    printf("      -l log auto escaped variables\n");
    return -1;
  }

  err = hdf_init(&hdf);

  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }
  err = hdf_read_file(hdf, hdf_file);

  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  if (html) {
    err = hdf_set_int_value(hdf, "Config.AutoEscape", 1);
    if (err != STATUS_OK) {
      printf("hdf_set_value() failed");
      exit(1);
    }
  }

  if (do_logging) {
    err = hdf_set_int_value(hdf, "Config.LogAutoEscape", 1);
    if (err != STATUS_OK) {
      printf("hdf_set_int_value() failed");
      exit(1);
    }
  }

  /* Setting these 2 variables here, since the control characters 
     cannot be set inside hdf file.
  */
  err = hdf_set_value(hdf, "SpaceAttr", "hello\nworld\tto\r you");
  if (err != STATUS_OK) {
    printf("hdf_set_value() failed");
    exit(1);
  }
  
  err = hdf_set_value(hdf, "CtrlAttr", ctrl);
  if (err != STATUS_OK) {
    printf("hdf_set_value() failed");
    exit(1);
  }
  

  /* TODO(mugdha): Not testing with html_escape() etc, those functions are
     defined in cgi. Figure out a way to incorporate these tests.
  */
  printf ("Parsing %s\n", cs_file);
  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_parse_file (parse, cs_file); 
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  err = cs_render(parse, NULL, output);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  cs_destroy (&parse);

  hdf_destroy(&hdf);

  return 0;
}
