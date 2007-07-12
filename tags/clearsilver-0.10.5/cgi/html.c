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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_str.h"
#include "html.h"
#include "cgi.h"

static int has_space_formatting(const char *src, int slen)
{
  int spaces = 0;
  int returns = 0;
  int ascii_art = 0;
  int x = 0;

  for (x = 0; x < slen; x++)
  {
    if (src[x] == '\t') return 1;
    if (src[x] == ' ')
    {
      spaces++;
      if (x && (src[x-1] == '.'))
	spaces--;
    }
    else if (src[x] == '\n')
    {
      spaces = 0;
      returns++;
    }
    else if (strchr ("/\\<>:[]!@#$%^&*()|", src[x]))
    {
      ascii_art++;
      if (ascii_art > 3) return 2;
    }
    else if (src[x] != '\r')
    {
      if (returns > 2) return 1;
      if (spaces > 2) return 1;
      returns = 0;
      spaces = 0;
      ascii_art = 0;
    }
  }

  return 0;
}

/*
static int has_long_lines (char *s, int l)
{
  char *ptr;
  int x = 0;

  while (x < l)
  {
    ptr = strchr (s + x, '\n');
    if (ptr == NULL)
    {
      if (l - x > 75) return 1;
      return 0;
    }
    if (ptr - (s + x) > 75) return 1;
    x = ptr - s + 1;
  }
  return 0;
}
*/

/* The first step is to actually find all of the URLs and email
 * addresses using our handy regular expressions.  We then mark these,
 * and then go through convert non-special areas with straight
 * text->html escapes, and convert special parts as special parts
 */
struct _parts {
  int begin;
  int end;
  int type;
};

#define SC_TYPE_TEXT  1
#define SC_TYPE_URL   2
#define SC_TYPE_EMAIL 3

static char *EmailRe = "[^][@:;<>\\\"()[:space:][:cntrl:]]+@[-+a-zA-Z0-9]+\\.[-+a-zA-Z0-9\\.]+[-+a-zA-Z0-9]";
static char *URLRe = "((http|https|ftp|mailto):(//)?[^[:space:]>\"\t]*|www\\.[-a-z0-9\\.]+)[^[:space:];\t\">]*";

