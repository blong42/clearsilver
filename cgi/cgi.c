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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#if defined(HTML_COMPRESSION)
#include <zlib.h>
#endif

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_hdf.h"
#include "util/neo_str.h"
#include "cgi.h"
#include "cgiwrap.h"
#include "html.h"
#include "cs/cs.h"


struct _cgi_vars
{
  char *env_name;
  char *hdf_name;
} CGIVars[] = {
  {"AUTH_TYPE", "AuthType"},
  {"CONTENT_TYPE", "ContentType"},
  {"CONTENT_LENGTH", "ContentLength"},
  {"DOCUMENT_ROOT", "DocumentRoot"},
  {"GATEWAY_INTERFACE", "GatewayInterface"},
  {"PATH_INFO", "PathInfo"},
  {"PATH_TRANSLATED", "PathTranslated"},
  {"QUERY_STRING", "QueryString"},
  {"REDIRECT_REQUEST", "RedirectRequest"},
  {"REDIRECT_QUERY_STRING", "RedirectQueryString"},
  {"REDIRECT_STATUS", "RedirectStatus"},
  {"REDIRECT_URL", "RedirectURL",},
  {"REMOTE_ADDR", "RemoteAddress"},
  {"REMOTE_HOST", "RemoteHost"},
  {"REMOTE_IDENT", "RemoteIdent"},
  {"REMOTE_PORT", "RemotePort"},
  {"REMOTE_USER", "RemoteUser"},
  {"REMOTE_GROUP", "RemoteGroup"},
  {"REQUEST_METHOD", "RequestMethod"},
  {"REQUEST_URI", "RequestURI"},
  {"SCRIPT_FILENAME", "ScriptFilename"},
  {"SCRIPT_NAME", "ScriptName"},
  {"SERVER_ADDR", "ServerAddress"},
  {"SERVER_ADMIN", "ServerAdmin"},
  {"SERVER_NAME", "ServerName"},
  {"SERVER_PORT", "ServerPort"},
  {"SERVER_ROOT", "ServerRoot"},
  {"SERVER_PROTOCOL", "ServerProtocol"},
  {"SERVER_SOFTWARE", "ServerSoftware"},
  /* SSL Vars from mod_ssl */
  {"HTTPS", "HTTPS"},
  {"SSL_PROTOCOL", "SSL.Protocol"},
  {"SSL_SESSION_ID", "SSL.SessionID"},
  {"SSL_CIPHER", "SSL.Cipher"},
  {"SSL_CIPHER_EXPORT", "SSL.Cipher.Export"},
  {"SSL_CIPHER_USEKEYSIZE", "SSL.Cipher.UseKeySize"},
  {"SSL_CIPHER_ALGKEYSIZE", "SSL.Cipher.AlgKeySize"},
  {"SSL_VERSION_INTERFACE", "SSL.Version.Interface"},
  {"SSL_VERSION_LIBRARY", "SSL.Version.Library"},
  {"SSL_CLIENT_M_VERSION", "SSL.Client.M.Version"},
  {"SSL_CLIENT_M_SERIAL", "SSL.Client.M.Serial"},
  {"SSL_CLIENT_S_DN", "SSL.Client.S.DN"},
  {"SSL_CLIENT_S_DN_x509", "SSL.Client.S.DN.x509"},
  {"SSL_CLIENT_I_DN", "SSL.Client.I.DN"},
  {"SSL_CLIENT_I_DN_x509", "SSL.Client.I.DN.x509"},
  {"SSL_CLIENT_V_START", "SSL.Client.V.Start"},
  {"SSL_CLIENT_V_END", "SSL.Client.V.End"},
  {"SSL_CLIENT_A_SIG", "SSL.Client.A.SIG"},
  {"SSL_CLIENT_A_KEY", "SSL.Client.A.KEY"},
  {"SSL_CLIENT_CERT", "SSL.Client.CERT"},
  {"SSL_CLIENT_CERT_CHAINn", "SSL.Client.CERT.CHAINn"},
  {"SSL_CLIENT_VERIFY", "SSL.Client.Verify"},
  {"SSL_SERVER_M_VERSION", "SSL.Server.M.Version"},
  {"SSL_SERVER_M_SERIAL", "SSL.Server.M.Serial"},
  {"SSL_SERVER_S_DN", "SSL.Server.S.DN"},
  {"SSL_SERVER_S_DN_x509", "SSL.Server.S.DN.x509"},
  {"SSL_SERVER_S_DN_CN", "SSL.Server.S.DN.CN"},
  {"SSL_SERVER_S_DN_EMAIL", "SSL.Server.S.DN.Email"},
  {"SSL_SERVER_S_DN_O", "SSL.Server.S.DN.O"},
  {"SSL_SERVER_S_DN_OU", "SSL.Server.S.DN.OU"},
  {"SSL_SERVER_S_DN_C", "SSL.Server.S.DN.C"},
  {"SSL_SERVER_S_DN_SP", "SSL.Server.S.DN.SP"},
  {"SSL_SERVER_S_DN_L", "SSL.Server.S.DN.L"},
  {"SSL_SERVER_I_DN", "SSL.Server.I.DN"},
  {"SSL_SERVER_I_DN_x509", "SSL.Server.I.DN.x509"},
  {"SSL_SERVER_I_DN_CN", "SSL.Server.I.DN.CN"},
  {"SSL_SERVER_I_DN_EMAIL", "SSL.Server.I.DN.Email"},
  {"SSL_SERVER_I_DN_O", "SSL.Server.I.DN.O"},
  {"SSL_SERVER_I_DN_OU", "SSL.Server.I.DN.OU"},
  {"SSL_SERVER_I_DN_C", "SSL.Server.I.DN.C"},
  {"SSL_SERVER_I_DN_SP", "SSL.Server.I.DN.SP"},
  {"SSL_SERVER_I_DN_L", "SSL.Server.I.DN.L"},
  {"SSL_SERVER_V_START", "SSL.Server.V.Start"},
  {"SSL_SERVER_V_END", "SSL.Server.V.End"},
  {"SSL_SERVER_A_SIG", "SSL.Server.A.SIG"},
  {"SSL_SERVER_A_KEY", "SSL.Server.A.KEY"},
  {"SSL_SERVER_CERT", "SSL.Server.CERT"},
  /* SSL Vars mapped from others */
  /* Hmm, if we're running under mod_ssl w/ +CompatEnvVars, we set these
   * twice... */
  {"SSL_PROTOCOL_VERSION", "SSL.Protocol"},
  {"SSLEAY_VERSION", "SSL.Version.Library"},
  {"HTTPS_CIPHER", "SSL.Cipher"},
  {"HTTPS_EXPORT", "SSL.Cipher.Export"},
  {"HTTPS_SECRETKEYSIZE", "SSL.Cipher.UseKeySize"},
  {"HTTPS_KEYSIZE", "SSL.Cipher.AlgKeySize"},
  {"SSL_SERVER_KEY_SIZE", "SSL.Cipher.AlgKeySize"},
  {"SSL_SERVER_CERTIFICATE", "SSL.Server.CERT"},
  {"SSL_SERVER_CERT_START", "SSL.Server.V.Start"},
  {"SSL_SERVER_CERT_END", "SSL.Server.V.End"},
  {"SSL_SERVER_CERT_SERIAL", "SSL.Server.M.Serial"},
  {"SSL_SERVER_SIGNATURE_ALGORITHM", "SSL.Server.A.SIG"},
  {"SSL_SERVER_DN", "SSL.Server.S.DN"},
  {"SSL_SERVER_CN", "SSL.Server.S.DN.CN"},
  {"SSL_SERVER_EMAIL", "SSL.Server.S.DN.Email"},
  {"SSL_SERVER_O", "SSL.Server.S.DN.O"},
  {"SSL_SERVER_OU", "SSL.Server.S.DN.OU"},
  {"SSL_SERVER_C", "SSL.Server.S.DN.C"},
  {"SSL_SERVER_SP", "SSL.Server.S.DN.SP"},
  {"SSL_SERVER_L", "SSL.Server.S.DN.L"},
  {"SSL_SERVER_IDN", "SSL.Server.I.DN"},
  {"SSL_SERVER_ICN", "SSL.Server.I.DN.CN"},
  {"SSL_SERVER_IEMAIL", "SSL.Server.I.DN.Email"},
  {"SSL_SERVER_IO", "SSL.Server.I.DN.O"},
  {"SSL_SERVER_IOU", "SSL.Server.I.DN.OU"},
  {"SSL_SERVER_IC", "SSL.Server.I.DN.C"},
  {"SSL_SERVER_ISP", "SSL.Server.I.DN.SP"},
  {"SSL_SERVER_IL", "SSL.Server.I.DN.L"},
  {"SSL_CLIENT_CERTIFICATE", "SSL.Client.CERT"},
  {"SSL_CLIENT_CERT_START", "SSL.Client.V.Start"},
  {"SSL_CLIENT_CERT_END", "SSL.Client.V.End"},
  {"SSL_CLIENT_CERT_SERIAL", "SSL.Client.M.Serial"},
  {"SSL_CLIENT_SIGNATURE_ALGORITHM", "SSL.Client.A.SIG"},
  {"SSL_CLIENT_DN", "SSL.Client.S.DN"},
  {"SSL_CLIENT_CN", "SSL.Client.S.DN.CN"},
  {"SSL_CLIENT_EMAIL", "SSL.Client.S.DN.Email"},
  {"SSL_CLIENT_O", "SSL.Client.S.DN.O"},
  {"SSL_CLIENT_OU", "SSL.Client.S.DN.OU"},
  {"SSL_CLIENT_C", "SSL.Client.S.DN.C"},
  {"SSL_CLIENT_SP", "SSL.Client.S.DN.SP"},
  {"SSL_CLIENT_L", "SSL.Client.S.DN.L"},
  {"SSL_CLIENT_IDN", "SSL.Client.I.DN"},
  {"SSL_CLIENT_ICN", "SSL.Client.I.DN.CN"},
  {"SSL_CLIENT_IEMAIL", "SSL.Client.I.DN.Email"},
  {"SSL_CLIENT_IO", "SSL.Client.I.DN.O"},
  {"SSL_CLIENT_IOU", "SSL.Client.I.DN.OU"},
  {"SSL_CLIENT_IC", "SSL.Client.I.DN.C"},
  {"SSL_CLIENT_ISP", "SSL.Client.I.DN.SP"},
  {"SSL_CLIENT_IL", "SSL.Client.I.DN.L"},
  {"SSL_EXPORT", "SSL.Cipher.Export"},
  {"SSL_KEYSIZE", "SSL.Cipher.AlgKeySize"},
  {"SSL_SECKEYSIZE", "SSL.Cipher.UseKeySize"},
  {"SSL_SSLEAY_VERSION", "SSL.Version.Library"},
/* Old vars not in mod_ssl */
  {"SSL_STRONG_CRYPTO", "SSL.Strong.Crypto"},
  {"SSL_SERVER_KEY_EXP", "SSL.Server.Key.Exp"},
  {"SSL_SERVER_KEY_ALGORITHM", "SSL.Server.Key.Algorithm"},
  {"SSL_SERVER_KEY_SIZE", "SSL.Server.Key.Size"},
  {"SSL_SERVER_SESSIONDIR", "SSL.Server.SessionDir"},
  {"SSL_SERVER_CERTIFICATELOGDIR", "SSL.Server.CertificateLogDir"},
  {"SSL_SERVER_CERTFILE", "SSL.Server.CertFile"},
  {"SSL_SERVER_KEYFILE", "SSL.Server.KeyFile"},
  {"SSL_SERVER_KEYFILETYPE", "SSL.Server.KeyFileType"},
  {"SSL_CLIENT_KEY_EXP", "SSL.Client.Key.Exp"},
  {"SSL_CLIENT_KEY_ALGORITHM", "SSL.Client.Key.Algorithm"},
  {"SSL_CLIENT_KEY_SIZE", "SSL.Client.Key.Size"},
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
  {"HTTP_VIA", "Via"},
  /* SOAP */
  {"HTTP_SOAPACTION", "Soap.Action"},
  {NULL, NULL}
};

