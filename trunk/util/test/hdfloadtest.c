
#include <unistd.h>
#include <neo_hdf.h>
#include <neo_test.h>

int main(int argc, char *argv[])
{
  NEOERR *err;
  HDF *hdf;
  int x;
  double tstart = 0;
  char *file;
  int reps = 1000;

  if (argc > 1)
    file = argv[1];
  else
    file = "test.hdf";

  if (argc > 2)
    reps = atoi(argv[2]);

  err = hdf_init(&hdf);
  if (err != STATUS_OK) 
  {
    nerr_log_error(err);
    return -1;
  }

  tstart = ne_timef();

  for (x = 0; x < reps; x++)
  {
    err = hdf_read_file (hdf, file);
    if (err != STATUS_OK) 
    {
      nerr_log_error(err);
      return -1;
    }
  }
  ne_warn("Load test finished in %5.3fs", ne_timef() - tstart);
  /* hdf_dump(hdf, NULL); */

  hdf_destroy(&hdf);

  return 0;
}
