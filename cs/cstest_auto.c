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

static char *g_input_path = "";

static char *get_tmp_path()
{
  char *path = getenv("TEST_TMPDIR");
  if (path == NULL) {
    path = "/tmp";
  }
  return path;
}

static int ends_with(char *str, char *ends)
{
  int ends_len;
  int str_len;

  ends_len = strlen(ends);
  str_len = strlen(str);

  if (ends_len > str_len) return 0;
  if (strcmp(str + (str_len - ends_len), ends) == 0) return 1;
  return 0;
}

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

NEOERR *test_content_type()
{
  char *html_content_types[] =
    {"text/html", "text/plain"};
  char *js_content_types[] =
    {"application/javascript", "text/javascript"};
  char *json_content_types[] =
    {"application/json"};
  char *css_content_types[] =
    {"text/css"};

  char css_var[] = "\"scriptalert1script\"";
  char var[] = "<script>alert(1)</script>";
  char html_var[] = "\"&lt;script&gt;alert(1)&lt;/script&gt;\"";
  char js_var[] = "\"\\x3Cscript\\x3Ealert(1)\\x3C\\x2Fscript\\x3E\"";
  char json_var[] = "\"\\u003Cscript\\u003Ealert(1)\\u003C\\/script\\u003E\"";
  char body[] = "\"<?cs var: BadVar ?>\"";

  struct _test_cases test_cases[4];
  int num_cases = 0;

  NEOERR *err;
  HDF *hdf = NULL;
  char *result;
  int equal = 0;
  int num_errors = 0;
  int i, j;

  test_cases[0].content_types = html_content_types;
  test_cases[0].num_types = sizeof(html_content_types)/sizeof(char*);
  test_cases[0].text = body;
  test_cases[0].result = html_var;

  test_cases[1].content_types = js_content_types;
  test_cases[1].num_types = sizeof(js_content_types)/sizeof(char*);
  test_cases[1].text = body;
  test_cases[1].result = js_var;

  test_cases[2].content_types = json_content_types;
  test_cases[2].num_types = sizeof(json_content_types)/sizeof(char*);
  test_cases[2].text = body;
  test_cases[2].result = json_var;

  test_cases[3].content_types = css_content_types;
  test_cases[3].num_types = sizeof(css_content_types)/sizeof(char*);
  test_cases[3].text = body;
  test_cases[3].result = css_var;
  num_cases = 4;

  err = hdf_init(&hdf);
  if (err) return nerr_pass(err);

  err = hdf_set_int_value(hdf, "Config.AutoEscape", 1);
  if (err) return nerr_pass(err);

  err = hdf_set_value(hdf, "BadVar", var);
  if (err) return nerr_pass(err);

  for (i = 0; i < num_cases; i++)
  {
    for (j = 0; j < test_cases[i].num_types; j++)
    {
      result = test_single(test_cases[i].content_types[j],
                           hdf, test_cases[i].text);


      equal = (strcmp(result, test_cases[i].result) == 0);

      if (!equal)
      {
        printf("Match failed!! %s != %s\n", result, test_cases[i].result);
        num_errors++;
      }

      if (result)
        free(result);
    }
  }

  if (hdf)
    hdf_destroy(&hdf);

  if (num_errors) {
    return nerr_raise(NERR_ASSERT, "test_content_type found %d errors",
                      num_errors);
  }
  return STATUS_OK;
}

NEOERR *parse_template(HDF *hdf, CSPARSE *parse, const char *buf, int do_auto)
{
  NEOERR *err;

  err = hdf_set_int_value(hdf, "Config.AutoEscape", do_auto);
  if (err) return nerr_pass(err);

  err = cs_parse_string (parse, strdup(buf), strlen(buf));
  return nerr_pass(err);
}

NEOERR *render_template_check(HDF *hdf, CSPARSE *parse, const char *expect)
{
  STRING result;
  NEOERR *err;

  string_init(&result);

  err = cs_render(parse, &result, get_result);
  if (err) return nerr_pass(err);

  if (strcmp(result.buf, expect) != 0)
  {
    err = nerr_raise(NERR_ASSERT,
                     "Failure!\n" "Expected: %s \nActual : %s\n", expect,
                     result.buf);
  }
  string_clear(&result);
  return nerr_pass(err);
}

