
/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __CLEARSILVER_H_
#define __CLEARSILVER_H_ 1

#include "cs_config.h"

#include <stdlib.h>
#include <sys/stat.h>

/* Base libraries */
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_date.h"
#include "util/neo_files.h"
#include "util/neo_hash.h"
#include "util/neo_hdf.h"
#include "util/neo_rand.h"
#include "util/neo_net.h"
#include "util/neo_server.h"
#include "util/neo_str.h"
#include "util/ulist.h"
#include "util/wildmat.h"
#include "util/filter.h"

#ifdef HAVE_LOCKF
# include "util/ulocks.h"
# include "util/rcfs.h"

/* These are dependent on the pthread locking code in ulocks */
# ifdef HAVE_PTHREADS
#  include "util/skiplist.h"
#  include "util/dict.h"
# endif
#endif

/* This is dependent on Berkeley DB v2 */
#ifdef HAVE_DB2
# include "util/wdb.h"
#endif

/* The ClearSilver Template language */
#include "cs/cs.h"

/* The ClearSilver CGI connector */
#include "cgi/cgi.h"
#include "cgi/cgiwrap.h"
#include "cgi/date.h"
#include "cgi/html.h"

#endif /* __CLEARSILVER_H_ */
