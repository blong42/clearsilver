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

#ifndef __NEO_MISC_H_
#define __NEO_MISC_H_ 1

#include <stdlib.h>
#include <time.h>
#include <limits.h>

/* In case they didn't start from ClearSilver.h. */
#ifndef __CS_CONFIG_H_
#include "cs_config.h"
#endif

/* Fix Up for systems that don't define these standard things */
#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

#define PATH_BUF_SIZE 512

#ifndef S_IXGRP
#define S_IXGRP S_IXUSR
#endif
#ifndef S_IWGRP
#define S_IWGRP S_IWUSR
#endif
#ifndef S_IRGRP
#define S_IRGRP S_IRUSR
#endif
#ifndef S_IXOTH
#define S_IXOTH S_IXUSR
#endif
#ifndef S_IWOTH
#define S_IWOTH S_IWUSR
#endif
#ifndef S_IROTH
#define S_IROTH S_IRUSR
#endif

/* Format string checking for compilers that support it (GCC style) */

#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ > 6
#define ATTRIBUTE_PRINTF(a1,a2) __attribute__((__format__ (__printf__, a1, a2)))
#else
#define ATTRIBUTE_PRINTF(a1,a2)
#endif

/* Technically, we could do this in configure and detect what their compiler
 * can handle, but for now... */
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define USE_C99_VARARG_MACROS 1
#elif __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 4) || defined (S_SPLINT_S)
#define USE_GNUC_VARARG_MACROS 1
#else
#error The compiler is missing support for variable-argument macros.
#endif

/* For compilers (well, cpp actually) which don't define __PRETTY_FUNCTION__ */
#ifndef __GNUC__
#define __PRETTY_FUNCTION__ "unknown_function"
#endif


__BEGIN_DECLS

#ifndef HAVE_STRTOK_R
char * strtok_r (char *s,const char * delim, char **save_ptr);
#endif

#ifndef HAVE_LOCALTIME_R
struct tm *localtime_r (const time_t *timep, struct tm *ttm);
#endif

#ifndef HAVE_GMTIME_R
struct tm *gmtime_r(const time_t *timep, struct tm *ttm);
#endif

#ifndef HAVE_MKSTEMP
int mkstemp(char *path);
#endif

#ifndef HAVE_SNPRINTF
int snprintf (char *str, size_t count, const char *fmt, ...)
              ATTRIBUTE_PRINTF(3,4);
#endif

#ifndef SWIG // va_list causes problems for SWIG.
#ifndef HAVE_VSNPRINTF
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#endif
#endif

#include <stdarg.h>
#include <sys/types.h>

typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short int UINT16;
typedef short int INT16;
typedef unsigned char UINT8;
typedef char INT8;
typedef char BOOL;

#ifndef MIN
#define MIN(x,y)        (((x) < (y)) ? (x) : (y))
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*************************************************************************/
/* Logging facilities
 * Vastly over-simplified but somwhat like log4j as implemented by someone who
 * never really looked at any log4j docs and didn't want to implement the whole
 * thing anyways.
 */

/* Display options for log line prefix */
#define NE_LOG_DISPLAY_DATETIME (1<<0)
#define NE_LOG_DISPLAY_FUNCTION (1<<1)
#define NE_LOG_DISPLAY_FILELINE (1<<2)
#define NE_LOG_DISPLAY_PID (1<<3)

/* Log levels */
#define NE_LOG_ERROR 0
#define NE_LOG_WARN 1
#define NE_LOG_INFO 2
#define NE_LOG_FINE 3

/*
 * function: ne_set_log_options
 * description: Set display options for logging
 * arguments: Bitmask of NE_LOG_DISPLAY* bits to enable various display options
 *            for the log prefix.  Default is just NE_LOG_DISPLAY_DATETIME.
 *            Setting is global.
 * returns: Nothing.
 */
void ne_set_log_options(int display_options);

/*
 * function: ne_set_log
 * description: Set logging level
 * arguments: log entries <= level are logged, higher ones are ignored.
 *            default is NE_LOG_WARN.
 *            Setting is global.
 * returns: Nothing.
 */
void ne_set_log (int level);

/*
 * function: ne_log
 * description: Output's a log entry to stderr with the configured prefix
 *              if level <= the value set by ne_set_log
 * arguments: level - NE_LOG level.
 *            fmt - printf-style format string
 *            The macro insures that the function name, file, and lineno are
 *            passed, they are only displayed if configured to do so.
 * returns: Nothing.
 */
#if defined(USE_C99_VARARG_MACROS)
#define ne_log(l,f,...) \
   ne_logf(__PRETTY_FUNCTION__,__FILE__,__LINE__,l,f,__VA_ARGS__)
#elif defined(USE_GNUC_VARARG_MACROS)
#define ne_log(l,f,a...) \
   ne_logf(__PRETTY_FUNCTION__,__FILE__,__LINE__,l,f,##a)
#endif
void ne_logf(const char *func, const char *file, int lineno, int level,
             const char *fmt, ...)
             ATTRIBUTE_PRINTF(5,6);
/*
 * function: ne_vwarn
 * description: Output log entry at warning level to stderr with
 *              the configured prefix
 * arguments: This version takes a printf-style format string and a va_list.
 *            The macro insures that the function name, file, and lineno are
 *            passed, they are only displayed if configured to do so.
 * returns: Nothing.
 */
#ifndef SWIG // va_list causes problems for SWIG.
#define ne_vwarn(f, ap) \
  ne_vlogf(__PRETTY_FUNCTION__,__FILE__,__LINE__, NE_LOG_WARN, f, ap)
#endif

/*
 * function: ne_warn
 * description: Output log entry at warning level to stderr with
 *              the configured prefix
 * arguments: This version takes a printf-style format string and variable
 *            arguments.
 *            The macro insures that the function name, file, and lineno are
 *            passed, they are only displayed if configured to do so.
 * returns: Nothing.
 */
#if defined(USE_C99_VARARG_MACROS)
#define ne_warn(f,...) \
   ne_logf(__PRETTY_FUNCTION__,__FILE__,__LINE__,NE_LOG_WARN,f,__VA_ARGS__)
#elif defined(USE_GNUC_VARARG_MACROS)
#define ne_warn(f,a...) \
   ne_logf(__PRETTY_FUNCTION__,__FILE__,__LINE__,NE_LOG_WARN,f,##a)
#endif


UINT32 python_string_hash (const char *s);
UINT8 *ne_stream4 (UINT8  *dest, UINT32 num);
UINT8 *ne_unstream4 (UINT32 *pnum, UINT8 *src);
UINT8 *ne_stream2 (UINT8  *dest, UINT16 num);
UINT8 *ne_unstream2 (UINT16 *pnum, UINT8 *src);
UINT8 *ne_stream_str (UINT8 *dest, const char *s, int l);
UINT8 *ne_unstream_str (char *s, int l, UINT8 *src);
double ne_timef (void);
UINT32 ne_crc (UINT8 *data, UINT32 bytes);

__END_DECLS

#endif /* __NEO_MISC_H_ */
