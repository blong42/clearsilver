/*
 * Copyright (C) 2000 - Neotonic
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <zlib.h>

#include "cgi/cgiwrap.h"
#include "util/neo_err.h"
#include "util/neo_hdf.h"
#include "util/neo_misc.h"
#include "util/neo_str.h"
#include "cgi/cgi.h"
#include "cs/cs.h"

struct _cgi_vars
{
  char *env_name;
  char *hdf_name;
} CGIVars[] = {
  {"DOCUMENT_ROOT", "DocumentRoot"},
  {"GATEWAY_INTERFACE", "GatewayInterface"},
  {"PATH_INFO", "PathInfo"},
  {"PATH_TRANSLATED", "PathTranslated"},
  {"QUERY_STRING", "QueryString"},
  {"REMOTE_ADDR", "RemoteAddress"},
  {"REMOTE_PORT", "RemotePort"},
  {"REQUEST_METHOD", "RequestMethod"},
  {"REQUEST_URI", "RequestURI"},
  {"SCRIPT_FILENAME", "ScriptFilename"},
  {"SCRIPT_NAME", "ScriptName"},
  {"SERVER_ADMIN", "ServerAdmin"},
  {"SERVER_NAME", "ServerName"},
  {"SERVER_PORT", "ServerPort"},
  {"SERVER_PROTOCOL", "ServerProtocol"},
  {"SERVER_SOFTWARE", "ServerSoftware"},
  {NULL, NULL}
};

struct _http_vars
{
  char *env_name;
  char *hdf_name;
} HTTPVars[] = {
  {"HTTP_ACCEPT", "Accept"},
  {"HTTP_ACCEPT_CHARSET", "AcceptCharset"},
  {"HTTP_ACCEPT_ENCODING", "AcceptEncoding"},
  {"HTTP_ACCEPT_LANGUAGE", "AcceptLanguage"},
  {"HTTP_COOKIE", "Cookie"},
  {"HTTP_HOST", "Host"},
  {"HTTP_USER_AGENT", "UserAgent"},
  {NULL, NULL}
};

static NEOERR *_add_cgi_env_var (CGI *cgi, char *env, char *name)
{
  NEOERR *err;
  char *s;

  err = cgiwrap_getenv (env, &s);
  if (err != STATUS_OK) return nerr_pass (err);
  err = hdf_set_buf (cgi->hdf, name, s);
  if (err != STATUS_OK) 
  {
    free(s);
    return nerr_pass (err);
  }
  return STATUS_OK;
}

static char *url_decode (char *s)
{
  int i = 0, o = 0;

  while (s[i])
  {
    if (s[i] == '+')
    {
      s[o++] = ' ';
      i++;
    }
    else if (s[i] == '%' && isxdigit(s[i+1]) && isxdigit(s[i+2]))
    {
      char num;
      num = (s[i+1] >= 'A') ? ((s[i+1] & 0xdf) - 'A') + 10 : (s[i+1] - '0');
      num *= 16;
      num += (s[i+1] >= 'A') ? ((s[i+1] & 0xdf) - 'A') + 10 : (s[i+1] - '0');
      s[o++] = num;
      i+=3;
    }
    else {
      s[o++] = s[i++];
    }
  }
  s[o] = '\0';
  return s;
}

static NEOERR *_parse_query (CGI *cgi)
{
  NEOERR *err = STATUS_OK;
  char *s, *t, *k, *v;
  char buf[256];
  HDF *obj, *child;

  err = hdf_get_copy (cgi->hdf, "CGI.QueryString", &s, NULL);
  if (err != STATUS_OK) return nerr_pass (err);
  if (s)
  {
    k = strtok(s, "=");
    while (k)
    {
      v = strtok(NULL, "&");
      snprintf(buf, sizeof(buf), "Query.%s", url_decode(k));
      url_decode(v);
      obj = hdf_get_obj (cgi->hdf, buf);
      if (obj != NULL)
      {
	int i = 0;
	char buf2[10];
	child = hdf_obj_child (obj);
	if (child == NULL)
	{
	  t = hdf_obj_value (obj);
	  err = hdf_set_value (obj, "0", t);
	  if (err != STATUS_OK) break;
	  i = 1;
	}
	else
	{
	  while (child != NULL)
	  {
	    i++;
	    child = hdf_obj_next (child);
	    if (err != STATUS_OK) break;
	  }
	  if (err != STATUS_OK) break;
	}
	snprintf (buf2, sizeof(buf2), "%d", i);
	err = hdf_set_value (obj, buf2, v);
	if (err != STATUS_OK) break;
      }
      err = hdf_set_value (cgi->hdf, buf, v);
      if (err != STATUS_OK) break;
      k = strtok(NULL, "=");
    }
    free(s);
  }
  return nerr_pass(err);
}

NEOERR *cgi_parse (CGI *cgi)
{
  NEOERR *err;
  int x = 0;
  char buf[256];

  while (CGIVars[x].env_name)
  {
    snprintf (buf, sizeof(buf), "CGI.%s", CGIVars[x].hdf_name);
    err = _add_cgi_env_var(cgi, CGIVars[x].env_name, buf);
    if (err != STATUS_OK) return nerr_pass (err);
    x++;
  }
  x = 0;
  while (HTTPVars[x].env_name)
  {
    snprintf (buf, sizeof(buf), "HTTP.%s", HTTPVars[x].hdf_name);
    err = _add_cgi_env_var(cgi, HTTPVars[x].env_name, buf);
    if (err != STATUS_OK) return nerr_pass (err);
    x++;
  }
  err = _parse_query(cgi);
  if (err != STATUS_OK) return nerr_pass (err);

  return STATUS_OK;
}

NEOERR *cgi_init (CGI **cgi, char *hdf_file)
{
  NEOERR *err = STATUS_OK;
  CGI *mycgi;

  *cgi = NULL;
  mycgi = (CGI *) calloc (1, sizeof(CGI));
  if (mycgi == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate space for CGI");

  do 
  {
    err = hdf_init (&(mycgi->hdf));
    if (err != STATUS_OK) break;
    err = cgi_parse (mycgi);
    if (err != STATUS_OK) break;
    if (hdf_file != NULL)
    {
      err = hdf_read_file (mycgi->hdf, hdf_file);
      if (err != STATUS_OK) break;
    }

    mycgi->time_start = ne_timef();
  } while (0);

  if (err == STATUS_OK)
    *cgi = mycgi;
  else
  {
    cgi_destroy(&mycgi);
  }
  return nerr_pass(err);
}

void cgi_destroy (CGI **cgi)
{
  if ((*cgi)->hdf)
    hdf_destroy (&((*cgi)->hdf));
  free (*cgi);
  *cgi = NULL;
}

static NEOERR *render_cb (void *ctx, char *buf)
{
  STRING *str= (STRING *)ctx;

  return nerr_pass(string_append(str, buf));
}

static NEOERR *cgi_headers (CGI *cgi)
{
  NEOERR *err = STATUS_OK;
  HDF *obj, *child;
  char *s;

  obj = hdf_get_obj (cgi->hdf, "cgiout");
  if (obj)
  {
    s = hdf_get_value (obj, "Status", NULL);
    if (s)
      err = cgiwrap_writef ("Status: %s\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
    s = hdf_get_value (obj, "Location", NULL);
    if (s)
      err = cgiwrap_writef ("Location: %s\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
    child = hdf_get_obj (cgi->hdf, "cgiout.other");
    if (child)
    {
      child = hdf_obj_child (child);
      while (child != NULL)
      {
	s = hdf_obj_value (child);
	err = cgiwrap_writef ("%s\n", s);
	if (err != STATUS_OK) return nerr_pass (err);
	child = hdf_obj_next(child);
      }
    }
    s = hdf_get_value (obj, "ContentType", "text/html");
    err = cgiwrap_writef ("Content-Type: %s\n\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
  }
  else
  {
    /* Default */
    err = cgiwrap_writef ("Content-Type: text/html\n\n");
    if (err != STATUS_OK) return nerr_pass (err);
  }
  return STATUS_OK;
}

