/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "neo_date.h"

/* This is pretty much a HACK.  Eventually, we might bring the parsing
 * and stuff into this library (we can use the public domain source code
 * from ftp://elsie.nci.nih.gov/pub/ as a base)
 *
 * For now, we just do a putenv(TZ)... which sucks, especially since
 * many versions of putenv do a strdup... and then leak the memory the
 * next time you putenv the same var.
 */

/* Since this is set to a partial filename and TZ=, it can't be bigger
 * than this */
static char TzBuf[_POSIX_PATH_MAX + 4];

static int time_set_tz (char *timezone)
{
  snprintf (TzBuf, sizeof(TzBuf), "TZ=%s", timezone);
  putenv(TzBuf);
  tzset();
  return 0;
}

void neo_time_expand (const time_t tt, char *timezone, struct tm *ttm)
{
  time_set_tz (timezone);
#ifdef __WINDOWS_GCC__
  ttm = localtime(&tt);
#else
  localtime_r (&tt, ttm);
#endif
}

time_t neo_time_compact (struct tm *ttm, char *timezone)
{
  time_t r;
  int save_isdst = ttm->tm_isdst;

  time_set_tz (timezone);
  ttm->tm_isdst = -1;
  r = mktime(ttm);
  ttm->tm_isdst = save_isdst;
  return r;
}

/* Hefted from NCSA HTTPd src/util.c -- What a pain in the ass. */
long neo_tz_offset(struct tm *ttm) {
#if defined(HAVE_GMTOFF)
  return ttm->tm_gmtoff;
#elif defined(HAVE_TIMEZONE)
  long tz;
  tz = - timezone;
  if(ttm->tm_isdst)
    tz += 3600;
  return tz;
#else
  long tz;
  struct tm loc_tm, gmt_tm;
  time_t tt; 

  /* We probably shouldn't use the _r versions here since this
   * is for older platforms... */
  tt = time(NULL);
  localtime_r(&tt, &loc_tm);
  gmtime_r(&tt, &gmt_tm);
  tz = mktime(&loc_tm) - mktime(&gmt_tm);
  return tz;
#endif /* GMT OFFSet Crap */
}

