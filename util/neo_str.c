/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
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

NEOERR *string_append (STRING *str, char *buf)
{
  int l;

  l = strlen(buf);
  if (str->buf == NULL)
  {
    str->max = l * 10;
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
      return nerr_raise (NERR_NOMEM, "Unable to allocate render buf of size %d",
	  str->max);
  }
  strcpy(str->buf + str->len, buf);
  str->len += l;

  return STATUS_OK;
}


