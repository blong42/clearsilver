/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <regex.h>
#include "neo_err.h"
#include "neo_str.h"
#include "neo_misc.h"
#include "ulist.h"

char *neos_strip (char *s)
{
  int x;

  x = strlen(s) - 1;
  while (x>=0 && isspace(s[x])) s[x--] = '\0';

  while (*s && isspace(*s)) s++;
  
  return s;
}

void string_init (STRING *str)
{
  str->buf = NULL;
  str->len = 0;
  str->max = 0;
}

void string_clear (STRING *str)
{
  if (str->buf != NULL)
    free(str->buf);
  string_init(str);
}

static NEOERR* string_check_length (STRING *str, int l)
{
  if (str->buf == NULL)
  {
    if (l * 10 > 256)
      str->max = l * 10;
    else
      str->max = 256;
    str->buf = (char *) malloc (sizeof(char) * str->max);
    if (str->buf == NULL)
      return nerr_raise (NERR_NOMEM, "Unable to allocate render buf of size %d",
	  str->max);
  }
  else if (str->len + l >= str->max)
  {
    do
    {
      str->max *= 2;
    } while (str->len + l >= str->max);
    str->buf = (char *) realloc (str->buf, sizeof(char) * str->max);
    if (str->buf == NULL)
      return nerr_raise (NERR_NOMEM, "Unable to allocate STRING buf of size %d",
	  str->max);
  }
  return STATUS_OK;
}

NEOERR *string_append (STRING *str, char *buf)
{
  NEOERR *err;
  int l;

  l = strlen(buf);
  err = string_check_length (str, l);
  if (err != STATUS_OK) return nerr_pass (err);
  strcpy(str->buf + str->len, buf);
  str->len += l;

  return STATUS_OK;
}

NEOERR *string_appendn (STRING *str, char *buf, int l)
{
  NEOERR *err;

  err = string_check_length (str, l+1);
  if (err != STATUS_OK) return nerr_pass (err);
  strncpy(str->buf + str->len, buf, l);
  str->len += l;
  str->buf[str->len] = '\0';

  return STATUS_OK;
}

NEOERR *string_append_char (STRING *str, char c)
{
  NEOERR *err;
  err = string_check_length (str, 1);
  if (err != STATUS_OK) return nerr_pass (err);
  str->buf[str->len] = c;
  str->buf[str->len + 1] = '\0';
  str->len += 1;

  return STATUS_OK;
}

void string_array_init (STRING_ARRAY *arr)
{
  arr->entries = NULL;
  arr->count = 0;
  arr->max = 0;
}

NEOERR *string_array_split (ULIST **list, char *s, char *sep, int max)
{
  NEOERR *err;
  char *p, *n, *f;
  int sl;
  int x = 0;

  if (sep[0] == '\0') 
    return nerr_raise (NERR_ASSERT, "separator must be at least one character");

  err = uListInit (list, 10, 0);
  if (err) return nerr_pass(err);

  sl = strlen(sep);
  p = (sl == 1) ? strchr (s, sep[0]) : strstr (s, sep);
  f = s;
  while (p != NULL)
  {
    if (x >= max) break;
    *p = '\0';
    n = strdup(f);
    *p = sep[0];
    if (n) err = uListAppend (*list, n);
    else err = nerr_raise(NERR_NOMEM, 
	"Unable to allocate memory to split %s", s);
    if (err) goto split_err;
    f = p+sl;
    p = (sl == 1) ? strchr (f, sep[0]) : strstr (f, sep);
    x++;
  }
  /* Handle remainder */
  n = strdup(f);
  if (n) err = uListAppend (*list, n);
  else err = nerr_raise(NERR_NOMEM, 
      "Unable to allocate memory to split %s", s);
  if (err) goto split_err;
  return STATUS_OK;

split_err:
  uListDestroy(list, ULIST_FREE);
  return err;
}

void string_array_clear (STRING_ARRAY *arr)
{
  int x;

  for (x = 0; x < arr->count; x++)
  {
    if (arr->entries[x] != NULL) free (arr->entries[x]);
    arr->entries[x] = NULL;
  }
  free (arr->entries);
  arr->entries = NULL;
  arr->count = 0;
}

/* This works better with a C99 compliant vsnprintf, but should work ok
 * with versions that return a -1 if it overflows the buffer */
char *vsprintf_alloc (char *fmt, va_list ap)
{
  char buf[4096];
  char *b = NULL;
  int bl, size;

  size = sizeof (buf);
  bl = vsnprintf (buf, sizeof (buf), fmt, ap);
  if (bl > -1 && bl < size)
    return strdup (buf);

  if (bl > -1)
    size = bl + 1;
  else
    size *= 2;

  b = (char *) malloc (size * sizeof(char));
  if (b == NULL) return NULL;
  while (1)
  {
    bl = vsnprintf (b, size, fmt, ap);
    if (bl > -1 && bl < size)
      return b;
    size *= 2;
    b = (char *) realloc (b, size * sizeof(char));
    if (b == NULL) return NULL;
  }
}

char *sprintf_alloc (char *fmt, ...)
{
  va_list ap;
  char *r;

  va_start (ap, fmt);
  r = vsprintf_alloc (fmt, ap);
  va_end (ap);
  return r;
}

BOOL reg_search (char *re, char *str)
{
  regex_t search_re;
  int errcode;
  char buf[256];

  if ((errcode = regcomp (&search_re, re, REG_ICASE | REG_EXTENDED | REG_NOSUB)))
  {
    regerror (errcode, &search_re, buf, sizeof(buf));
    ne_warn ("Unable to compile regex %s: %s", re, buf);
    return FALSE;
  }
  errcode = regexec (&search_re, str, 0, NULL, 0);
  regfree (&search_re);
  if (errcode == 0)
    return TRUE;
  return FALSE;
}