static NEOERR *split_and_convert (const char *src, int slen,
                                  STRING *out, HTML_CONVERT_OPTS *opts)
{
  NEOERR *err = STATUS_OK;
  static int compiled = 0;
  static regex_t email_re, url_re;
  regmatch_t email_match, url_match;
  int errcode;
  char *ptr, *esc;
  char errbuf[256];
  struct _parts *parts;
  int part_count;
  int part;
  int x, i;
  int spaces = 0;

  if (!compiled)
  {
    if ((errcode = regcomp (&email_re, EmailRe, REG_ICASE | REG_EXTENDED)))
    {
      regerror (errcode, &email_re, errbuf, sizeof(errbuf));
      return nerr_raise (NERR_PARSE, "Unable to compile EmailRE: %s", errbuf);
    }
    if ((errcode = regcomp (&url_re, URLRe, REG_ICASE | REG_EXTENDED)))
    {
      regerror (errcode, &url_re, errbuf, sizeof(errbuf));
      return nerr_raise (NERR_PARSE, "Unable to compile URLRe: %s", errbuf);
    }
    compiled = 1;
  }

  part_count = 20;
  parts = (struct _parts *) malloc (sizeof(struct _parts) * part_count);
  part = 0;

  x = 0;
  if (regexec (&email_re, src+x, 1, &email_match, 0) != 0)
  {
    email_match.rm_so = -1;
    email_match.rm_eo = -1;
  }
  else
  {
    email_match.rm_so += x;
    email_match.rm_eo += x;
  }
  if (regexec (&url_re, src+x, 1, &url_match, 0) != 0)
  {
    url_match.rm_so = -1;
    url_match.rm_eo = -1;
  }
  else
  {
    url_match.rm_so += x;
    url_match.rm_eo += x;
  }
  while ((x < slen) && !((email_match.rm_so == -1) && (url_match.rm_so == -1)))
  {
    if (part >= part_count)
    {
      part_count *= 2;
      parts = (struct _parts *) realloc (parts, sizeof(struct _parts) * part_count);
    }
    if ((url_match.rm_so != -1) && ((email_match.rm_so == -1) || (url_match.rm_so <= email_match.rm_so)))
    {
      parts[part].begin = url_match.rm_so;
      parts[part].end = url_match.rm_eo;
      parts[part].type = SC_TYPE_URL;
      x = parts[part].end + 1;
      part++;
      if (x < slen)
      {
	if (regexec (&url_re, src+x, 1, &url_match, 0) != 0)
	{
	  url_match.rm_so = -1;
	  url_match.rm_eo = -1;
	}
	else
	{
	  url_match.rm_so += x;
	  url_match.rm_eo += x;
	}
	if ((email_match.rm_so != -1) && (x > email_match.rm_so))
	{
	  if (regexec (&email_re, src+x, 1, &email_match, 0) != 0)
	  {
	    email_match.rm_so = -1;
	    email_match.rm_eo = -1;
	  }
	  else
	  {
	    email_match.rm_so += x;
	    email_match.rm_eo += x;
	  }
	}
      }
    }
    else
    {
      parts[part].begin = email_match.rm_so;
      parts[part].end = email_match.rm_eo;
      parts[part].type = SC_TYPE_EMAIL;
      x = parts[part].end + 1;
      part++;
      if (x < slen)
      {
	if (regexec (&email_re, src+x, 1, &email_match, 0) != 0)
	{
	  email_match.rm_so = -1;
	  email_match.rm_eo = -1;
	}
	else
	{
	  email_match.rm_so += x;
	  email_match.rm_eo += x;
	}
	if ((url_match.rm_so != -1) && (x > url_match.rm_so))
	{
	  if (regexec (&url_re, src+x, 1, &url_match, 0) != 0)
	  {
	    url_match.rm_so = -1;
	    url_match.rm_eo = -1;
	  }
	  else
	  {
	    url_match.rm_so += x;
	    url_match.rm_eo += x;
	  }
	}
      }
    }
  }

  i = 0;
  x = 0;
  while (x < slen)
  {
    if ((i >= part) || (x < parts[i].begin))
    {
      ptr = strpbrk(src + x, "&<>\r\n ");
      if (ptr == NULL)
      {
	if (spaces)
	{
	  int sp;
	  for (sp = 0; sp < spaces - 1; sp++)
	  {
	    err = string_append (out, "&nbsp;");
	    if (err != STATUS_OK) break;
	  }
	  if (err != STATUS_OK) break;
	  err = string_append_char (out, ' ');
	}
	spaces = 0;
	if (i < part)
	{
	  err = string_appendn (out, src + x, parts[i].begin - x);
	  x = parts[i].begin;
	}
	else
	{
	  err = string_append (out, src + x);
	  x = slen;
	}
      }
      else
      {
	if ((i >= part) || ((ptr - src) < parts[i].begin))
	{
	  if (spaces)
	  {
	    int sp;
	    for (sp = 0; sp < spaces - 1; sp++)
	    {
	      err = string_append (out, "&nbsp;");
	      if (err != STATUS_OK) break;
	    }
	    if (err != STATUS_OK) break;
	    err = string_append_char (out, ' ');
	  }
	  spaces = 0;
	  err = string_appendn (out, src + x, (ptr - src) - x);
	  if (err != STATUS_OK) break;
	  x = ptr - src;
	  if (src[x] == ' ')
	  {
	    if (opts->space_convert)
	    {
	      spaces++;
	    }
	    else
	      err = string_append_char (out, ' ');
	  }
	  else
	  {
	    if (src[x] != '\n' && spaces)
	    {
	      int sp;
	      for (sp = 0; sp < spaces - 1; sp++)
	      {
		err = string_append (out, "&nbsp;");
		if (err != STATUS_OK) break;
	      }
	      if (err != STATUS_OK) break;
	      err = string_append_char (out, ' ');
	    }
	    spaces = 0;

	    if (src[x] == '&')
	      err = string_append (out, "&amp;");
	    else if (src[x] == '<')
	      err = string_append (out, "&lt;");
	    else if (src[x] == '>')
	      err = string_append (out, "&gt;");
	    else if (src[x] == '\n')
	      if (opts->newlines_convert)
		err = string_append (out, "<br/>\n");
	      else if (x && src[x-1] == '\n')
		err = string_append (out, "<p/>\n");
	      else
		err = string_append_char (out, '\n');
	    else if (src[x] != '\r')
	      err = nerr_raise (NERR_ASSERT, "src[x] == '%c'", src[x]);
	  }
	  x++;
	}
	else
	{
	  if (spaces)
	  {
	    int sp;
	    for (sp = 0; sp < spaces - 1; sp++)
	    {
	      err = string_append (out, "&nbsp;");
	      if (err != STATUS_OK) break;
	    }
	    if (err != STATUS_OK) break;
	    err = string_append_char (out, ' ');
	  }
	  spaces = 0;
	  err = string_appendn (out, src + x, parts[i].begin - x);
	  x = parts[i].begin;
	}
      }
    }
    else
    {
      if (spaces)
      {
	int sp;
	for (sp = 0; sp < spaces - 1; sp++)
	{
	  err = string_append (out, "&nbsp;");
	  if (err != STATUS_OK) break;
	}
	if (err != STATUS_OK) break;
	err = string_append_char (out, ' ');
      }
      spaces = 0;
      if (parts[i].type == SC_TYPE_URL)
      {
        char last_char = src[parts[i].end-1];
        int suffix=0;
        if (last_char == '.' || last_char == ',') { suffix=1; }
	err = string_append (out, " <a ");
	if (err != STATUS_OK) break;
	if (opts->url_class)
	{
	    err = string_appendf (out, "class=%s ", opts->url_class);
	    if (err) break;
	}
	if (opts->url_target)
	{
	  err = string_appendf (out, "target=\"%s\" ", opts->url_target);
	  if (err) break;
	}
	err = string_append(out, "href=\"");
	if (err) break;
	if (opts->bounce_url)
	{
	  char *url, *esc_url, *new_url;
	  int url_len;
	  if (!strncasecmp(src + x, "www.", 4))
	  {
	    url_len = 7 + parts[i].end - x - suffix;
	    url = (char *) malloc(url_len+1);
	    if (url == NULL)
	    {
	      err = nerr_raise(NERR_NOMEM,
		  "Unable to allocate memory to convert url");
	      break;
	    }
	    strcpy(url, "http://");
	    strncat(url, src + x, parts[i].end - x - suffix);
	  }
	  else
	  {
	    url_len = parts[i].end - x - suffix;
	    url = (char *) malloc(url_len+1);
	    if (url == NULL)
	    {
	      err = nerr_raise(NERR_NOMEM,
		  "Unable to allocate memory to convert url");
	      break;
	    }
	    strncpy(url, src + x, parts[i].end - x - suffix);
	    url[url_len] = '\0';
	  }
	  err = cgi_url_escape(url, &esc_url);
	  free(url);
	  if (err) {
	    free(esc_url);
	    break;
	  }

	  new_url = sprintf_alloc(opts->bounce_url, esc_url);
	  free(esc_url);
	  if (new_url == NULL)
	  {
	    err = nerr_raise(NERR_NOMEM, "Unable to allocate memory to convert url");
	    break;
	  }
	  err = string_append (out, new_url);
	  free(new_url);
	  if (err) break;
	}
	else
	{
	  if (!strncasecmp(src + x, "www.", 4))
	  {
	    err = string_append (out, "http://");
	    if (err != STATUS_OK) break;
	  }
	  err = string_appendn (out, src + x, parts[i].end - x - suffix);
	  if (err != STATUS_OK) break;
	}
	err = string_append (out, "\">");
	if (err != STATUS_OK) break;
        if (opts->link_name) {
          err = html_escape_alloc((opts->link_name),
                                  strlen(opts->link_name), &esc);
        } else {
          err = html_escape_alloc((src + x), parts[i].end - x - suffix, &esc);
        }
	if (err != STATUS_OK) break;
	err = string_append (out, esc);
	free(esc);
	if (err != STATUS_OK) break;
	err = string_append (out, "</a>");
        if (suffix) {
            err  = string_appendn(out,src + parts[i].end - 1,1);
	    if (err != STATUS_OK) break;
        }
      }
      else /* type == SC_TYPE_EMAIL */
      {
	err = string_append (out, "<a ");
	if (err != STATUS_OK) break;
	if (opts->mailto_class)
	{
	    err = string_appendf (out, "class=%s ", opts->mailto_class);
	    if (err) break;
	}
	err = string_append(out, "href=\"mailto:");
	if (err) break;
	err = string_appendn (out, src + x, parts[i].end - x);
	if (err != STATUS_OK) break;
	err = string_append (out, "\">");
	if (err != STATUS_OK) break;
	err = html_escape_alloc(src + x, parts[i].end - x, &esc);
	if (err != STATUS_OK) break;
	err = string_append (out, esc);
	free(esc);
	if (err != STATUS_OK) break;
	err = string_append (out, "</a>");
      }
      x = parts[i].end;
      i++;
    }
    if (err != STATUS_OK) break;
  }
  free (parts);
  return err;
}

