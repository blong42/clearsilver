/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __DATE_H_
#define __DATE_H_ 1

#include <time.h>

NEOERR *export_date_tm (HDF *obj, char *prefix, struct tm *ttm);
NEOERR *export_date_time_t (HDF *obj, char *prefix, char *timezone, time_t tt);

#endif /* __DATE_H_ */