NEOERR *init_template(HDF **phdf, CSPARSE **pparse, const char *hdf_string)
{
  NEOERR *err;

  err = hdf_init(phdf);
  if (err) return nerr_pass(err);

  err = cs_init (pparse, *phdf);
  if (err) return nerr_pass(err);

  err = hdf_read_string(*phdf, hdf_string);
  return nerr_pass(err);
}

/*
 * Testing re-use of CSPARSE object, with various combinations of
 * auto escaped and unescaped templates.
 */
NEOERR *test_multiple_renders()
{
  HDF *hdf = NULL;
  CSPARSE *parse = NULL;
  NEOERR *err;

  /* Multiple calls to cs_render.
     The template ends with <script> tag, so the test verifies that the second
     call to cs_render does not inherit context from the first cs_render.
   */
  err = init_template(&hdf, &parse, "One=<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?><script>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                            "&lt;script&gt;alert(1);&lt;/script&gt;<script>");
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "&lt;script&gt;alert(1);&lt;/script&gt;<script>");
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  /* Multiple calls to cs_parse, with different values for Config.AutoEscape.
     The second template will also be escaped, though it sets
     Config.AutoEscape = 0.
  */
  err = init_template(&hdf, &parse, "One=<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?>", 1);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?>", 0);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "&lt;script&gt;alert(1);&lt;/script&gt;"\
                              "&lt;script&gt;alert(1);&lt;/script&gt;"\
                              "&lt;script&gt;alert(1);&lt;/script&gt;");
  if (err) return nerr_pass(err);

  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Sanity check: calls to cs_render and cs_parse, with no autoescaping. */
  err = init_template(&hdf, &parse, "One=<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?><script>", 0);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse, "<script>alert(1);</script><script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?>", 0);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "<script>alert(1);</script><script>"\
                              "<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Sanity check: calls with no value for Config.AutoEscape */
  err = init_template(&hdf, &parse, "One=<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  err = cs_parse_string (parse, strdup("<?cs var: One ?><script>"),
                         strlen("<?cs var: One ?><script>"));
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "<script>alert(1);</script><script>");
  if (err) return nerr_pass(err);

  err = cs_parse_string (parse, strdup("<?cs var: One ?>"),
                         strlen("<?cs var: One ?>"));
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "<script>alert(1);</script><script>"\
                              "<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  cs_destroy (&parse);
  hdf_destroy(&hdf);

  /* Multiple calls to cs_render, interspersed with calls to cs_parse. */
  err = init_template(&hdf, &parse, "One=<script>alert(1);</script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?><script>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "&lt;script&gt;alert(1);&lt;/script&gt;<script>");
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var: One ?>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "&lt;script&gt;alert(1);&lt;/script&gt;<script>"\
                              "null");
  if (err) return nerr_pass(err);

  cs_destroy (&parse);
  hdf_destroy(&hdf);

  return STATUS_OK;
}

/* Helper function to initialize, parse and render a single template file. */
NEOERR *process_template(HDF *hdf, char *cs_file, int enable_auto, int log)
{
  CSPARSE *parse;
  NEOERR *err;

  err = cs_init (&parse, hdf);
  if (err) return nerr_pass(err);

  do {
    /* Set the flags *after* cs_init and before cs_parse to verify that the
       auto escape enabled check happens at this stage */
    if (enable_auto) {
      err = hdf_set_int_value(hdf, "Config.AutoEscape", 1);
      if (err) break;
    }

    if (log) {
      err = hdf_set_int_value(hdf, "Config.LogAutoEscape", 1);
      if (err) break;
    }

    err = cs_parse_file (parse, cs_file);
    if (err) break;

    err = cs_render(parse, NULL, output);
    if (err) break;
  } while (0);

  cs_destroy (&parse);

  return nerr_pass(err);
}

/*
 * Create a template file by populating it with the list of statements
 * provided in "cmds".
 */
NEOERR *create_template(char *name, char *cmds[], int cmds_len)
{
  FILE *fcs;
  int i;

  fcs = fopen(name, "w");
  if (!fcs)
  {
    return nerr_raise_errno(NERR_IO, "Unable to create file %s", name);
  }

  for (i = 0; i < cmds_len; i++)
    fprintf(fcs, "%s", cmds[i]);

  fclose(fcs);
  return STATUS_OK;
}