static void strip_white_space_end (STRING *str)
{
  int x = 0;
  int ol = str->len;
  char *ptr;
  int i;

  while (x < str->len)
  {
    ptr = strchr(str->buf + x, '\n');
    if (ptr == NULL)
    {
      /* just strip the white space at the end of the string */
      ol = strlen(str->buf);
      while (ol && isspace(str->buf[ol-1]))
      {
	str->buf[ol - 1] = '\0';
	ol--;
      }
      str->len = ol;
      return;
    }
    else
    {
      x = i = ptr - str->buf;
      if (x)
      {
	x--;
	while (x && isspace(str->buf[x]) && (str->buf[x] != '\n')) x--;
	if (x) x++;
	memmove (str->buf + x, ptr, ol - i + 1);
	x++;
	str->len -= ((i - x) + 1);
	str->buf[str->len] = '\0';
	ol = str->len;
      }
    }
  }
}

NEOERR *convert_text_html_alloc (const char *src, int slen,
                                 char **out)
{
    return nerr_pass(convert_text_html_alloc_options(src, slen, out, NULL));
}

NEOERR *convert_text_html_alloc_options (const char *src, int slen,
                                         char **out,
                                         HTML_CONVERT_OPTS *opts)
{
  NEOERR *err;
  STRING out_s;
  int formatting = 0;
  HTML_CONVERT_OPTS my_opts;

  string_init(&out_s);

  if (opts == NULL)
  {
    opts = &my_opts;
    opts->bounce_url = NULL;
    opts->url_class = NULL;
    opts->url_target = "_blank";
    opts->mailto_class = NULL;
    opts->long_lines = 0;
    opts->space_convert = 0;
    opts->newlines_convert = 1;
    opts->longline_width = 75; /* This hasn't been used in a while, actually */
    opts->check_ascii_art = 1;
    opts->link_name = NULL;
  }

  do
  {
    if  (opts->check_ascii_art)
    {
	formatting = has_space_formatting (src, slen);
	if (formatting) opts->space_convert = 1;
    }
    if (formatting == 2)
    {
      /* Do <pre> formatting */
      opts->newlines_convert = 1;
      err = string_append (&out_s, "<tt>");
      if (err != STATUS_OK) break;
      err = split_and_convert(src, slen, &out_s, opts);
      if (err != STATUS_OK) break;
      err = string_append (&out_s, "</tt>");
      if (err != STATUS_OK) break;
      /* Strip white space at end of lines */
      strip_white_space_end (&out_s);
    }
    else
    {
      /* int nl = has_long_lines (src, slen); */
      err = split_and_convert(src, slen, &out_s, opts);
    }
  } while (0);
  if (err != STATUS_OK)
  {
    string_clear (&out_s);
    return nerr_pass (err);
  }
  if (out_s.buf == NULL)
  {
    *out = strdup("");
  }
  else
  {
    *out = out_s.buf;
  }
  return STATUS_OK;
}

