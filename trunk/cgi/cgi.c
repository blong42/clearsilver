/*
 * Copyright (C) 2000 - Neotonic
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <zlib.h>
#include <stdarg.h>
#include <time.h>

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
  {"CONTENT_TYPE", "ContentType"},
  {"CONTENT_LENGTH", "ContentLength"},
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
  {"HTTP_IF_MODIFIED_SINCE", "IfModifiedSince"},
  {"HTTP_REFERER", "Referer"},
  {NULL, NULL}
};

static char *Argv0 = "";

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

  if (s == NULL) return s;
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
      num += (s[i+2] >= 'A') ? ((s[i+2] & 0xdf) - 'A') + 10 : (s[i+2] - '0');
      s[o++] = num;
      i+=3;
    }
    else {
      s[o++] = s[i++];
    }
  }
  if (i && o) s[o] = '\0';
  return s;
}

NEOERR *cgi_url_escape (char *buf, char **esc)
{
  int nl = 0;
  int l = 0;
  char *s;

  while (buf[l])
  {
    if (buf[l] == '/' || buf[l] == '+' || buf[l] == '=' || buf[l] == '&' || 
	buf[l] == '"' ||
	buf[l] < 32 || buf[l] > 122)
    {
      nl += 2;
    }
    nl++;
    l++;
  }

  s = (char *) malloc (sizeof(char) * (nl + 1));
  if (s == NULL) 
    return nerr_raise (NERR_NOMEM, "Unable to allocate memory to escape %s", 
	buf);

  nl = 0; l = 0;
  while (buf[l])
  {
    if (buf[l] == ' ')
    {
      s[nl++] = '+';
      l++;
    }
    else
    if (buf[l] == '/' || buf[l] == '+' || buf[l] == '=' || buf[l] == '&' || 
	buf[l] == '"' ||
	buf[l] < 32 || buf[l] > 122)
    {
      s[nl++] = '%';
      s[nl++] = "0123456789ABCDEF"[buf[l] / 16];
      s[nl++] = "0123456789ABCDEF"[buf[l] % 16];
      l++;
    }
    else
    {
      s[nl++] = buf[l++];
    }
  }
  s[nl] = '\0';

  *esc = s;
  return STATUS_OK;
}

static NEOERR *_parse_query (CGI *cgi, char *query)
{
  NEOERR *err = STATUS_OK;
  char *t, *k, *v, *l;
  char buf[256];
  HDF *obj, *child;

  if (query)
  {
    k = strtok_r(query, "=", &l);
    while (k)
    {
      if (*l == '&')
      {
	l++;
	v = "";
      }
      else
      {
	v = strtok_r(NULL, "&", &l);
      }
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
      k = strtok_r(NULL, "=", &l);
    }
  }
  return nerr_pass(err);
}

/* Is it an error if its a short read? */
static NEOERR *_parse_post_form (CGI *cgi)
{
  NEOERR *err = STATUS_OK;
  char *l, *query;
  int len, r;

  l = hdf_get_value (cgi->hdf, "CGI.ContentLength", NULL);
  if (l == NULL) return STATUS_OK;
  len = atoi (l);
  query = (char *) malloc (sizeof(char) * (len + 1));
  if (query == NULL) 
    return nerr_raise (NERR_NOMEM, 
	"Unable to allocate memory to read POST input of length %d", l);

  err = cgiwrap_read (query, len, &r);
  if (err) 
  {
    free(query);
    return nerr_pass(err);
  }
  if (r != len)
  {
    free(query);
    return nerr_raise (NERR_IO, "Short read on CGI POST input (%d < %d)", 
	r, len);
  }
  query[len] = '\0';
  err = _parse_query (cgi, query);
  free(query);
  return nerr_pass(err);
}

static NEOERR *_parse_cookie (CGI *cgi)
{
  NEOERR *err;
  char *cookie;
  char *k, *v, *l; 
  HDF *obj;

  err = hdf_get_copy (cgi->hdf, "HTTP.Cookie", &cookie, NULL);
  if (err != STATUS_OK) return nerr_pass(err);
  if (cookie == NULL) return STATUS_OK;

  err = hdf_set_value (cgi->hdf, "Cookie", cookie);
  if (err != STATUS_OK) 
  {
    free(cookie);
    return nerr_pass(err);
  }
  obj = hdf_get_obj (cgi->hdf, "Cookie");

  k = strtok_r (cookie, "=", &l);
  while (k != NULL)
  {
    if (*l == ';')
    {
      v = "";
      l++;
    } else {
      v = strtok_r (NULL, ";", &l);
    }
    if (v == NULL) break;
    k = neos_strip (k);
    v = neos_strip (v);
    if (k[0] && v[0])
    {
      err = hdf_set_value (obj, k, v);
      if (err) break;
    }
    k = strtok_r (NULL, "=", &l);
  }

  free (cookie);

  return nerr_pass(err);
}