/*
 * Test that auto escaped variables are properly logged.
 * The function redirects stderr to a temporary file to capture log messages.
 * Logs start with a timestamp, which is ignored for the purposes of this test.
 */
NEOERR *test_log_message()
{
  NEOERR *err;
  HDF *hdf;
  int fd_stderr;
  FILE* ftmp;
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
    "tmp_auto.cs]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",

    "test_include.cs]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",

    "tmp_auto.cs]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",

    "test_include.cs]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",

    "csvar]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",

    "tmp_auto.cs]: Auto-escape changed variable [ Title ] from"\
    " [</title><script>alert(1)</script>] to"\
    " [&lt;/title&gt;&lt;script&gt;alert(1)&lt;/script&gt;]",
  };
  int cmd_list_len = 6;
  char line[256];
  char tmp_auto_cs[PATH_MAX];
  char tmp_auto_out[PATH_MAX];

  /* Temporary CS template */
  snprintf(tmp_auto_cs, sizeof(tmp_auto_cs), "%s/tmp_auto.cs", get_tmp_path());
  snprintf(tmp_auto_out, sizeof(tmp_auto_out), "%s/tmp_auto.out",
           get_tmp_path());
  err = create_template(tmp_auto_cs, cmd_list, cmd_list_len);
  if (err) return nerr_pass(err);

  err = hdf_init(&hdf);
  if (err) return nerr_pass(err);
  if (g_input_path) {
    err = hdf_set_value(hdf, "hdf.loadpaths.0", g_input_path);
    if (err) return nerr_pass(err);
  }
  err = hdf_read_file(hdf, "test.hdf");
  if (err) return nerr_pass(err);

  /*
     Temporarily redirect stderr to a file,
     so we can validate the messages logged to stderr.
  */
  fd_stderr = dup(fileno(stderr));
  if (freopen(tmp_auto_out, "w", stderr) == NULL) {
    return nerr_raise_errno(NERR_IO, "Unable to freopen stderr");
  }

  err = process_template(hdf, tmp_auto_cs, 1, 1);

  /* Restore original stderr */
  fclose(stderr);
  stderr = fdopen(fd_stderr, "w");

  if (err) return nerr_pass(err);

  hdf_destroy(&hdf);

  /* Now, verify the messages logged by cs_render */
  ftmp = fopen(tmp_auto_out, "r");
  if (!ftmp)
  {
    return nerr_raise_errno(NERR_IO, "Unable to fopen %s", tmp_auto_out);
  }

  for (i = 0; i < cmd_list_len; i++)
  {
    if (fgets(line, sizeof(line), ftmp) == NULL)
    {
      return nerr_raise_errno(NERR_IO, "Unable to fgets");
    }

    if (ends_with(line, msg_list[i]))
    {
      return nerr_raise(NERR_ASSERT, "Did not match: %s : %s \n", line,
                        msg_list[i]);
    }
  }

  remove(tmp_auto_out);
  remove(tmp_auto_cs);

  return STATUS_OK;
}

NEOERR *normal_func(CSPARSE *parse, CS_FUNCTION *csf, CSARG *args,
                    CSARG *result)
{
  result->op_type = CS_TYPE_STRING;
  result->n = 0;
  result->alloc = 1;
  result->s = strdup("<normal>");
  return STATUS_OK;
}

NEOERR *another_func(CSPARSE *parse, CS_FUNCTION *csf, CSARG *args,
                    CSARG *result)
{
  result->op_type = CS_TYPE_STRING;
  result->n = 0;
  result->alloc = 1;
  result->s = strdup("<another>");
  return STATUS_OK;
}

NEOERR *escape_func(CSPARSE *parse, CS_FUNCTION *csf, CSARG *args,
                    CSARG *result)
{
  result->op_type = CS_TYPE_STRING;
  result->n = 0;
  result->alloc = 1;
  result->s = strdup("<script>");
  return STATUS_OK;
}

