
#include <time.h>
#include <stdio.h>
#include "cgi.h"
#include "date.h"
#include "util/neo_err.h"
#include "util/neo_hdf.h"
#include "util/neo_date.h"

/* 
 * prefix.sec
 * prefix.min
 * prefix.hour  - 12 hour
 * prefix.am - 1 if AM
 * prefix.24hour - 24 hour
 * prefix.mday
 * prefix.mon   - numeric month
 * prefix.year  - full year (ie, 4 digits)
 * prefix.wday  - day of the week
 *
 */

NEOERR *export_date_tm (HDF *data, char *prefix, struct tm *ttm)
{
  NEOERR *err;
  HDF *obj;
  int hour, am = 1;
  char buf[256];

  err = hdf_set_value (data, prefix, "");
  if (err) return nerr_pass(err);
  obj = hdf_get_obj (data, prefix);

  snprintf (buf, sizeof(buf), "%02d", ttm->tm_sec);
  err = hdf_set_value (obj, "sec", buf);
  if (err) return nerr_pass(err);
  snprintf (buf, sizeof(buf), "%02d", ttm->tm_min);
  err = hdf_set_value (obj, "min", buf);
  if (err) return nerr_pass(err);
  err = hdf_set_int_value (obj, "24hour", ttm->tm_hour);
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
  err = hdf_set_int_value (obj, "wday", ttm->tm_wday);
  if (err) return nerr_pass(err);

  return STATUS_OK;
}

NEOERR *export_date_time_t (HDF *data, char *prefix, char *timezone, time_t tt)
{
  struct tm ttm;

  neo_time_expand (tt, timezone, &ttm);
  return nerr_pass (export_date_tm (data, prefix, &ttm));
}
