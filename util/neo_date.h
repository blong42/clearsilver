/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef _NEO_DATE_H_
#define _NEO_DATE_H_ 1

#include "osdep.h"
#include <time.h>

__BEGIN_DECLS

/* UTC time_t -> struct tm in local timezone */
void neo_time_expand (const time_t tt, char *timezone, struct tm *ttm);

/* local timezone struct tm -> time_t UTC */
time_t neo_time_compact (struct tm *ttm, char *timezone);

__END_DECLS

#endif /* _NEO_DATE_H_ */