NEOERR *html_escape_alloc (const char *src, int slen,
                           char **out)
{
  return nerr_pass(neos_html_escape(src, slen, out));
}

/* Replace ampersand with iso-8859-1 character code */
static unsigned char _expand_amp_8859_1_char (const char *s)
{
  if (s[0] == '\0')
    return 0;

  switch (s[0]) {
    case '#':
      if (s[1] == 'x') return strtol (s+2, NULL, 16);
      return strtol (s+1, NULL, 10);
    case 'a':
      if (!strcmp(s, "agrave")) return 0xe0; /* à */
      if (!strcmp(s, "aacute")) return 0xe1; /* á */
      if (!strcmp(s, "acirc")) return 0xe2; /* â */
      if (!strcmp(s, "atilde")) return 0xe3; /* ã */
      if (!strcmp(s, "auml")) return 0xe4; /* ä */
      if (!strcmp(s, "aring")) return 0xe5; /* å */
      if (!strcmp(s, "aelig")) return 0xe6; /* æ */
      if (!strcmp(s, "amp")) return '&';
      return 0;
    case 'c':
      if (!strcmp(s, "ccedil")) return 0xe7; /* ç */
      return 0;
    case 'e':
      if (!strcmp(s, "egrave")) return 0xe8; /* è */
      if (!strcmp(s, "eacute")) return 0xe9; /* é */
      if (!strcmp(s, "ecirc")) return 0xea; /* ê */
      if (!strcmp(s, "euml")) return 0xeb; /* ë */
      if (!strcmp(s, "eth")) return 0xf0; /* ð */
      return 0;
    case 'i':
      if (!strcmp(s, "igrave")) return 0xec; /* ì */
      if (!strcmp(s, "iacute")) return 0xed; /* í */
      if (!strcmp(s, "icirc")) return 0xee; /* î */
      if (!strcmp(s, "iuml")) return 0xef; /* ï */
      return 0;
    case 'g':
      if (!strcmp(s, "gt")) return '>';
      return 0;
    case 'l':
      if (!strcmp(s, "lt")) return '<';
      return 0;
    case 'n':
      if (!strcmp(s, "ntilde")) return 0xf1; /* ñ */
      if (!strcmp(s, "nbsp")) return ' ';
      return 0;
    case 'o':
      if (!strcmp(s, "ograve")) return 0xf2; /* ò */
      if (!strcmp(s, "oacute")) return 0xf3; /* ó */
      if (!strcmp(s, "ocirc")) return 0xf4; /* ô */
      if (!strcmp(s, "otilde")) return 0xf5; /* õ */
      if (!strcmp(s, "ouml")) return 0xf6; /* ö */
      if (!strcmp(s, "oslash")) return 0xf8; /* ø */
      return 0;
    case 'q': /* quot */
      if (!strcmp(s, "quot")) return '"';
      return 0;
    case 's':
      if (!strcmp(s, "szlig")) return 0xdf; /* ß */
      return 0;
    case 't':
      if (!strcmp(s, "thorn")) return 0xfe; /* þ */
      return 0;
    case 'u':
      if (!strcmp(s, "ugrave")) return 0xf9; /* ù */
      if (!strcmp(s, "uacute")) return 0xfa; /* ú */
      if (!strcmp(s, "ucirc")) return 0xfb; /* û */
      if (!strcmp(s, "uuml")) return 0xfc; /* ü */
      return 0;
    case 'y':
      if (!strcmp(s, "yacute")) return 0xfd; /* ý */

  }
  return 0;
}

