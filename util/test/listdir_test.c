
#include "cs_config.h"
#include <stdio.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/ulist.h"
#include "util/neo_files.h"

int main(int argc, char **argv)
{
  char *path;
  ULIST *files = NULL;
  char *filename;
  NEOERR *err;
  int x;

  if (argc > 1)
    path = argv[1];
  else
    path = ".";

  ne_warn("Testing ne_listdir()");
  err = ne_listdir(path, &files);
  if (err)
  {
    nerr_log_error(err);
    return -1;
  }

  for (x = 0; x < uListLength(files); x++)
  {
    err = uListGet(files, x, (void *)&filename);
    printf("%s\n", filename);
  }

  uListDestroy(&files, ULIST_FREE);

  ne_warn("Testing ne_listdir_match() with *.c");
  err = ne_listdir_match(path, &files, "*.c");
  if (err)
  {
    nerr_log_error(err);
    return -1;
  }

  for (x = 0; x < uListLength(files); x++)
  {
    err = uListGet(files, x, (void *)&filename);
    printf("%s\n", filename);
  }

  uListDestroy(&files, ULIST_FREE);
  return 0;
}
