/* Missing Functions 
 * 
 * The functions included here are provided as replacements for system
 * library functions that are missing (or broken).
 *
 * Yes, configure usually tries to get you to have a single file for
 * each function, but pppbbbttthhhhh.
 *
 * To my knowledge, each of these replacement functions are available as
 * free code... if not, I'll have to steal them from somewhere else that
 * is free (hmm, FreeBSD libc?)
 */

#include "cs_config.h"

#ifndef HAVE_STRTOK_R
#include <string.h>

/* from glibc */
/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
     char s[] = "-abc-=-def";
     char *sp;
     x = strtok_r(s, "-", &sp);  // x = "abc", sp = "=-def"
     x = strtok_r(NULL, "-=", &sp);  // x = "def", sp = NULL
     x = strtok_r(NULL, "=", &sp);   // x = NULL 
          // s = "abc\0-def\0"
*/

char * strtok_r (char *s,const char * delim, char **save_ptr)
{
  char *token;

  if (s == NULL)
    s = *save_ptr;

  /* Scan leading delimiters.  */
  s += strspn (s, delim);
  if (*s == '\0')
  {
    *save_ptr = s;
    return NULL;
  }

  /* Find the end of the token.  */
  token = s;
  s = strpbrk (token, delim);
  if (s == NULL)
    /* This token finishes the string.  */
    /**save_ptr = __rawmemchr (token, '\0');*/
    *save_ptr = strchr (token, '\0');
  else
  {
    /* Terminate the token and make
     * *SAVE_PTR point past it.  */
    *s = '\0';
    *save_ptr = s + 1;
  }
  return token;
}
#endif

#include <time.h>

#ifndef HAVE_LOCALTIME_R

struct tm *localtime_r (const time_t *timep, struct tm *ttm)
{
  ttm = localtime(timep);
  return ttm;
}
#endif

#ifndef HAVE_GMTIME_R
struct tm *gmtime_r(const time_t *timep, struct tm *ttm)
{
  ttm = gmtime(timep);
  return ttm;
}

#endif

#ifndef HAVE_MKSTEMP
#include <fcntl.h>

int mkstemp(char *path) 
{
  return open(mktemp(path),O_RDWR);
}
#endif
