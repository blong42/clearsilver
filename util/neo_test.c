
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "neo_misc.h"
#include "ulist.h"

static int RandomInit = 0;

void neot_seed_rand (long int seed)
{
  ne_warn ("Rand Seed is %ld", seed);
  srand48(seed);
  RandomInit = 1;
}

int neot_rand (int max)
{
  int r;

  if (RandomInit == 0)
  {
    neot_seed_rand (time(NULL));
  }
  r = drand48() * max;
  return r;
}

int neot_rand_string (char *s, int max)
{
  int size;
  int x = 0;

  size = neot_rand(max-1);
  for (x = 0; x < size; x++)
  {
    s[x] = (char)(32 + neot_rand(127-32));
    if (s[x] == '/') s[x] = ' ';
  }
  s[x] = '\0';

  return 0;
}

static ULIST *Words = NULL;

int neot_rand_word (char *s, int max)
{
  NEOERR *err;
  int x;
  char *word;

  if (Words == NULL)
  {
    FILE *fp;
    char buf[256];

    err = uListInit(&Words, 40000, 0);
    if (err) 
    {
      nerr_log_error(err);
      return -1;
    }
    fp = fopen ("/usr/dict/words", "r");
    while (fgets (buf, sizeof(buf), fp) != NULL)
    {
      x = strlen (buf);
      if (buf[x-1] == '\n')
	buf[x-1] = '\0';
      uListAppend(Words, strdup(buf));
    }
    fclose (fp);
  }
  x = neot_rand (uListLength(Words));
  uListGet(Words, x, (void *)&word);
  strncpy (s, word, max);
  s[max-1] = '\0';

  return 0;
}

