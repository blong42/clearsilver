/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#ifndef __NEO_STR_H_
#define __NEO_STR_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include "util/neo_misc.h"

/* This modifies the string its called with by replacing all the white
 * space on the end with \0, and returns a pointer to the first
 * non-white space character in the string 
 */
char *neos_strip (char *s);

char *sprintf_alloc (char *fmt, ...);
char *vsprintf_alloc (char *fmt, va_list ap);

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

void string_init (STRING *str);
NEOERR *string_append (STRING *str, char *buf);
NEOERR *string_appendn (STRING *str, char *buf, int l);
NEOERR *string_append_char (STRING *str, char c);
NEOERR *string_appendf (STRING *str, char *fmt, ...);
NEOERR *string_appendvf (STRING *str, char *fmt, va_list ap);
void string_clear (STRING *str);

/* typedef struct _ulist ULIST; */
#include "ulist.h"
NEOERR *string_array_split (ULIST **list, char *s, char *sep, int max);

BOOL reg_search (char *re, char *str);


__END_DECLS

#endif /* __NEO_STR_H_ */
