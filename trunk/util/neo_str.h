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

#ifndef __NEO_STR_H_
#define __NEO_STR_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include <stdio.h>
#include "util/neo_misc.h"

/* This modifies the string its called with by replacing all the white
 * space on the end with \0, and returns a pointer to the first
 * non-white space character in the string 
 */
char *neos_strip (char *s);

void neos_lower (char *s);

char *sprintf_alloc (const char *fmt, ...);
char *nsprintf_alloc (int start_size, const char *fmt, ...);
char *vsprintf_alloc (const char *fmt, va_list ap);
char *vnsprintf_alloc (int start_size, const char *fmt, va_list ap);

/* Versions of the above which actually return a length, necessary if 
 * you expect embedded NULLs */
int vnisprintf_alloc (char **buf, int start_size, const char *fmt, va_list ap);
int visprintf_alloc (char **buf, const char *fmt, va_list ap);
int isprintf_alloc (char **buf, const char *fmt, ...);

typedef struct _string
{
  char *buf;
  int len;
  int max;
} STRING;

typedef struct _string_array
{
  char **entries;
  int count;
  int max;
} STRING_ARRAY;


/* At some point, we should add the concept of "max len" to these so we
 * can't get DoS'd by someone sending us a line without an end point,
 * etc. */
void string_init (STRING *str);
NEOERR *string_set (STRING *str, const char *buf);
NEOERR *string_append (STRING *str, const char *buf);
NEOERR *string_appendn (STRING *str, const char *buf, int l);
NEOERR *string_append_char (STRING *str, char c);
NEOERR *string_appendf (STRING *str, const char *fmt, ...);
NEOERR *string_appendvf (STRING *str, const char *fmt, va_list ap);
NEOERR *string_readline (STRING *str, FILE *fp);
void string_clear (STRING *str);

/* typedef struct _ulist ULIST; */
#include "util/ulist.h"
/* s is not const because we actually temporarily modify the string
 * during split */
NEOERR *string_array_split (ULIST **list, char *s, const char *sep, 
                            int max);

BOOL reg_search (const char *re, const char *str);


NEOERR* neos_escape(UINT8 *buf, int buflen, char esc_char, const char *escape, 
                    char **esc);
UINT8 *neos_unescape (UINT8 *s, int buflen, char esc_char);

char *repr_string_alloc (const char *s);

__END_DECLS

#endif /* __NEO_STR_H_ */