static char *Argv0 = "";

int IgnoreEmptyFormVars = 0;

static int ExceptionsInit = 0;
NERR_TYPE CGIFinished = -1;
NERR_TYPE CGIUploadCancelled = -1;
NERR_TYPE CGIParseNotHandled = -1;

static NEOERR *_add_cgi_env_var (CGI *cgi, char *env, char *name)
{
  NEOERR *err;
  char *s;

  err = cgiwrap_getenv (env, &s);
  if (err != STATUS_OK) return nerr_pass (err);
  if (s != NULL)
  {
    err = hdf_set_buf (cgi->hdf, name, s);
    if (err != STATUS_OK)
    {
      free(s);
      return nerr_pass (err);
    }
  }
  return STATUS_OK;
}

char *cgi_url_unescape (char *value)
{
  int i = 0, o = 0;
  unsigned char *s = (unsigned char *)value;

  if (s == NULL) return value;
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
  return (char *)s;
}

NEOERR *cgi_js_escape (const char *in, char **esc)
{
  return nerr_pass(neos_js_escape(in, esc));
}

NEOERR *cgi_url_escape (const char *buf, char **esc)
{
  return nerr_pass(neos_url_escape(buf, esc, NULL));
}

NEOERR *cgi_url_escape_more (const char *in, char **esc,
                                 const char *other)
{
  return nerr_pass(neos_url_escape(in, esc, other));
}