/* Copy these here from zutil.h, which we aren't supposed to include */
#define DEF_MEM_LEVEL 8
#define OS_CODE 0x03

static NEOERR *cgi_compress (STRING *str, char *obuf, int *olen)
{
  z_stream stream;
  int err;

  stream.next_in = (Bytef*)str->buf;
  stream.avail_in = (uInt)str->len;
  stream.next_out = obuf;
  stream.avail_out = (uInt)*olen;
  if ((uLong)stream.avail_out != *olen) 
    return nerr_raise(NERR_NOMEM, "Destination too big: %ld", *olen);

  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;

  /* err = deflateInit(&stream, Z_DEFAULT_COMPRESSION); */
  err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (err != Z_OK) 
    return nerr_raise(NERR_SYSTEM, "deflateInit2 returned %d", err);

  err = deflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    deflateEnd(&stream);
    return nerr_raise(NERR_SYSTEM, "deflate returned %d", err);
  }
  *olen = stream.total_out;

  err = deflateEnd(&stream);
  return STATUS_OK;
}

static NEOERR *cgi_output (CGI *cgi, STRING *str)
{
  NEOERR *err = STATUS_OK;
  double dis;
  int is_html = 0;
  int use_deflate = 0;
  int use_gzip = 0;
  char *s, *e;

  dis = ne_timef();
  if (err != STATUS_OK) return nerr_pass (err);
  s = hdf_get_value (cgi->hdf, "cgiout.ContentType", "text/html");
  if (!strcasecmp(s, "text/html"))
    is_html = 1;

  /* Determine whether or not we can compress the output */
  if (is_html)
  {
    err = hdf_get_copy (cgi->hdf, "HTTP.AcceptEncoding", &s, NULL);
    if (err != STATUS_OK) return nerr_pass (err);
    if (s)
    {

      e = strtok (s, ",");
      while (e && !use_deflate)
      {
	if (strstr(e, "deflate") != NULL)
	{
	  use_deflate = 1;
	  use_gzip = 0;
	}
	else if (strstr(e, "gzip") != NULL)
	  use_gzip = 1;
	e = strtok (NULL, ",");
      }
      free (s);
    }
    s = hdf_get_value (cgi->hdf, "HTTP.UserAgent", NULL);
    if (s)
    {
      if (strstr(s, "MSIE 4") || strstr(s, "MSIE 5"))
      {
	e = hdf_get_value (cgi->hdf, "HTTP.Accept", NULL);
	if (e && !strcmp(e, "*/*"))
	{
	  use_deflate = 0;
	  use_gzip = 0;
	}
      }
      else
      {
	if (strncasecmp(s, "mozilla/5.", 10))
	{
	  use_deflate = 0;
	  use_gzip = 0;
	}
      }
    }
    else
    {
      use_deflate = 0;
      use_gzip = 0;
    }
    if (use_deflate)
    {
      err = hdf_set_value (cgi->hdf, "cgiout.other.encoding", 
	  "Content-Encoding: deflate");
    }
    else if (use_gzip)
    {
      err = hdf_set_value (cgi->hdf, "cgiout.other.encoding", 
	  "Content-Encoding: gzip");
    }
    if (err != STATUS_OK) return nerr_pass(err);
  }

  err = cgi_headers(cgi);
  if (err != STATUS_OK) return nerr_pass(err);

  if (is_html)
  {
    char buf[50];
    snprintf (buf, sizeof(buf), "\n<!-- %5.3f:%d -->\n",
	dis - cgi->time_start, use_deflate || use_gzip);
    err = string_append (str, buf);
    if (err != STATUS_OK) return nerr_pass(err);
  }

  if (is_html && (use_deflate || use_gzip))
  {
    char *dest;
    static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
    unsigned long crc = 0;
    int len2;

    if (use_gzip)
    {
      crc = crc32(0L, Z_NULL, 0);
      crc = crc32(crc, str->buf, str->len);
    }
    len2 = str->len * 2;
    dest = (char *) malloc (sizeof(char) * len2);
    if (dest != NULL)
    {
      err = cgi_compress (str, dest, &len2);
      if (err == STATUS_OK)
      {
	if (use_gzip)
	{
	  err = cgiwrap_writef("%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
	      Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE);
	}
	err = cgiwrap_write(dest, len2);

	if (use_gzip)
	{
	  err = cgiwrap_writef("%c%c%c%c", (0xff & (crc >> 0)), (0xff & (crc >> 8)), (0xff & (crc >> 16)), (0xff & (crc >> 24)));
	  err = cgiwrap_writef("%c%c%c%c", (0xff & (str->len >> 0)), (0xff & (str->len >> 8)), (0xff & (str->len >> 16)), (0xff & (str->len >> 24)));
	}
      }
      else
      {
	nerr_log_error (err);
	err = cgiwrap_write(str->buf, str->len);
      }
      free (dest);
    }
    else
    {
      err = cgiwrap_write(str->buf, str->len);
    }
  }
  else
  {
    err = cgiwrap_write(str->buf, str->len);
  }

  return err;
}

