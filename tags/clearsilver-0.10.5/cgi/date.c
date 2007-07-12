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

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_hdf.h"
#include "util/neo_date.h"
#include "cgi.h"
#include "date.h"

/* 
 * prefix.sec
 * prefix.min
 * prefix.hour  - 12 hour
 * prefix.am - 1 if AM
 * prefix.24hour - 24 hour
 * prefix.mday
 * prefix.mon   - numeric month
 * prefix.year  - full year (ie, 4 digits)
 * prefix.2yr  - year (2 digits)
 * prefix.wday  - day of the week
 * prefix.tzoffset - hhmm from UTC
 *
 */

NEOERR *export_date_tm (HDF *data, const char *prefix, struct tm *ttm)
{
  NEOERR *err;
  HDF *obj;
  int hour, am = 1;
  char buf[256];
  int tzoffset_seconds = 0;
  int tzoffset = 0;
  char tzsign = '+';

  obj = hdf_get_obj (data, prefix);
  if (obj == NULL)
  {
    err = hdf_set_value (data, prefix, "");
    if (err) return nerr_pass(err);
    obj = hdf_get_obj (data, prefix);
  }

  snprintf (buf, sizeof(buf), "%02d", ttm->tm_sec);
  err = hdf_set_value (obj, "sec", buf);
  if (err) return nerr_pass(err);
  snprintf (buf, sizeof(buf), "%02d", ttm->tm_min);
  err = hdf_set_value (obj, "min", buf);
  if (err) return nerr_pass(err);
  snprintf (buf, sizeof(buf), "%02d", ttm->tm_hour);
  err = hdf_set_value (obj, "24hour", buf);
  if (err) return nerr_pass(err);
  hour = ttm->tm_hour;
  if (hour == 0)
  {
    hour = 12;
  }
  else if (hour == 12)
  {
    am = 0;
  }
  else if (hour > 12)
  {
    am = 0;
    hour -= 12;
  }
  err = hdf_set_int_value (obj, "hour", hour);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "am", am);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "mday", ttm->tm_mday);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "mon", ttm->tm_mon + 1);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "year", ttm->tm_year + 1900);
  if (err) return nerr_pass(err);
  snprintf(buf, sizeof(buf), "%02d", ttm->tm_year % 100);
  err = hdf_set_value (obj, "2yr", buf);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "wday", ttm->tm_wday);
  if (err) return nerr_pass(err);
  // neo_tz_offset() returns offset from GMT in seconds
  tzoffset_seconds = neo_tz_offset(ttm);
  tzoffset = tzoffset_seconds / 60;
  if (tzoffset < 0)
  {
    tzoffset *= -1;
    tzsign = '-';
  }
  snprintf(buf, sizeof(buf), "%c%02d%02d", tzsign, tzoffset / 60, tzoffset % 60);
  err = hdf_set_value (obj, "tzoffset", buf);
  if (err) return nerr_pass(err);

  return STATUS_OK;
}

NEOERR *export_date_time_t (HDF *data, const char *prefix, const char *timezone,
                            time_t tt)
{
  struct tm ttm;

  neo_time_expand (tt, timezone, &ttm);
  return nerr_pass (export_date_tm (data, prefix, &ttm));
}

/* from httpd util.c : made infamous with Roy owes Rob beer. */
static char *months[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

int find_month(char *mon) {
  register int x;

  for(x=0;x<12;x++)
    if(!strcmp(months[x],mon))
      return x;
  return -1;
}

int later_than(struct tm *lms, char *ims) {
  char *ip;
  char mname[256];
  int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0, x;

  /* Whatever format we're looking at, it will start
   * with weekday. */
  /* Skip to first space. */
  if(!(ip = strchr(ims,' ')))
    return 0;
  else
    while(isspace(*ip))
      ++ip;

  if(isalpha(*ip)) {
    /* ctime */
    sscanf(ip,"%25s %d %d:%d:%d %d",mname,&day,&hour,&min,&sec,&year);
  }
  else if(ip[2] == '-') {
    /* RFC 850 (normal HTTP) */
    char t[256];
    sscanf(ip,"%s %d:%d:%d",t,&hour,&min,&sec);
    t[2] = '\0';
    day = atoi(t);
    t[6] = '\0';
    strcpy(mname,&t[3]);
    x = atoi(&t[7]);
    /* Prevent
     * wraparound
     * from
     * ambiguity
     * */
    if(x < 70)
      x += 100;
    year = 1900 + x;
  }
  else {
    /* RFC 822 */
    sscanf(ip,"%d %s %d %d:%d:%d",&day,mname,&year,&hour,&min,&sec);
  }
  month = find_month(mname);

  if((x = (1900+lms->tm_year) - year))
    return x < 0;
  if((x = lms->tm_mon - month))
    return x < 0;
  if((x = lms->tm_mday - day))
    return x < 0;
  if((x = lms->tm_hour - hour))
    return x < 0;
  if((x = lms->tm_min - min))
    return x < 0;
  if((x = lms->tm_sec - sec))
    return x < 0;

  return 1;
}