NEOERR *cgi_parse (CGI *cgi)
{
  NEOERR *err;
  int x = 0;
  char buf[256];
  char *method, *type, *query;

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
  err = _parse_cookie(cgi);
  if (err != STATUS_OK) return nerr_pass (err);

  err = hdf_get_copy (cgi->hdf, "CGI.QueryString", &query, NULL);
  if (err != STATUS_OK) return nerr_pass (err);
  if (query != NULL)
  {
    err = _parse_query(cgi, query);
    free(query);
    if (err != STATUS_OK) return nerr_pass (err);
  }

  method = hdf_get_value (cgi->hdf, "CGI.RequestMethod", "GET");
  type = hdf_get_value (cgi->hdf, "CGI.ContentType", NULL);
  if (!strcmp(method, "POST"))
  {
    if (type && !strcmp(type, "application/x-www-form-urlencoded"))
    {
      err = _parse_post_form(cgi);
      if (err != STATUS_OK) return nerr_pass (err);
    }
  }

  return STATUS_OK;
}

static void _launch_debugger (CGI *cgi, char *display)
{
  pid_t myPid, pid;
  char buffer[127];
  char *debugger;

  myPid = getpid();

  if ((pid = fork()) < 0)
    return;

  if ((debugger = hdf_get_value (cgi->hdf, "Config.Debugger", NULL)) == NULL)
  {
    debugger = "/usr/local/bin/sudo /usr/local/bin/ddd -display %s %s %d";
  }

  if (!pid)
  {
    sprintf(buffer, debugger, display, Argv0, myPid);
    execl("/bin/sh", "sh", "-c", buffer, NULL);
  }
  else
  {
    sleep(60);
  }
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

    {
      char *display;

      display = hdf_get_value (mycgi->hdf, "Query.xdisplay", NULL);
      if (display)
      {
	fprintf(stderr, "** Got display %s\n", display);
	_launch_debugger(mycgi, display);
      }
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
    int x;

    snprintf (buf, sizeof(buf), "\n<!-- %5.3f:%d -->\n",
	dis - cgi->time_start, use_deflate || use_gzip);
    err = string_append (str, buf);
    if (err != STATUS_OK) return nerr_pass(err);

    if (hdf_get_int_value (cgi->hdf, "Query.debug", 0))
    {
      err = string_append (str, "<hr>");
      if (err != STATUS_OK) return nerr_pass(err);
      x = 0;
      while (1)
      {
	char *k, *v;
	err = cgiwrap_iterenv (x, &k, &v);
	if (err != STATUS_OK) return nerr_pass(err);
	if (k == NULL) break;
	err =string_appendf (str, "%s = %s<br>", k, v);
	if (err != STATUS_OK) return nerr_pass(err);
	free(k);  
	free(v);
	x++;
      }
      err = string_append (str, "<pre>");
      if (err != STATUS_OK) return nerr_pass(err);
      err = hdf_dump_str (cgi->hdf, NULL, str);
      if (err != STATUS_OK) return nerr_pass(err);
    }
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
  char *debug;
  CSPARSE *cs = NULL;
  STRING str;

  string_init(&str);

  debug = hdf_get_value (cgi->hdf, "Query.debug", NULL);

  do
  {
    err = cs_init (&cs, cgi->hdf);
    if (err != STATUS_OK) break;
    err = cs_parse_file (cs, cs_file);
    if (err != STATUS_OK) break;
    if (debug && !strcmp(debug, "dump"))
    {
      cgiwrap_writef("Content-Type: text/plain\n\n");
      hdf_dump(cgi->hdf, NULL);
      cs_dump(cs);
      break;
    }
    else
    {
      err = cs_render (cs, &str, render_cb);
      if (err != STATUS_OK) break;
    }
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
  char *v, *k;

  Argv0 = argv[0];

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
	k = neos_strip(line);
	cgiwrap_putenv (line, v);
      }
    }
    fclose(fp);
  }
}