char *html_expand_amp_8859_1(const char *amp,
                                      char *buf)
{
  unsigned char ch;

  ch = _expand_amp_8859_1_char(amp);
  if (ch == '\0')
  {
    if (!strcmp(amp, "copy")) return "(C)";
    return "";
  }
  else {
    buf[0] = (char)ch;
    buf[1] = '\0';
    return buf;
  }
}

NEOERR *html_strip_alloc(const char *src, int slen,
                         char **out)
{
  NEOERR *err = STATUS_OK;
  STRING out_s;
  int x = 0;
  int strip_match = -1;
  int state = 0;
  char amp[10];
  int amp_start = 0;
  char buf[10];
  int ampl = 0;

  string_init(&out_s);
  err = string_append (&out_s, "");
  if (err) return nerr_pass (err);

  while (x < slen)
  {
    switch (state) {
      case 0:
	/* Default */
	if (src[x] == '&')
	{
	  state = 3;
	  ampl = 0;
	  amp_start = x;
	}
	else if (src[x] == '<')
	{
	  state = 1;
	}
	else
	{
	  if (strip_match == -1)
	  {
	    err = string_append_char(&out_s, src[x]);
	    if (err) break;
	  }
	}
	x++;
	break;
      case 1:
	/* Starting TAG */
	if (src[x] == '>')
	{
	  state = 0;
	}
	else if (src[x] == '/')
	{
	}
	else
	{
	}
	x++;
	break;
      case 2:
	/* In TAG */
	if (src[x] == '>')
	{
	  state = 0;
	}
	x++;
	break;
      case 3:
	/* In AMP */
	if (src[x] == ';')
	{
	  amp[ampl] = '\0';
	  state = 0;
	  err = string_append(&out_s, html_expand_amp_8859_1(amp, buf));
	  if (err) break;
	}
	else
	{
	  if (ampl < sizeof(amp)-1)
	    amp[ampl++] = tolower(src[x]);
	  else
	  {
	    /* broken html... just back up */
	    x = amp_start;
	    err = string_append_char(&out_s, src[x]);
	    if (err) break;
	    state = 0;
	  }
	}
	x++;
	break;
    }
    if (err) break;
  }


  if (err)
  {
    string_clear (&out_s);
    return nerr_pass (err);
  }
  *out = out_s.buf;
  return STATUS_OK;
}
