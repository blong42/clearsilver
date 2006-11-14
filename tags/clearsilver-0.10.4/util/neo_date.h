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

#ifndef __NEO_DATE_H_
#define __NEO_DATE_H_ 1

#include <time.h>

__BEGIN_DECLS

/* UTC time_t -> struct tm in local timezone */
void neo_time_expand (const time_t tt, const char *timezone, struct tm *ttm);

/* local timezone struct tm -> time_t UTC */
time_t neo_time_compact (struct tm *ttm, const char *timezone);

/* To be portable... in seconds */
long neo_tz_offset(struct tm *ttm);

__END_DECLS

#endif /* __NEO_DATE_H_ */
