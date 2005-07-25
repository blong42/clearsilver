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

#ifndef __DATE_H_
#define __DATE_H_ 1

#include <time.h>

__BEGIN_DECLS

NEOERR *export_date_tm (HDF *obj, const char *prefix, struct tm *ttm);
NEOERR *export_date_time_t (HDF *obj, const char *prefix, const char *timezone, 
                            time_t tt);

__END_DECLS

#endif /* __DATE_H_ */
