/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include "neo_err.h"
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

UINT8 *ne_stream2 (UINT8  *dest, UINT16 num)
{
  dest[0] = num & 0xFF;
  dest[1] = (num >> 8) & 0xFF;

  return dest + 2;
}

UINT8 *ne_unstream4 (UINT32 *pnum, UINT8 *src)
{
  *pnum = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);

  return src + 4;
}

UINT8 *ne_unstream2 (UINT16 *pnum, UINT8 *src)
{
  *pnum = src[0] | (src[1] << 8);

  return src + 2;
}

/* This handles strings of less than 256 bytes */
UINT8 *ne_unstream_str (char *s, int l, UINT8 *src)
{
  UINT8 sl;

  sl = src[0];
  if (sl+1 < l)
    l = sl;
  memcpy (s, src+1, sl);
  s[l] = '\0';
  return src+sl+1;
}

UINT8 *ne_stream_str (UINT8 *dest, char *s, int l)
{
  if (l > 255)
  {
    ne_warn("WARNING: calling ne_stream_str with l>255");
    l = 255;
  }
  dest[0] = l;
  memcpy (dest+1, s, l);
  return dest+l+1;
}

double ne_timef (void)
{
  double f = 0;
  struct timeval tv;
  int ret;

  ret = gettimeofday(&tv, NULL);
  if (ret == 0)
  {
    f = tv.tv_sec + (tv.tv_usec / 1000000.0);
  }
  return f;
}

NEOERR *ne_mkdirs (char *path, mode_t mode)
{
  char mypath[_POSIX_PATH_MAX];
  int x;
  int r;

  strncpy (mypath, path, sizeof(mypath));
  x = strlen(mypath);
  if ((x < sizeof(mypath)) && (mypath[x-1] != '/'))
  {
    mypath[x] = '/';
    mypath[x+1] = '\0';
  }

  for (x = 1; mypath[x]; x++)
  {
    if (mypath[x] == '/')
    {
      mypath[x] = '\0';
      r = mkdir (mypath, mode);
      if (r == -1 && errno != EEXIST)
      {
	return nerr_raise(NERR_SYSTEM, "ne_mkdirs: mkdir(%s, %x) failed: %s", mypath, mode, strerror(errno));
      }
      mypath[x] = '/';
    }
  }
  return STATUS_OK;
}
