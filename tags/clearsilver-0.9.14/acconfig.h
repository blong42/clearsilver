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

/*
 * config file
 */

#ifndef __CS_CONFIG_H_
#define __CS_CONFIG_H_ 1

@TOP@

/* Enable support for HTML Compression (still must be enabled at run time) */
#undef HTML_COMPRESSION

/* Enable support for X Remote CGI Debugging */
#undef ENABLE_REMOTE_DEBUG

/********* SYSTEM CONFIG ***************************************************/
/* autoconf/configure should figure all of these out for you */

/* Does your system have the snprintf() call? */
#undef HAVE_SNPRINTF

/* Does your system have the vsnprintf() call? */
#undef HAVE_VSNPRINTF

/* Does your system have the strtok_r() call? */
#undef HAVE_STRTOK_R

/* Does your system have the localtime_r() call? */
#undef HAVE_LOCALTIME_R

/* Does your system have the gmtime_r() call? */
#undef HAVE_GMTIME_R

/* Does your system have the mkstemp() call? */
#undef HAVE_MKSTEMP

/* Does your system have regex.h */
#undef HAVE_REGEX

/* Does your system have pthreads? */
#undef HAVE_PTHREADS

/* Does your system have lockf ? */
#undef HAVE_LOCKF

/* Does your system have Berkeley DB v2 ? */
#undef HAVE_DB2

/* Enable support for gettext message translation */
#undef ENABLE_GETTEXT

@BOTTOM@

#endif /* __CS_CONFIG_H_ */
