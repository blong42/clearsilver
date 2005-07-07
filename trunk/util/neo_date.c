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
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "util/neo_misc.h"
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

static int time_set_tz (const char *timezone)
{
  snprintf (TzBuf, sizeof(TzBuf), "TZ=%s", timezone);
  putenv(TzBuf);
  tzset();
  return 0;
}

void neo_time_expand (const time_t tt, const char *timezone, struct tm *ttm)
{
  time_set_tz (timezone);
  localtime_r (&tt, ttm);
}

time_t neo_time_compact (struct tm *ttm, const char *timezone)
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
  /* We assume here that HAVE_TM_ZONE implies HAVE_TM_GMTOFF and
   * HAVE_TZNAME implies HAVE_TIMEZONE since AC_STRUCT_TIMEZONE defines
   * the former and not the latter */
#if defined(HAVE_TM_ZONE)
  return ttm->tm_gmtoff;
#elif defined(HAVE_TZNAME)
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