NEOERR *test_register_esc()
{
  CSPARSE *parse;
  NEOERR *err;
  HDF *hdf;

  err = init_template(&hdf, &parse, "");
  if (err) return nerr_pass(err);

  err = cs_register_esc_function(parse, "escape_func", 1, escape_func);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var:escape_func(some_var) ?>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse, "<script>");
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  err = init_template(&hdf, &parse, "");
  if (err) return nerr_pass(err);

  err = cs_register_esc_function(parse, "escape_func", 1, escape_func);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var:escape_func(some_var) ?>", 0);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse, "<script>");
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  err = init_template(&hdf, &parse, "");
  if (err) return nerr_pass(err);

  err = cs_register_function(parse, "normal_func", 1, normal_func);
  if (err) return nerr_pass(err);
  err = cs_register_esc_function(parse, "escape_func", 1, escape_func);
  if (err) return nerr_pass(err);
  err = cs_register_function(parse, "another_func", 1, another_func);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var:normal_func(some_var) ?>"\
                       "<?cs var:escape_func(some_var) ?>"\
                       "<?cs var:another_func(some_var) ?>", 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse,
                              "&lt;normal&gt;<script>&lt;another&gt;");
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  err = init_template(&hdf, &parse, "");
  if (err) return nerr_pass(err);

  err = cs_register_function(parse, "normal_func", 1, normal_func);
  if (err) return nerr_pass(err);
  err = cs_register_esc_function(parse, "escape_func", 1, escape_func);
  if (err) return nerr_pass(err);
  err = cs_register_function(parse, "another_func", 1, another_func);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, "<?cs var:normal_func(some_var) ?>"\
                       "<?cs var:escape_func(some_var) ?>"\
                       "<?cs var:another_func(some_var) ?>", 0);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse, "<normal><script><another>");
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  return STATUS_OK;
}

NEOERR *check_output(const char *hdf_str, const char *template_str,
                     const char *expect)
{
  HDF *hdf;
  CSPARSE *parse;
  NEOERR *err;

  err = init_template(&hdf, &parse, hdf_str);
  if (err) return nerr_pass(err);

  err = cs_register_function(parse, "normal_func", 1, normal_func);
  if (err) return nerr_pass(err);

  err = cs_register_esc_function(parse, "escape_func", 1, escape_func);
  if (err) return nerr_pass(err);

  err = parse_template(hdf, parse, template_str, 1);
  if (err) return nerr_pass(err);

  err = render_template_check(hdf, parse, expect);
  if (err) return nerr_pass(err);

  cs_destroy(&parse);
  hdf_destroy(&hdf);

  return STATUS_OK;
}