NEOERR *cgi_url_validate (const char *buf, char **esc)
{
  return nerr_pass(neos_url_validate(buf, esc));
}

static NEOERR *_parse_query (CGI *cgi, char *query)
{
  NEOERR *err = STATUS_OK;
  char *t, *k, *v, *l;
  char buf[256];
  char unnamed[10];
  int unnamed_count = 0;
  HDF *obj, *child;

  if (query && *query)
  {
    k = strtok_r(query, "&", &l);
    while (k && *k)
    {
      v = strchr(k, '=');
      if (v == NULL)
      {
	v = "";
      }
      else
      {
	*v = '\0';
	v++;
      }


      /* Check for some invalid query strings */
      if (*k == 0) {
        /*  '?=foo' gets mapped in as Query._1=foo */
        snprintf(unnamed,sizeof(unnamed), "_%d", unnamed_count++);
        k = unnamed;
      } else if (*k == '.') {
        /* an hdf element can't start with a period */
        *k = '_';
      }
      snprintf(buf, sizeof(buf), "Query.%s", cgi_url_unescape(k));

      if (!(cgi->ignore_empty_form_vars && (*v == '\0')))
      {


	cgi_url_unescape(v);
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
	if (nerr_match(err, NERR_ASSERT)) {
	  STRING str;

	  string_init(&str);
	  nerr_error_string(err, &str);
	  ne_warn("Unable to set Query value: %s = %s: %s", buf, v, str.buf);
	  string_clear(&str);
	  nerr_ignore(&err);
	}
	if (err != STATUS_OK) break;
      }
      k = strtok_r(NULL, "&", &l);
    }
  }
  return nerr_pass(err);
}

