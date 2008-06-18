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
#include <errno.h>

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

int parse_template(HDF *hdf, CSPARSE *parse, char *buf, int do_auto)
{
  NEOERR *err;

  err = hdf_set_int_value(hdf, "Config.AutoEscape", do_auto);
  if (err != STATUS_OK) {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  err = cs_parse_string (parse, strdup(buf), strlen(buf)); 
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  return 0;
}

int render_template_check(HDF *hdf, CSPARSE *parse, char *expect)
{
  STRING result;
  NEOERR *err;

  string_init(&result);

  err = cs_render(parse, &result, get_result);
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  if (strcmp(result.buf, expect) != 0)
  {
    printf("Failure!\n");
    printf("Expected: %s \nActual : %s\n", expect, result.buf);
    return -1;
  }
  else
    return 0;
}

int init_template(HDF **phdf, CSPARSE **pparse, char *hdf_string)
{
  NEOERR *err;

  err = hdf_init(phdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = cs_init (pparse, *phdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_read_string(*phdf, hdf_string);
  if (err != STATUS_OK) {
    printf("hdf_set_value() failed");
    exit(-1);
  }

  return 0;
}

/*
 * Testing re-use of CSPARSE object, with various combinations of
 * auto escaped and unescaped templates.
 */
int test_multiple_renders()
{
  HDF *hdf;
  CSPARSE *parse;
  NEOERR *err;

  /* Multiple calls to cs_render.
     The template ends with <script> tag, so the test verifies that the second
     call to cs_render does not inherit context from the first cs_render.
   */
  if (init_template(&hdf, &parse, "One=<script>alert(1);</script>") != 0)
    return -1;

  if (parse_template(hdf, parse, "<?cs var: One ?><script>", 1) != 0)
    return -1;

  if (render_template_check(hdf, parse,
                            "&lt;script&gt;alert(1);&lt;/script&gt;<script>")
      != 0)
    return -1;
  
  if (render_template_check(hdf, parse,
                            "&lt;script&gt;alert(1);&lt;/script&gt;<script>")
      != 0)
    return -1;

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  /* Multiple calls to cs_parse, with different values for Config.AutoEscape.
     The second template will also be escaped, though it sets
     Config.AutoEscape = 0.
  */
  if (init_template(&hdf, &parse, "One=<script>alert(1);</script>") != 0)
    return -1;

  if (parse_template(hdf, parse, "<?cs var: One ?>", 1) != 0) {
    return -1;
  }

  if (parse_template(hdf, parse, "<?cs var: One ?>", 0) != 0) {
    return -1;
  }

  if (parse_template(hdf, parse, "<?cs var: One ?>", 1) != 0) {
    return -1;
  }

  if (render_template_check(hdf, parse, 
                            "&lt;script&gt;alert(1);&lt;/script&gt;"\
                            "&lt;script&gt;alert(1);&lt;/script&gt;"\
                            "&lt;script&gt;alert(1);&lt;/script&gt;")
      != 0) {
    return -1;
  }

  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Sanity check: calls to cs_render and cs_parse, with no autoescaping. */
  if (init_template(&hdf, &parse, "One=<script>alert(1);</script>") != 0)
    return -1;

  if (parse_template(hdf, parse, "<?cs var: One ?><script>", 0) != 0) {
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "<script>alert(1);</script><script>")
      != 0) {
    return -1;
  }

  if (parse_template(hdf, parse, "<?cs var: One ?>", 0) != 0) {
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "<script>alert(1);</script><script>"\
                            "<script>alert(1);</script>")
      != 0) {
    return -1;
  }
  
  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Sanity check: calls with no value for Config.AutoEscape */
  if (init_template(&hdf, &parse, "One=<script>alert(1);</script>") != 0)
    return -1;

  err = cs_parse_string (parse, strdup("<?cs var: One ?><script>"),
                         strlen("<?cs var: One ?><script>")); 
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "<script>alert(1);</script><script>")
      != 0) {
    return -1;
  }

  err = cs_parse_string (parse, strdup("<?cs var: One ?>"),
                         strlen("<?cs var: One ?>")); 
  if (err != STATUS_OK)
  {
    err = nerr_pass(err);
    nerr_log_error(err);
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "<script>alert(1);</script><script>"\
                            "<script>alert(1);</script>")
      != 0) {
    return -1;
  }
  
  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Multiple calls to cs_render, interspersed with calls to cs_parse. */
  if (init_template(&hdf, &parse, "One=<script>alert(1);</script>") != 0)
    return -1;

  if (parse_template(hdf, parse, "<?cs var: One ?><script>", 1) != 0) {
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "&lt;script&gt;alert(1);&lt;/script&gt;<script>")
      != 0) {
    return -1;
  }

  if (parse_template(hdf, parse, "<?cs var: One ?>", 1) != 0) {
    return -1;
  }

  if (render_template_check(hdf, parse,
                            "&lt;script&gt;alert(1);&lt;/script&gt;<script>"\
                            "&lt;script&gt;alert(1);&lt;/script&gt;")
      != 0) {
    return -1;
  }
  
  cs_destroy (&parse);
  hdf_destroy(&hdf);

  return 0;
}