/* Test that Config.PropagateEscapeStatus works properly with 'set' calls */
NEOERR *test_set_vars()
{
  NEOERR *err;
  const char *hdf_str =
      "x=<script>\n y=<b>\n a.1=<b>\n a.2=<p>\n Config.PropagateEscapeStatus=1";

  /* One without setting the flag, to verify normal behaviour */
  err = check_output("",
                     "<?cs set: One = '<script>' ?><?cs var: One ?>",
                     "&lt;script&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: One = '<script>' ?><?cs var: One ?>",
                     "<script>");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: One = '<script>' + '<b>' ?><?cs var: One ?>",
                     "<script><b>");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: var1 = x ?><?cs var: var1 ?>",
                     "&lt;script&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: var1 = a[1] ?><?cs var: var1 ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: var1 = a.1 ?><?cs var: var1 ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: var1 = normal_func(x) ?><?cs var: var1 ?>",
                     "&lt;normal&gt;");
  if (err) return nerr_pass(err);

  /* Set child of an existing global variable, it should *not* be escaped */
  err = check_output(hdf_str,
                     "<?cs set: x.1 = '<p>' ?><?cs var: x.1 ?><?cs var:x[1] ?>",
                     "<p><p>");
  if (err) return nerr_pass(err);

  /* Set child of non existing global variable, it should *not* be escaped */
  err = check_output(hdf_str,
                     "<?cs set: z.1 = '<p>' ?><?cs var: z.1 ?><?cs var:z[1] ?>",
                     "<p><p>");
  if (err) return nerr_pass(err);

  /* escape_func() is labelled as an escaping function, so its output
     should *not* be escaped */
  err = check_output(hdf_str,
                     "<?cs set: var1 = escape_func(x) ?><?cs var: var1 ?>",
                     "<script>");
  if (err) return nerr_pass(err);

  /* concatenation of two escape_func()s should be treated as trusted */
  err = check_output(hdf_str,
                     "<?cs set:var1 = escape_func(x) + escape_func(x)?>"\
                     "<?cs var:var1 ?>",
                     "<script><script>");
  if (err) return nerr_pass(err);

  /* concatenation of escape_func() and constant should be treated as trusted */
  err = check_output(hdf_str,
                     "<?cs set:var1 = escape_func(x) + '<p>'?><?cs var:var1 ?>",
                     "<script><p>");
  if (err) return nerr_pass(err);

  /* Mixed content, the whole string *should* be escaped */
  err = check_output(hdf_str,
                     "<?cs set: var1 = '<b>' + x ?><?cs var: var1 ?>",
                     "&lt;b&gt;&lt;script&gt;");
  if (err) return nerr_pass(err);

  /* Mixed content again, *should* be escaped */
  err = check_output(hdf_str,
                     "<?cs set: var1 = x + '<b>' ?><?cs var: var1 ?>",
                     "&lt;script&gt;&lt;b&gt;");
  if (err) return nerr_pass(err);

  /* concatenation of escape_func() and variable: mixed content */
  err = check_output(hdf_str,
                     "<?cs set:var1 = escape_func(x) + x ?><?cs var:var1 ?>",
                     "&lt;script&gt;&lt;script&gt;");
  if (err) return nerr_pass(err);

  /* mixed content with a function */
  err = check_output(hdf_str,
                     "<?cs set:var1 = normal_func(x) + '<p>'?><?cs var:var1 ?>",
                     "&lt;normal&gt;&lt;p&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: var1 = x + y ?><?cs var: var1 ?>",
                     "&lt;script&gt;&lt;b&gt;");
  if (err) return nerr_pass(err);

  /* Verify that the "trusted" attribute is passed up through multiple "set"s */
  err = check_output(hdf_str,
                     "<?cs set: var1 = '<b>' ?><?cs var: var1 ?> "\
                     "<?cs set: var2 = var1 ?><?cs var: var2 ?>",
                     "<b> <b>");
  if (err) return nerr_pass(err);

  /* Verify that the "trusted" attribute is passed through multiple "set"s
     and concatenations.
  */
  err = check_output(hdf_str,
                     "<?cs set: var1 = '<b>' ?><?cs var: var1 ?> "\
                     "<?cs set: var2 = var1 + '<p>' ?><?cs var: var2 ?>",
                     "<b> <b><p>");
  if (err) return nerr_pass(err);

  /* Modify variable using "set" multiple times. The "trusted" status
     should get propagated across all the commands.
  */
  err = check_output(hdf_str,
                     "<?cs set: var1 = '<b>' ?>"\
                     "<?cs set: var1 = var1 + '<p>' ?><?cs var: var1 ?>",
                     "<b><p>");
  if (err) return nerr_pass(err);

  /* Modify variable from untrusted to trusted */
  err = check_output(hdf_str,
                     "<?cs set: var1 = x ?><?cs var: var1 ?>"\
                     "<?cs set: var1 = '<p>' ?> <?cs var: var1 ?>",
                     "&lt;script&gt; <p>");
  if (err) return nerr_pass(err);

  return STATUS_OK;
}

