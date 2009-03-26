/*
 * Copyright 2008 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

/* cgi_test.c
 * standalone test, to be expanded */

#include "ClearSilver.h"

#include <string.h>


/* Used by test_http_headers, this is the old hard-coded list of environment
 * variables to HDF variables, used to verify the new dynamically generated
 * ones match. */
struct _old_http_vars
{
  char *env_name;
  char *hdf_name;
  char *value;
} HTTPVars[] = {
  {"HTTP_ACCEPT", "Accept", "foo"},
  {"HTTP_ACCEPT_CHARSET", "AcceptCharset", "iso-8859-1, utf8"},
  {"HTTP_ACCEPT_ENCODING", "AcceptEncoding", "gzip"},
  {"HTTP_ACCEPT_LANGUAGE", "AcceptLanguage", "en:q=100; es"},
  {"HTTP_COOKIE", "Cookie", "Foo=bar"},
  {"HTTP_HOST", "Host", "www.fiction.net"},
  {"HTTP_USER_AGENT", "UserAgent", "Mozilla/5.0 (alike)"},
  {"HTTP_IF_MODIFIED_SINCE", "IfModifiedSince", "1234"},
  {"HTTP_REFERER", "Referer", "http://www.google.com/"},
  {"HTTP_VIA", "Via", "proxy:8080"},
  {"HTTP_X_REQUESTED_WITH", "XRequestedWith", "ajax"},
  /* SOAP */
  {"HTTP_SOAPACTION", "Soap.Action", "/wsgi"},
  {NULL, NULL, NULL}
};

NEOERR *test_http_headers() {
  NEOERR *err;
  CGI *cgi;
  char *v;
  char **argv;
  char **envp;
  int x, count;

  argv = (char **) malloc (2 * sizeof(char *));
  argv[0] = strdup("cgi_test");
  argv[1] = NULL;

  count = 0;
  while (HTTPVars[count].env_name) count++;

  envp = (char **) malloc ((count + 1) * sizeof(char *));
  for (x = 0; x < count; x++) {
    envp[x] = sprintf_alloc("%s=%s", HTTPVars[x].env_name, HTTPVars[x].value);
    putenv(envp[x]);
  }
  envp[count] = NULL;

  cgiwrap_init_std(1, argv, envp);

  err = cgi_init(&cgi, NULL);
  if (err) return nerr_pass(err);

  for (x = 0; x < count; x++) {
    v = hdf_get_valuef(cgi->hdf, "HTTP.%s", HTTPVars[x].hdf_name);
    if (v == NULL) {
      hdf_dump(cgi->hdf, NULL);
      return nerr_raise(NERR_ASSERT,
                        "HTTP.%s not found in dataset.", HTTPVars[x].hdf_name);
    }
    if (strcmp(v, HTTPVars[x].value)) {
      hdf_dump(cgi->hdf, "-E- ");
      return nerr_raise(NERR_ASSERT,
                        "HTTP.%s returned wrong value %s, expected %s.",
                        HTTPVars[x].hdf_name,
                        v,
                        HTTPVars[x].value);
    }
  }

  return STATUS_OK;
}

NEOERR *test_content_type_parsing() {
  NEOERR *err;
  CGI *cgi;
  char *v;
  char **argv;
  char **envp;
  int count;

  argv = (char **) malloc (2 * sizeof(char *));
  argv[0] = strdup("cgi_test");
  argv[1] = NULL;

  count = 1;

  envp = (char **) malloc ((count + 1) * sizeof(char *));
  envp[0] = strdup("CONTENT_TYPE="
                   "application/x-www-form-urlencoded; charset=UTF-8");
  putenv(envp[0]);
  envp[count] = NULL;

  cgiwrap_init_std(1, argv, envp);

  err = cgi_init(&cgi, NULL);
  if (err) return nerr_pass(err);

  v = hdf_get_value(cgi->hdf, "CGI.ContentType.Type", NULL);
  if (v == NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type not found in dataset.");
  }
  if (strcmp(v, "application/x-www-form-urlencoded")) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type returned wrong value %s, "
                      "expected %s.",
                      v,
                      "application/x-www-form-urlencoded");
  }
  v = hdf_get_value(cgi->hdf, "CGI.ContentType.charset", NULL);
  if (v == NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.charset not found in dataset.");
  }
  if (strcmp(v, "UTF-8")) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.charset returned wrong value %s, "
                      "expected %s.",
                      v,
                      "UTF-8");
  }

  cgi_destroy(&cgi);

  envp[0] = strdup("CONTENT_TYPE=text/html");
  putenv(envp[0]);

  cgiwrap_init_std(1, argv, envp);

  err = cgi_init(&cgi, NULL);
  if (err) return nerr_pass(err);

  v = hdf_get_value(cgi->hdf, "CGI.ContentType.Type", NULL);
  if (v == NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type not found in dataset.");
  }
  if (strcmp(v, "text/html")) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type returned wrong value %s, "
                      "expected %s.",
                      v,
                      "application/x-www-form-urlencoded");
  }
  v = hdf_get_value(cgi->hdf, "CGI.ContentType.charset", NULL);
  if (v != NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.charset found in dataset.");
  }

  cgi_destroy(&cgi);

  envp[0] = strdup("CONTENT_TYPE= text/html; boundary=\"qwerty 1234\"");
  putenv(envp[0]);

  cgiwrap_init_std(1, argv, envp);

  err = cgi_init(&cgi, NULL);
  if (err) return nerr_pass(err);

  v = hdf_get_value(cgi->hdf, "CGI.ContentType.Type", NULL);
  if (v == NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type not found in dataset.");
  }
  if (strcmp(v, "text/html")) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.Type returned wrong value %s, "
                      "expected %s.",
                      v,
                      "application/x-www-form-urlencoded");
  }
  v = hdf_get_value(cgi->hdf, "CGI.ContentType.boundary", NULL);
  if (v == NULL) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.boundary not found in dataset.");
  }
  if (strcmp(v, "qwerty 1234")) {
    hdf_dump(cgi->hdf, "-E- ");
    return nerr_raise(NERR_ASSERT,
                      "CGI.ContentType.boundary returned wrong value %s, "
                      "expected %s.",
                      v,
                      "qwerty 1234");
  }

  cgi_destroy(&cgi);

  return STATUS_OK;
}

int main(int argc, char **argv, char **envp) {
  NEOERR *err;

  err = test_http_headers();
  if (err) {
    nerr_log_error(err);
    return -1;
  }
  err = test_content_type_parsing();
  if (err) {
    nerr_log_error(err);
    return -1;
  }

  return 0;
}