/* Helper function to initialize, parse and render a single template file. */
static int process_template(HDF *hdf, char *cs_file, int enable_auto, int log)
{
  CSPARSE *parse;
  NEOERR *err;

  err = cs_init (&parse, hdf);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    return -1;
  }

  /* Set the flags *after* cs_init and before cs_parse to verify that the 
     auto escape enabled check happens at this stage */
  if (enable_auto) {
    err = hdf_set_int_value(hdf, "Config.AutoEscape", 1);
    if (err != STATUS_OK) {
      printf("hdf_set_value() failed");
      return -1;
    }
  }

  if (log) {
    err = hdf_set_int_value(hdf, "Config.LogAutoEscape", 1);
    if (err != STATUS_OK) {
      printf("hdf_set_int_value() failed");
      return -1;
    }
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

  return 0;
}

/*
 * Create a template file by populating it with the list of statements
 * provided in "cmds".
 */
static int create_template(char *name, char *cmds[], int cmds_len)
{
  FILE *fcs;
  int i;

  fcs = fopen(name, "w");
  if (!fcs)
  {
    perror("create_template");
    return -1;
  }

  for (i = 0; i < cmds_len; i++)
    fprintf(fcs, cmds[i]);

  fclose(fcs);
  return 0;
}

/*
 * Test that auto escaped variables are properly logged.
 * The function redirects stderr to a temporary file to capture log messages.
 * Logs start with a timestamp, which is ignored for the purposes of this test.
 */
static int test_log_message()
{
  HDF *hdf;
  int fd_stderr;
  FILE* ftmp;
  char *tmp;
  int i;

  char *cmd_list[] = {
    "<?cs var: Title ?>\n",
    "<?cs include: 'test_include.cs' ?>\n",
    "<?cs var: Title ?>\n",
    "<?cs linclude: 'test_include.cs' ?>\n",
    "<?cs lvar: csvar ?>\n",
    "<?cs var: Title ?>\n",
  };

  char *msg_list[] = {
    "tmp_auto.cs: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
    "test_include.cs: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
    "tmp_auto.cs: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
    "test_include.cs: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
    "csvar: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
    "tmp_auto.cs: Auto-escape changed variable [ Title ] from [</title><script>alert(1)</script>] to [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
  };
  int cmd_list_len = 6;
  char line[256];
  int len;
  int prefix_len = strlen("[06/15 22:14:26] ");

  /* Temporary CS template */
  if (create_template("tmp_auto.cs", cmd_list, cmd_list_len) != 0)
    return -1;

  hdf_init(&hdf);
  hdf_read_file(hdf, "test.hdf");

  /* 
     Temporarily redirect stderr to a file, 
     so we can validate the messages logged to stderr.
  */
  fd_stderr = dup(fileno(stderr));
  freopen("tmp_auto.out", "w", stderr);

  if (process_template(hdf, "tmp_auto.cs", 1, 1) != 0)
    return -1;

  /* Restore original stderr */
  fclose(stderr);
  stderr = fdopen(fd_stderr, "w");

  hdf_destroy(&hdf);

  /* Now, verify the messages logged by cs_render */
  ftmp = fopen("tmp_auto.out", "r");
  if (!ftmp)
  {
    perror("test_log_message");
    return -1;
  }

  for (i = 0; i < cmd_list_len; i++)
  {
    len = prefix_len + strlen(msg_list[i]) + 1;
    if (fgets(line, len + 1, ftmp) == NULL)
    {
      perror("test_log_message");
      return -1;
    }

    /* Skip past the timestamp part of the log message */
    tmp = line + prefix_len;
    if (strncmp(tmp, msg_list[i], strlen(msg_list[i])) != 0)
    {
      printf("Did not match: %s : %s \n", tmp, msg_list[i]);
      return -1;
    }
  }

  remove("tmp_auto.out");
  remove("tmp_auto.cs");

  return 0;
}

int run_extra_tests()
{
  int retval = test_content_type();

  if (retval != 0)
    return retval;

  retval = test_multiple_renders();
  if (retval != 0)
    return retval;

  retval = test_log_message();
  if (retval != 0)
    return retval;

  return 0;
}

int main (int argc, char *argv[])
{
  NEOERR *err;
  HDF *hdf;
  int html = 0;
  char *hdf_file = NULL;
  char *cs_file = NULL;
  int do_logging = 0;
  int c;
  char ctrl[] = {0x1, 'h', 'i', 0x3, 0x8, 'd', 0x1f, 0x7f, 'e', 0x00};
  int retval;

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
        return run_extra_tests();
    }
  }

  html = 1;

  if (!cs_file || !hdf_file)
  {
    printf("Usage: %s -h <file.hdf> -c <file.cs> [-l] [-t]\n", argv[0]);
    printf("      -h <file.hdf> load hdf file file.hdf\n");
    printf("      -c <file.cs> load cs file file.cs\n");
    printf("      -t run extra tests\n");
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

  retval = process_template(hdf, cs_file, html, do_logging);

  hdf_destroy(&hdf);

  return retval;
}
