/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#include <unistd.h>
#include <ctype.h>
#include "neo_str.h"

char *neos_strip (char *s)
{
  int x;

  x = strlen(s) - 1;
  while (x>=0 && isspace(s[x])) s[x--] = '\0';

  while (*s && isspace(*s)) s++;
  
  return s;
}
