/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "neo_misc.h"

void ne_vwarn (char *fmt, va_list ap)
{
  char tbuf[20];
  char buf[1024];
  struct tm my_tm;
  time_t now;
  int len;

  now = time(NULL);

  localtime_r(&now, &my_tm);

  strftime(tbuf, sizeof(tbuf), "%m/%d %T", &my_tm);

  vsnprintf (buf, sizeof(buf), fmt, ap);
  len = strlen(buf);
  while (len && isspace (buf[len-1])) buf[--len] = '\0';
  fprintf (stderr, "[%s] %s\n", tbuf, buf);
}

void ne_warn (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  ne_vwarn (fmt, ap);
  va_end (ap);
}

UINT32 python_string_hash (const char *s)
{
  int len=0;
  register UINT32 x;

  x = *s << 7;
  while(*s != 0) {
    x = (1000003*x) ^ *s;
    s++;
    len++;
  }
  x ^= len;
  if(x == -1) x = -2;
  return x;
}

UINT8 *ne_stream4 (UINT8  *dest, UINT32 num)
{
  dest[0] = num & 0xFF;
  dest[1] = (num >> 8) & 0xFF;
  dest[2] = (num >> 16) & 0xFF;
  dest[3] = (num >> 24) & 0xFF;

  return dest + 4;
}

UINT8 *ne_unstream4 (UINT32 *pnum, UINT8 *src)
{
  *pnum = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

  return src + 4;
}
