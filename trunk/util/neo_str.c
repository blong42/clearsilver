/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "neo_err.h"
#include "neo_str.h"

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
