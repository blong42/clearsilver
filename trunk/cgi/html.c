
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include "util/neo_err.h"
#include "util/neo_str.h"
#include "html.h"

static int has_space_formatting(char *src, int slen)
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
      if (ascii_art > 3) return 1;
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

static char *EmailRe = "[^][@:;<>\\\"()[:space:][:cntrl:]]+@[-+a-zA-Z0-9].[-+a-zA-Z0-9.]+";
static char *URLRe = "((((ht|f)tp)|mailto):(//)?[^ >\"\t]*|www\\.[-a-z0-9.]+)[^ .,;\t\">]";

static NEOERR *split_and_convert (char *src, int slen, STRING *out, int newlines)
{
  NEOERR *err = STATUS_OK;
  static int compiled = 0;
  static regex_t email_re, url_re;
  regmatch_t email_match, url_match;
  int errcode;
  char buf[256], *ptr;
  struct _parts *parts;
  int part_count;
  int part;
  int x, i;

  if (!compiled)
  {
    if ((errcode = regcomp (&email_re, EmailRe, REG_ICASE | REG_EXTENDED)))
    {
      regerror (errcode, &email_re, buf, sizeof(buf));
      return nerr_raise (NERR_PARSE, "Unable to compile EmailRE: %s", buf);
    }
    if ((errcode = regcomp (&url_re, URLRe, REG_ICASE | REG_EXTENDED)))
    {
      regerror (errcode, &url_re, buf, sizeof(buf));
      return nerr_raise (NERR_PARSE, "Unable to compile URLRe: %s", buf);
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
  if (regexec (&url_re, src+x, 1, &url_match, 0) != 0)
  {
    url_match.rm_so = -1;
    url_match.rm_eo = -1;
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
      parts[part].begin = x + url_match.rm_so;
      parts[part].end = x + url_match.rm_eo;
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
	if ((email_match.rm_so != -1) && (x > email_match.rm_so))
	{
	  if (regexec (&email_re, src+x, 1, &email_match, 0) != 0)
	  {
	    email_match.rm_so = -1;
	    email_match.rm_eo = -1;
	  }
	}
      }
    }
    else
    {
      parts[part].begin = x + email_match.rm_so;
      parts[part].end = x + email_match.rm_eo;
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
	if ((url_match.rm_so != -1) && (x > url_match.rm_so))
	{
	  if (regexec (&url_re, src+x, 1, &url_match, 0) != 0)
	  {
	    url_match.rm_so = -1;
	    url_match.rm_eo = -1;
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
      ptr = strpbrk(src + x, "&<>\r\n");
      if (ptr == NULL)
      {
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
	  err = string_appendn (out, src + x, (ptr - src) - x);
	  if (err != STATUS_OK) break;
	  x = ptr - src;
	  if (src[x] == '&')
	    err = string_append (out, "&amp;");
	  else if (src[x] == '<')
	    err = string_append (out, "&lt;");
	  else if (src[x] == '>')
	    err = string_append (out, "&gt;");
	  else if (src[x] == '\n')
	    if (newlines) 
	      err = string_append (out, "<br>");
	    else if (x && src[x-1] == '\n')
	      err = string_append (out, "<p>");
	    else 
	      err = string_append_char (out, '\n');
	  else if (src[x] != '\r')
	    err = nerr_raise (NERR_ASSERT, "src[x] == '%c'", src[x]);
	  x++;
	}
	else
	{
	  err = string_appendn (out, src + x, parts[i].begin - x);
	  x = parts[i].begin;
	}
      }
    }
    else 
    {
      if (parts[i].type == SC_TYPE_URL)
      {
	err = string_append (out, "<a target=_top href=\"");
	if (err != STATUS_OK) break;
	err = string_appendn (out, src + x, parts[i].end - x);
	if (err != STATUS_OK) break;
	err = string_append (out, "\">");
	if (err != STATUS_OK) break;
	err = string_appendn (out, src + x, parts[i].end - x);
	if (err != STATUS_OK) break;
	err = string_append (out, "</a>");
      }
      else /* type == SC_TYPE_EMAIL */
      {
	err = string_append (out, "<a href=\"mailto:");
	if (err != STATUS_OK) break;
	err = string_appendn (out, src + x, parts[i].end - x);
	if (err != STATUS_OK) break;
	err = string_append (out, "\">");
	if (err != STATUS_OK) break;
	err = string_appendn (out, src + x, parts[i].end - x);
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
	memmove (str->buf + x, ptr, ol - i);
	x++;
	str->len -= ((i - x) + 1);
	str->buf[str->len-1] = '\0';
      }
    }
  }
}

NEOERR *convert_text_html_alloc (char *src, int slen, char **out)
{
  NEOERR *err;
  STRING out_s;

  string_init(&out_s);

  do
  {
    if (has_space_formatting (src, slen))
    {
      /* Do <pre> formatting */
      err = string_append (&out_s, "<pre>");
      if (err != STATUS_OK) break;
      err = split_and_convert(src, slen, &out_s, 0);
      if (err != STATUS_OK) break;
      err = string_append (&out_s, "</pre>");
      if (err != STATUS_OK) break;
      /* Strip white space at end of lines */
      strip_white_space_end (&out_s);
    }
    else
    {
      int nl = has_long_lines (src, slen);
      err = split_and_convert(src, slen, &out_s, !nl);
    }
  } while (0);
  if (err != STATUS_OK) 
  {
    string_clear (&out_s);
    return nerr_pass (err);
  }
  *out = out_s.buf;
  return STATUS_OK;
}