NEOERR *cgi_display (CGI *cgi, char *cs_file)
{
  NEOERR *err = STATUS_OK;
  char *s;
  CSPARSE *cs = NULL;
  STRING str;

  string_init(&str);

  s = hdf_get_value (cgi->hdf, "cgidata.debug", NULL);

  do
  {
    err = cs_init (&cs, cgi->hdf);
    if (err != STATUS_OK) break;
    err = cs_parse_file (cs, cs_file);
    if (err != STATUS_OK) break;
    err = cs_render (cs, &str, render_cb);
    if (err != STATUS_OK) break;
    err = cgi_output(cgi, &str);
    if (err != STATUS_OK) break;
  } while (0);

  cs_destroy(&cs);
  string_clear (&str);
  return nerr_pass(err);
}

void cgi_neo_error (CGI *cgi, NEOERR *err)
{
  STRING str;

  string_init(&str);
  cgiwrap_writef("Status: 500\n");
  cgiwrap_writef("Content-Type: text/html\n\n");

  cgiwrap_writef("<html><body>\nAn error occured:<pre>");
  nerr_error_string (err, &str);
  cgiwrap_write(str.buf, str.len);
  cgiwrap_writef("</pre></body></html>\n");
}

void cgi_error (CGI *cgi, char *fmt, ...)
{
  va_list ap;

  cgiwrap_writef("Status: 500\n");
  cgiwrap_writef("Content-Type: text/html\n\n");
  cgiwrap_writef("<html><body>\nAn error occured:<pre>");
  va_start (ap, fmt);
  cgiwrap_writevf (fmt, ap);
  va_end (ap);
  cgiwrap_writef("</pre></body></html>\n");
}

void cgi_debug_init (int argc, char **argv)
{
  FILE *fp;
  char line[256];
  char *k, *v;

  if (argc)
  {
    fp = fopen(argv[1], "r");
    if (fp == NULL)
      return;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      v = strchr(line, '=');
      if (v != NULL)
      {
	*v = '\0';
	v = neos_strip(v+1);
	cgiwrap_putenv (line, v);
      }
    }
    fclose(fp);
  }
}