/* Test that Config.PropagateEscapeStatus works properly
   with local variables created by 'call', 'each' and 'with'.
*/
NEOERR *test_local_vars()
{
  NEOERR *err;
  const char* hdf_str =
      "x=<b>\n a.1=<b>\n a.2=<p>\n Config.PropagateEscapeStatus=1";

  err = check_output("",
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac('<b>') ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac(x) ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac('<b>') ?>",
                     "<b>");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs set: y = '<b>' ?><?cs call:mac(y) ?>",
                     "<b>");
  if (err) return nerr_pass(err);

  /* Some tests for doing 'set' inside a macro */
  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs set: p = x ?>"\
                     "<?cs var: p ?><?cs /def ?>"          \
                     "<?cs call:mac('<l>') ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs set: p = '<b>' ?>"\
                     "<?cs var: p ?><?cs /def ?>"              \
                     "<?cs call:mac(x) ?>",
                     "<b>");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs set: p = '<b>' ?>"\
                     "<?cs var: p ?><?cs /def ?>"              \
                     "<?cs call:mac(1) ?>",
                     "<b>");
  if (err) return nerr_pass(err);

  /* The macro arguments are call by reference, so changing the value of
     argument changes the original HDF variable. Ensure that the escaping
     status gets updated accordingly */
  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?>"\
                     "<?cs set: p = x ?><?cs var: p ?> <?cs /def ?>"\
                     "<?cs set: y = '<p>' ?><?cs var: y ?> "\
                     "<?cs call:mac(y) ?><?cs var: y ?>",
                     "<p> &lt;b&gt; &lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs each: y = a ?><?cs var: y ?> <?cs /each ?>",
                     "&lt;b&gt; &lt;p&gt; ");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: b.1 = '<b>' ?><?cs set: b.2 = '<p>' ?>"\
                     "<?cs each: y = b ?><?cs var: y ?> <?cs /each ?>",
                     "<b> <p> ");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs with: y = a ?><?cs var: y.1 ?><?cs /with ?>",
                     "&lt;b&gt;");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: b.1 = '<b>' ?><?cs set: b.2 = '<p>' ?>"\
                     "<?cs with: y = b ?><?cs var: y.1 ?><?cs /with ?>",
                     "<b>");
  if (err) return nerr_pass(err);

  /* escape function - should not be escaped */
  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac(escape_func(x)) ?>",
                     "<script>");
  if (err) return nerr_pass(err);

  /* concatenation of escape_func() and constant should be treated as trusted */
  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac(escape_func(x) + '<p>') ?>",
                     "<script><p>");
  if (err) return nerr_pass(err);

  /* concatenation of escape_func() and variable: mixed content */
  err = check_output(hdf_str,
                     "<?cs def:mac(p) ?><?cs var: p ?><?cs /def ?>"\
                     "<?cs call:mac(escape_func(x) + x) ?>",
                     "&lt;script&gt;&lt;b&gt;");
  if (err) return nerr_pass(err);

  return STATUS_OK;
}

/* Test that Config.PropagateEscapeStatus gets properly
   inherited with evar and lvar commands.
*/
NEOERR *test_inherit_flag()
{
  NEOERR *err;
  const char* hdf_str =
      "my_lvar=<?cs var: One ?>\n Config.PropagateEscapeStatus=1";

  err = check_output(hdf_str,
                     "<?cs set: One = '<script>' ?><?cs evar: my_lvar ?>",
                     "<script>");
  if (err) return nerr_pass(err);

  err = check_output(hdf_str,
                     "<?cs set: One = '<script>' ?><?cs lvar: my_lvar ?>",
                     "<script>");
  return nerr_pass(err);
}

NEOERR *test_propagate_escape_status()
{
  NEOERR *err;

  err = test_set_vars();
  if (err) return nerr_pass(err);

  err = test_local_vars();
  if (err) return nerr_pass(err);

  err = test_inherit_flag();
  return nerr_pass(err);
}

NEOERR *run_extra_tests()
{
  NEOERR* err;

  err = test_content_type();
  if (err) return nerr_pass(err);

  err = test_multiple_renders();
  if (err) return nerr_pass(err);

  err = test_log_message();
  if (err) return nerr_pass(err);

  err = test_register_esc();
  if (err) return nerr_pass(err);

  err = test_propagate_escape_status();
  return nerr_pass(err);
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
  char nonascii[] = {0xe2, 0x82, 0xac, 0x00};
  int do_extra_tests = 0;

  while ((c = getopt(argc, argv, "i:h:c:lt")) != EOF ) {
    switch (c) {
      case 'i':
        g_input_path=optarg;
        break;
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
        do_extra_tests = 1;
        break;
    }
  }

  if (do_extra_tests) {
    err = run_extra_tests();
    if (err) {
      nerr_log_error(err);
      nerr_ignore(&err);
      return -1;
    }
    return 0;
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
  err = hdf_set_value(hdf, "SpaceAttr", "hello\nwo\vrld\tto\r you");
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_value(hdf, "CtrlAttr", ctrl);
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_value(hdf, "NonAscii", nonascii);
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_value(hdf, "CtrlUrl", "java\x1fscript:alert(1)");
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }
  
  /* TODO(mugdha): Not testing with html_escape() etc, those functions are
     defined in cgi. Figure out a way to incorporate these tests.
  */
  printf ("Parsing %s\n", cs_file);

  err = process_template(hdf, cs_file, html, do_logging);
  if (err != STATUS_OK) {
    nerr_log_error(err);
    return -1;
  }

  hdf_destroy(&hdf);

  return 0;
}