/* Is it an error if its a short read? */
static NEOERR *_parse_post_form (CGI *cgi)
{
  NEOERR *err = STATUS_OK;
  char *l, *query;
  int len, r, o;

  l = hdf_get_value (cgi->hdf, "CGI.ContentLength", NULL);
  if (l == NULL) return STATUS_OK;
  len = atoi (l);
  if (len <= 0) return STATUS_OK;

  cgi->data_expected = len;

  query = (char *) malloc (sizeof(char) * (len + 1));
  if (query == NULL)
    return nerr_raise (NERR_NOMEM,
	"Unable to allocate memory to read POST input of length %d", len);


  o = 0;
  while (o < len)
  {
    cgiwrap_read (query + o, len - o, &r);
    if (r <= 0) break;
    o = o + r;
  }
  if (r < 0)
  {
    free(query);
    return nerr_raise_errno (NERR_IO, "Short read on CGI POST input (%d < %d)",
	o, len);
  }
  if (o != len)
  {
    free(query);
    return nerr_raise (NERR_IO, "Short read on CGI POST input (%d < %d)",
	o, len);
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

  k = l = cookie;
  while (*l && *l != '=' && *l != ';') l++;
  while (*k)
  {
    if (*l == '=')
    {
      if (*l) *l++ = '\0';
      v = l;
      while (*l && *l != ';') l++;
      if (*l) *l++ = '\0';
    }
    else
    {
      v = "";
      if (*l) *l++ = '\0';
    }
    k = neos_strip (k);
    v = neos_strip (v);
    if (k[0] && v[0])
    {
      err = hdf_set_value (obj, k, v);
      if (nerr_match(err, NERR_ASSERT)) {
	STRING str;

	string_init(&str);
	nerr_error_string(err, &str);
	ne_warn("Unable to set Cookie value: %s = %s: %s", k, v, str.buf);
	string_clear(&str);
	nerr_ignore(&err);
      }
      if (err) break;
    }
    k = l;
    while (*l && *l != '=' && *l != ';') l++;
  }

  free (cookie);

  return nerr_pass(err);
}

#ifdef ENABLE_REMOTE_DEBUG

static void _launch_debugger (CGI *cgi, char *display)
{
  pid_t myPid, pid;
  char buffer[127];
  char *debugger;
  HDF *obj;
  char *allowed;

  /* Only allow remote debugging from allowed hosts */
  for (obj = hdf_get_child (cgi->hdf, "Config.Displays");
      obj; obj = hdf_obj_next (obj))
  {
    allowed = hdf_obj_value (obj);
    if (allowed && !strcmp (display, allowed)) break;
  }
  if (obj == NULL) return;

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

#endif

static NEOERR *cgi_pre_parse (CGI *cgi)
{
  NEOERR *err;
  int x = 0;
  char buf[256];
  char *query;

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

  {
    char *d = hdf_get_value(cgi->hdf, "Query.debug_pause", NULL);
    char *d_p = hdf_get_value(cgi->hdf, "Config.DebugPassword", NULL);

    if (hdf_get_int_value(cgi->hdf, "Config.DebugEnabled", 0) &&
        d && d_p && !strcmp(d, d_p)) {
      sleep(20);
    }
  }

#ifdef ENABLE_REMOTE_DEBUG
  {
    char *display;

    display = hdf_get_value (cgi->hdf, "Query.xdisplay", NULL);
    if (display)
    {
      fprintf(stderr, "** Got display %s\n", display);
      _launch_debugger(cgi, display);
    }
  }
#endif

  return STATUS_OK;
}

NEOERR *cgi_register_parse_cb(CGI *cgi, const char *method, const char *ctype,
                              void *rock, CGI_PARSE_CB parse_cb)
{
  struct _cgi_parse_cb *my_pcb;

  if (method == NULL || ctype == NULL)
    return nerr_raise(NERR_ASSERT, "method and type must not be NULL to register cb");

  my_pcb = (struct _cgi_parse_cb *) calloc(1, sizeof(struct _cgi_parse_cb));
  if (my_pcb == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory to register parse cb");

  my_pcb->method = strdup(method);
  my_pcb->ctype = strdup(ctype);
  if (my_pcb->method == NULL || my_pcb->ctype == NULL)
  {
    if (my_pcb->method != NULL)
      free(my_pcb->method);
    if (my_pcb->ctype != NULL)
      free(my_pcb->ctype);
    free(my_pcb);
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory to register parse cb");
  }
  if (!strcmp(my_pcb->method, "*"))
    my_pcb->any_method = 1;
  if (!strcmp(my_pcb->ctype, "*"))
    my_pcb->any_ctype = 1;
  my_pcb->rock = rock;
  my_pcb->parse_cb = parse_cb;
  my_pcb->next = cgi->parse_callbacks;
  cgi->parse_callbacks = my_pcb;
  return STATUS_OK;
}

NEOERR *cgi_parse (CGI *cgi)
{
  NEOERR *err;
  char *method, *type;
  struct _cgi_parse_cb *pcb;


  method = hdf_get_value (cgi->hdf, "CGI.RequestMethod", "GET");
  type = hdf_get_value (cgi->hdf, "CGI.ContentType", NULL);
  /* Walk the registered parse callbacks for a matching one */
  pcb = cgi->parse_callbacks;
  while (pcb != NULL)
  {
    if ( (pcb->any_method || !strcasecmp(pcb->method, method)) &&
	 (pcb->any_ctype || (type && !strcasecmp(pcb->ctype, type))) )
    {
      err = pcb->parse_cb(cgi, method, type, pcb->rock);
      if (err && !nerr_handle(&err, CGIParseNotHandled))
	return nerr_pass(err);
    }
    pcb = pcb->next;
  }

  /* Fallback to internal methods */

  if (!strcmp(method, "POST"))
  {
    if (type && !strcmp(type, "application/x-www-form-urlencoded"))
    {
      err = _parse_post_form(cgi);
      if (err != STATUS_OK) return nerr_pass (err);
    }
    else if (type && !strncmp (type, "multipart/form-data", 19))
    {
      err = parse_rfc2388 (cgi);
      if (err != STATUS_OK) return nerr_pass (err);
    }
#if 0
    else
    {
      int len, x, r;
      char *l;
      char buf[4096];
      FILE *fp;

      fp = fopen("/tmp/upload", "w");

      l = hdf_get_value (cgi->hdf, "CGI.ContentLength", NULL);
      if (l == NULL) return STATUS_OK;
      len = atoi (l);

      x = 0;
      while (x < len)
      {
	if (len-x > sizeof(buf))
	  cgiwrap_read (buf, sizeof(buf), &r);
	else
	  cgiwrap_read (buf, len - x, &r);
	fwrite (buf, 1, r, fp);
	x += r;
      }
      fclose (fp);
      if (err) return nerr_pass(err);
    }
#endif
  }
  else if (!strcmp(method, "PUT"))
  {
    FILE *fp;
    int len, x, r, w;
    char *l;
    char buf[4096];
    int unlink_files = hdf_get_int_value(cgi->hdf, "Config.Upload.Unlink", 1);

    err = open_upload(cgi, unlink_files, &fp);
    if (err) return nerr_pass(err);

    l = hdf_get_value (cgi->hdf, "CGI.ContentLength", NULL);
    if (l == NULL) return STATUS_OK;
    len = atoi (l);
    if (len <= 0) return STATUS_OK;

    x = 0;
    while (x < len)
    {
      if (len-x > sizeof(buf))
	cgiwrap_read (buf, sizeof(buf), &r);
      else
	cgiwrap_read (buf, len - x, &r);
      w = fwrite (buf, sizeof(char), r, fp);
      if (w != r)
      {
	err = nerr_raise_errno(NERR_IO, "Short write on PUT: %d < %d", w, r);
	break;
      }
      x += r;
    }
    if (err) return nerr_pass(err);
    fseek(fp, 0, SEEK_SET);
    l = hdf_get_value(cgi->hdf, "CGI.PathInfo", NULL);
    if (l) err = hdf_set_value (cgi->hdf, "PUT", l);
    if (err) return nerr_pass(err);
    if (type) err = hdf_set_value (cgi->hdf, "PUT.Type", type);
    if (err) return nerr_pass(err);
    err = hdf_set_int_value (cgi->hdf, "PUT.FileHandle", uListLength(cgi->files));
    if (err) return nerr_pass(err);
    if (!unlink_files)
    {
      char *name;
      err = uListGet(cgi->filenames, uListLength(cgi->filenames)-1,
	  (void *)&name);
      if (err) return nerr_pass(err);
      err = hdf_set_value (cgi->hdf, "PUT.FileName", name);
      if (err) return nerr_pass(err);
    }
  }
  return STATUS_OK;
}

NEOERR *cgi_init (CGI **cgi, HDF *hdf)
{
  NEOERR *err = STATUS_OK;
  CGI *mycgi;

  if (ExceptionsInit == 0)
  {
    err = nerr_init();
    if (err) return nerr_pass(err);
    err = nerr_register(&CGIFinished, "CGIFinished");
    if (err) return nerr_pass(err);
    err = nerr_register(&CGIUploadCancelled, "CGIUploadCancelled");
    if (err) return nerr_pass(err);
    err = nerr_register(&CGIUploadCancelled, "CGIParseNotHandled");
    if (err) return nerr_pass(err);
    ExceptionsInit = 1;
  }

  *cgi = NULL;
  mycgi = (CGI *) calloc (1, sizeof(CGI));
  if (mycgi == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate space for CGI");

  mycgi->time_start = ne_timef();

  mycgi->ignore_empty_form_vars = IgnoreEmptyFormVars;

  do
  {
    if (hdf == NULL)
    {
      err = hdf_init (&(mycgi->hdf));
      if (err != STATUS_OK) break;
    }
    else
    {
      mycgi->hdf = hdf;
    }
    err = cgi_pre_parse (mycgi);
    if (err != STATUS_OK) break;

  } while (0);

  if (err == STATUS_OK)
    *cgi = mycgi;
  else
  {
    cgi_destroy(&mycgi);
  }
  return nerr_pass(err);
}

static void _destroy_tmp_file(char *filename)
{
  unlink(filename);
  free(filename);
}

void cgi_destroy (CGI **cgi)
{
  CGI *my_cgi;

  if (!cgi || !*cgi)
    return;
  my_cgi = *cgi;
  if (my_cgi->hdf)
    hdf_destroy (&(my_cgi->hdf));
  if (my_cgi->buf)
    free(my_cgi->buf);
  if (my_cgi->files)
    uListDestroyFunc(&(my_cgi->files), (void (*)(void *))fclose);
  if (my_cgi->filenames)
    uListDestroyFunc(&(my_cgi->filenames), (void (*)(void *))_destroy_tmp_file);
  free (*cgi);
  *cgi = NULL;
}

static NEOERR *render_cb (void *ctx, char *buf)
{
  STRING *str = (STRING *)ctx;
  NEOERR *err;

  err = nerr_pass(string_append(str, buf));
  return err;
}

static NEOERR *cgi_headers (CGI *cgi)
{
  NEOERR *err = STATUS_OK;
  HDF *obj, *child;
  char *s, *charset = NULL;

  if (hdf_get_int_value (cgi->hdf, "Config.NoCache", 0))
  {
    /* Ok, we try really hard to defeat caches here */
    /* this isn't in any HTTP rfc's, it just seems to be a convention */
    err = cgiwrap_writef ("Pragma: no-cache\r\n");
    if (err != STATUS_OK) return nerr_pass (err);
    err = cgiwrap_writef ("Expires: Fri, 01 Jan 1990 00:00:00 GMT\r\n");
    if (err != STATUS_OK) return nerr_pass (err);
    err = cgiwrap_writef ("Cache-control: no-cache, must-revalidate, no-cache=\"Set-Cookie\", private\r\n");
    if (err != STATUS_OK) return nerr_pass (err);
  }
  obj = hdf_get_obj (cgi->hdf, "cgiout");
  if (obj)
  {
    s = hdf_get_value (obj, "Status", NULL);
    if (s)
      err = cgiwrap_writef ("Status: %s\r\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
    s = hdf_get_value (obj, "Location", NULL);
    if (s)
      err = cgiwrap_writef ("Location: %s\r\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
    child = hdf_get_obj (cgi->hdf, "cgiout.other");
    if (child)
    {
      child = hdf_obj_child (child);
      while (child != NULL)
      {
	s = hdf_obj_value (child);
	err = cgiwrap_writef ("%s\r\n", s);
	if (err != STATUS_OK) return nerr_pass (err);
	child = hdf_obj_next(child);
      }
    }
    charset = hdf_get_value (obj, "charset", NULL);
    s = hdf_get_value (obj, "ContentType", "text/html");
    if (charset)
      err = cgiwrap_writef ("Content-Type: %s; charset=%s\r\n\r\n", s, charset);
    else
      err = cgiwrap_writef ("Content-Type: %s\r\n\r\n", s);
    if (err != STATUS_OK) return nerr_pass (err);
  }
  else
  {
    /* Default */
    err = cgiwrap_writef ("Content-Type: text/html\r\n\r\n");
    if (err != STATUS_OK) return nerr_pass (err);
  }
  return STATUS_OK;
}

#if defined(HTML_COMPRESSION)
/* Copy these here from zutil.h, which we aren't supposed to include */
#define DEF_MEM_LEVEL 8
#define OS_CODE 0x03

static NEOERR *cgi_compress (STRING *str, char *obuf, int *olen)
{
  z_stream stream;
  int err;

  stream.next_in = (Bytef*)str->buf;
  stream.avail_in = (uInt)str->len;
  stream.next_out = (Bytef*)obuf;
  stream.avail_out = (uInt)*olen;
  if ((uLong)stream.avail_out != *olen)
    return nerr_raise(NERR_NOMEM, "Destination too big: %d", *olen);

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
#endif

/* This ws strip function is Dave's version, designed to make debug
 * easier, and the output a bit smaller... but not as small as it could
 * be: essentially, it strips all empty lines, all extra space at the
 * end of the line, except in pre/textarea tags.
 *
 * Ok, expanding to 3 levels:
 * 0 - No stripping
 * 1 - Dave's debug stripper (as above)
 * 2 - strip all extra white space
 *
 * We don't currently strip white space in a tag
 *
 * */

#if 0
static void debug_output(char *header, char *s, int n)
{
  int x;
  fprintf (stderr, "%s ", header);
  for (x = 0; x < n; x++)
  {
    fprintf (stderr, "%c", s[x]);
  }
  fprintf(stderr, "\n");
}
#endif

void cgi_html_ws_strip(STRING *str, int level)
{
  int ws = 0;
  int seen_nonws = level > 1;
  int i, o, l;
  char *ch;

  i = o = 0;
  if (str->len) {
    ws = isspace(str->buf[0]);
  }
  while (i < str->len)
  {
    if (str->buf[i] == '<')
    {
      str->buf[o++] = str->buf[i++];
      if (!strncasecmp(str->buf + i, "textarea", 8))
      {
	ch = str->buf + i;
	do
	{
	  ch = strchr(ch, '<');
	  if (ch == NULL)
	  {
	    memmove(str->buf + o, str->buf + i, str->len - i);
	    str->len = o + str->len - i;
	    str->buf[str->len] = '\0';
	    return;
	  }
	  ch++;
	} while (strncasecmp(ch, "/textarea>", 10));
	ch += 10;
	l = ch - str->buf - i;
	memmove(str->buf + o, str->buf + i, l);
	o += l;
	i += l;
      }
      else if (!strncasecmp(str->buf + i, "pre", 3))
      {
	ch = str->buf + i;
	do
	{
	  ch = strchr(ch, '<');
	  if (ch == NULL)
	  {
	    memmove(str->buf + o, str->buf + i, str->len - i);
	    str->len = o + str->len - i;
	    str->buf[str->len] = '\0';
	    return;
	  }
	  ch++;
	} while (strncasecmp(ch, "/pre>", 5));
	ch += 5;
	l = ch - str->buf - i;
	memmove(str->buf + o, str->buf + i, l);
	o += l;
	i += l;
      }
      else
      {
	ch = strchr(str->buf + i, '>');
	if (ch == NULL)
	{
	  memmove(str->buf + o, str->buf + i, str->len - i);
	  str->len = o + str->len - i;
	  str->buf[str->len] = '\0';
	  return;
	}
	ch++;
	/* debug_output("copying tag", str->buf + i, ch - str->buf - i);
	 * */
	l = ch - str->buf - i;
	memmove(str->buf + o, str->buf + i, l);
	o += l;
	i += l;
      }
      /* debug_output("result", str->buf, o); */
      seen_nonws = 1;
      ws = 0;
    }
    else if (str->buf[i] == '\n')
    {
      /* This has the effect of erasing all whitespace at the eol plus
       * erasing all blank lines */
      while (o && isspace(str->buf[o-1])) o--;
      str->buf[o++] = str->buf[i++];
      ws = level > 1;
      seen_nonws = level > 1;
    }
    else if (seen_nonws && isspace(str->buf[i]))
    {
      if (ws)
      {
	i++;
      }
      else
      {
	str->buf[o++] = str->buf[i++];
	ws = 1;
      }
    }
    else
    {
      seen_nonws = 1;
      ws = 0;
      str->buf[o++] = str->buf[i++];
    }
  }

  str->len = o;
  str->buf[str->len] = '\0';
}

NEOERR *cgi_output (CGI *cgi, STRING *str)
{
  NEOERR *err = STATUS_OK;
  double dis;
  int is_html = 0;
  int use_deflate = 0;
  int use_gzip = 0;
  int do_debug = 0;
  int do_timefooter = 0;
  int ws_strip_level = 0;
  char *s, *e;

  s = hdf_get_value (cgi->hdf, "Query.debug", NULL);
  e = hdf_get_value (cgi->hdf, "Config.DebugPassword", NULL);
  if (hdf_get_int_value(cgi->hdf, "Config.DebugEnabled", 0) && 
      s && e && !strcmp(s, e)) do_debug = 1;
  do_timefooter = hdf_get_int_value (cgi->hdf, "Config.TimeFooter", 1);
  ws_strip_level = hdf_get_int_value (cgi->hdf, "Config.WhiteSpaceStrip", 1);

  dis = ne_timef();
  s = hdf_get_value (cgi->hdf, "cgiout.ContentType", "text/html");
  if (!strcasecmp(s, "text/html"))
    is_html = 1;

#if defined(HTML_COMPRESSION)
  /* Determine whether or not we can compress the output */
  if (is_html && hdf_get_int_value (cgi->hdf, "Config.CompressionEnabled", 0))
  {
    err = hdf_get_copy (cgi->hdf, "HTTP.AcceptEncoding", &s, NULL);
    if (err != STATUS_OK) return nerr_pass (err);
    if (s)
    {
      char *next;

      e = strtok_r (s, ",", &next);
      while (e && !use_deflate)
      {
	if (strstr(e, "deflate") != NULL)
	{
	  use_deflate = 1;
	  use_gzip = 0;
	}
	else if (strstr(e, "gzip") != NULL)
	  use_gzip = 1;
	e = strtok_r (NULL, ",", &next);
      }
      free (s);
    }
    s = hdf_get_value (cgi->hdf, "HTTP.UserAgent", NULL);
    if (s)
    {
      if (strstr(s, "MSIE 4") || strstr(s, "MSIE 5") || strstr(s, "MSIE 6"))
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
#endif

  err = cgi_headers(cgi);
  if (err != STATUS_OK) return nerr_pass(err);

  if (is_html)
  {
    char buf[50];
    int x;

    if (do_timefooter)
    {
      snprintf (buf, sizeof(buf), "\n<!-- %5.3f:%d -->\n",
	  dis - cgi->time_start, use_deflate || use_gzip);
      err = string_append (str, buf);
      if (err != STATUS_OK) return nerr_pass(err);
    }

    if (ws_strip_level)
    {
      cgi_html_ws_strip(str, ws_strip_level);
    }

    if (do_debug)
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
      err = hdf_dump_str (cgi->hdf, NULL, 0, str);
      if (err != STATUS_OK) return nerr_pass(err);
    }
  }

#if defined(HTML_COMPRESSION)
    if (is_html && (use_deflate || use_gzip))
    {
      char *dest;
      static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
      char gz_buf[20]; /* gzip header/footer buffer, len of header is 10 bytes */
      unsigned int crc = 0;
      int len2;

      if (use_gzip)
      {
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (const Bytef *)(str->buf), str->len);
      }
      len2 = str->len * 2;
      dest = (char *) malloc (sizeof(char) * len2);
      if (dest != NULL)
      {
	do {
	  err = cgi_compress (str, dest, &len2);
	  if (err == STATUS_OK)
	  {
	    if (use_gzip)
	    {
	      /* I'm using sprintf instead of cgiwrap_writef since
	       * the wrapper writef might not handle values with
	       * embedded NULLs... though I should fix the python one
	       * now as well */
	      sprintf(gz_buf, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
		  Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/,
		  OS_CODE);
	      err = cgiwrap_write(gz_buf, 10);
	    }
	    if (err != STATUS_OK) break;
	    err = cgiwrap_write(dest, len2);
	    if (err != STATUS_OK) break;

	    if (use_gzip)
	    {
	      /* write crc and len in network order */
	      sprintf(gz_buf, "%c%c%c%c%c%c%c%c",
		  (0xff & (crc >> 0)),
		  (0xff & (crc >> 8)),
		  (0xff & (crc >> 16)),
		  (0xff & (crc >> 24)),
		  (0xff & (str->len >> 0)),
		  (0xff & (str->len >> 8)),
		  (0xff & (str->len >> 16)),
		  (0xff & (str->len >> 24)));
	      err = cgiwrap_write(gz_buf, 8);
	      if (err != STATUS_OK) break;
	    }
	  }
	  else
	  {
	    nerr_log_error (err);
	    err = cgiwrap_write(str->buf, str->len);
	  }
	} while (0);
	free (dest);
      }
      else
      {
	err = cgiwrap_write(str->buf, str->len);
      }
    }
    else
#endif
    {
      err = cgiwrap_write(str->buf, str->len);
    }

  return nerr_pass(err);
}

NEOERR *cgi_html_escape_strfunc(const char *str, char **ret)
{
  return nerr_pass(html_escape_alloc(str, strlen(str), ret));
}

NEOERR *cgi_html_strip_strfunc(const char *str, char **ret)
{
  return nerr_pass(html_strip_alloc(str, strlen(str), ret));
}

NEOERR *cgi_text_html_strfunc(const char *str, char **ret)
{
  return nerr_pass(convert_text_html_alloc(str, strlen(str), ret));
}

NEOERR *cgi_register_strfuncs(CSPARSE *cs)
{
  NEOERR *err;

  err = cs_register_esc_strfunc(cs, "url_escape", cgi_url_escape);
  if (err != STATUS_OK) return nerr_pass(err);
  err = cs_register_esc_strfunc(cs, "html_escape", cgi_html_escape_strfunc);
  if (err != STATUS_OK) return nerr_pass(err);
  err = cs_register_strfunc(cs, "text_html", cgi_text_html_strfunc);
  if (err != STATUS_OK) return nerr_pass(err);
  err = cs_register_esc_strfunc(cs, "js_escape", cgi_js_escape);
  if (err != STATUS_OK) return nerr_pass(err);
  err = cs_register_strfunc(cs, "html_strip", cgi_html_strip_strfunc);
  if (err != STATUS_OK) return nerr_pass(err);
  err = cs_register_esc_strfunc(cs, "url_validate", cgi_url_validate);
  if (err != STATUS_OK) return nerr_pass(err);
  return STATUS_OK;
}

NEOERR *cgi_cs_init(CGI *cgi, CSPARSE **cs)
{
  NEOERR *err;

  *cs = NULL;

  do
  {
    err = cs_init (cs, cgi->hdf);
    if (err != STATUS_OK) break;
    err = cgi_register_strfuncs(*cs);
    if (err != STATUS_OK) break;
  } while (0);

  if (err && *cs) cs_destroy(cs);
  return nerr_pass(err);
}

NEOERR *cgi_display (CGI *cgi, const char *cs_file)
{
  NEOERR *err = STATUS_OK;
  char *debug;
  CSPARSE *cs = NULL;
  STRING str;
  int do_dump = 0;
  char *t;

  string_init(&str);

  debug = hdf_get_value (cgi->hdf, "Query.debug", NULL);
  t = hdf_get_value (cgi->hdf, "Config.DumpPassword", NULL);
  if (hdf_get_int_value(cgi->hdf, "Config.DebugEnabled", 0) &&
      debug && t && !strcmp (debug, t)) do_dump = 1;

  do
  {
    err = cs_init (&cs, cgi->hdf);
    if (err != STATUS_OK) break;
    err = cgi_register_strfuncs(cs);
    if (err != STATUS_OK) break;
    err = cs_parse_file (cs, cs_file);
    if (err != STATUS_OK) break;
    if (do_dump)
    {
      cgiwrap_writef("Content-Type: text/plain\n\n");
      hdf_dump_str(cgi->hdf, "", 0, &str);
      cs_dump(cs, &str, render_cb);
      cgiwrap_writef("%s", str.buf);
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
  nerr_error_traceback(err, &str);
  cgiwrap_write(str.buf, str.len);
  cgiwrap_writef("</pre></body></html>\n");
}

void cgi_error (CGI *cgi, const char *fmt, ...)
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
  char line[4096];
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

void cgi_vredirect (CGI *cgi, int uri, const char *fmt, va_list ap)
{
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
    char *host;
    int https = 0;

    if (!strcmp(hdf_get_value(cgi->hdf, "CGI.HTTPS", "off"), "on"))
    {
      https = 1;
    }

    host = hdf_get_value (cgi->hdf, "HTTP.Host", NULL);
    if (host == NULL)
      host = hdf_get_value (cgi->hdf, "CGI.ServerName", "localhost");

    cgiwrap_writef ("Location: %s://%s", https ? "https" : "http", host);

    if ((strchr(host, ':') == NULL)) {
      int port = hdf_get_int_value(cgi->hdf, "CGI.ServerPort", 80);

      if (!((https && port == 443) || (!https && port == 80)))
      {
	cgiwrap_writef(":%d", port);
      }
    }
  }
  cgiwrap_writevf (fmt, ap);
  cgiwrap_writef ("\r\n\r\n");
  cgiwrap_writef ("Redirect page<br><br>\n");
#if 0
  /* Apparently this crashes on some computers... I don't know if its
   * legal to reuse the va_list */
  cgiwrap_writef ("  Destination: <A HREF=\"");
  cgiwrap_writevf (fmt, ap);
  cgiwrap_writef ("\">");
  cgiwrap_writevf (fmt, ap);
  cgiwrap_writef ("</A><BR>\n<BR>\n");
#endif
  cgiwrap_writef ("There is nothing to see here, please move along...");

}

void cgi_redirect (CGI *cgi, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  cgi_vredirect (cgi, 0, fmt, ap);
  va_end(ap);
  return;
}

void cgi_redirect_uri (CGI *cgi, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  cgi_vredirect (cgi, 1, fmt, ap);
  va_end(ap);
  return;
}

char *cgi_cookie_authority (CGI *cgi, const char *host)
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

/* For more information about Cookies, see:
 * The original Netscape Cookie Spec:
 * http://wp.netscape.com/newsref/std/cookie_spec.html
 *
 * HTTP State Management Mechanism
 * http://www.ietf.org/rfc/rfc2109.txt
 */

NEOERR *cgi_cookie_set (CGI *cgi, const char *name, const char *value,
                        const char *path, const char *domain,
                        const char *time_str, int persistent, int secure)
{
  NEOERR *err;
  STRING str;
  char my_time[256];

  if (path == NULL) path = "/";

  string_init(&str);
  do {
    err = string_appendf(&str, "Set-Cookie: %s=%s; path=%s", name, value, path);
    if (err) break;

    if (persistent)
    {
      if (time_str == NULL)
      {
	/* Expires in one year */
	time_t exp_date = time(NULL) + 31536000;

	strftime (my_time, 48, "%A, %d-%b-%Y 23:59:59 GMT",
	    gmtime (&exp_date));
	time_str = my_time;
      }
      err = string_appendf(&str, "; expires=%s", time_str);
      if (err) break;
    }
    if (domain)
    {
      err = string_appendf(&str, "; domain=%s", domain);
      if (err) break;
    }
    if (secure)
    {
      err = string_append(&str, "; secure");
      if (err) break;
    }
    err = string_append(&str, "\r\n");
  } while (0);
  if (err)
  {
    string_clear(&str);
    return nerr_pass(err);
  }
  cgiwrap_write(str.buf, str.len);
  string_clear(&str);
  return STATUS_OK;
}

/* This will actually issue up to three set cookies, attempting to clear
 * the damn thing. */
NEOERR *cgi_cookie_clear (CGI *cgi, const char *name, const char *domain,
                          const char *path)
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