void cgi_vredirect (CGI *cgi, int uri, char *fmt, va_list ap)
{
  char *host;

  cgiwrap_writef ("Status: 302\r\n");
  cgiwrap_writef ("Content-Type: text/html\r\n");
  cgiwrap_writef ("Pragma: no-cache\r\n");
  cgiwrap_writef ("Expires: Fri, 01 Jan 1999 00:00:00 GMT\r\n");
  cgiwrap_writef ("Cache-control: no-cache, no-cache=\"Set-Cookie\", private\r\n");

  if (uri)
  {
    cgiwrap_writef ("Location: ");
  }
  else
  {
    host = hdf_get_value (cgi->hdf, "HTTP.Host", NULL);
    if (host == NULL)
      host = hdf_get_value (cgi->hdf, "CGI.ServerName", NULL);
    cgiwrap_writef ("Location: http://%s", host);
  }
  cgiwrap_writevf (fmt, ap);
  cgiwrap_writef ("\r\n\r\n");
  cgiwrap_writef ("There is nothing to see here, please move along...");

}

void cgi_redirect (CGI *cgi, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  cgi_vredirect (cgi, 0, fmt, ap);
  va_end(ap);
  return;
}

void cgi_redirect_uri (CGI *cgi, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  cgi_vredirect (cgi, 1, fmt, ap);
  va_end(ap);
  return;
}

char *cgi_cookie_authority (CGI *cgi, char *host)
{
  HDF *obj;
  char *domain;
  int hlen = 0, dlen = 0;

  if (host == NULL)
  {
    host = hdf_get_value (cgi->hdf, "HTTP.Host", NULL);
  }
  if (host == NULL) return NULL;

  while (host[hlen] && host[hlen] != ':') hlen++;

  obj = hdf_get_obj (cgi->hdf, "CookieAuthority");
  if (obj == NULL) return NULL;
  for (obj = hdf_obj_child (obj);
       obj;
       obj = hdf_obj_next (obj))
  {
    domain = hdf_obj_value (obj);
    dlen = strlen(domain);
    if (hlen >= dlen)
    {
      if (!strncasecmp (host + hlen - dlen, domain, dlen))
	return domain;
    }
  }

  return NULL;
}

NEOERR *cgi_cookie_set (CGI *cgi, char *name, char *value, char *path, 
        char *domain, char *time_str, int persistant)
{
  char my_time[256];

  if (path == NULL) path = "/";
  if (persistant)
  {
    if (time_str == NULL)
    {
      /* Expires in one year */
      time_t exp_date = time(NULL) + 31536000;

      strftime (my_time, 48, "%A, %d-%b-%Y 23:59:59 GMT",
	  gmtime (&exp_date));
      time_str = my_time;
    }
    if (domain)
    {
      cgiwrap_writef ("Set-Cookie: %s=%s; path=%s; expires=%s; domain=%s\r\n",
	  name, value, path, time_str, domain);
    }
    else
    {
      cgiwrap_writef ("Set-Cookie: %s=%s; path=%s; expires=%s\r\n", name, 
	  value, path, time_str);
    }
  }
  else
  {
    if (domain)
    {
      cgiwrap_writef ("Set-Cookie: %s=%s; path=%s; domain=%s\r\n", name, 
	  value, path, domain);
    }
    else
    {
      cgiwrap_writef ("Set-Cookie: %s=%s; path=%s\r\n", name, value, path);
    }
  }
  return STATUS_OK;
}

/* This will actually issue up to three set cookies, attempting to clear
 * the damn thing. */
NEOERR *cgi_cookie_clear (CGI *cgi, char *name, char *domain, char *path)
{
  if (path == NULL) path = "/";
  if (domain)
  {
    if (domain[0] == '.')
    {
      cgiwrap_writef ("Set-Cookie: %s=; path=%s; domain=%s;"
	  "expires=Thursday, 01-Jan-1970 00:00:00 GMT\r\n", name, path,
	  domain + 1);
    }
    cgiwrap_writef("Set-Cookie: %s=; path=%s; domain=%s;"
	"expires=Thursday, 01-Jan-1970 00:00:00 GMT\r\n", name, path,
	domain);
  }
  cgiwrap_writef("Set-Cookie: %s=; path=%s; "
      "expires=Thursday, 01-Jan-1970 00:00:00 GMT\r\n", name, path);

  return STATUS_OK;
}

