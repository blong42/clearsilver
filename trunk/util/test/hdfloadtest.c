
#include <unistd.h>
#include <neo_hdf.h>
#include <neo_test.h>

int main(int argc, char *argv[])
{
  NEOERR *err;
  HDF *hdf;
  int x;
  char name[256];
  char value[256];
  double tstart = 0;

  err = hdf_init(&hdf);
  if (err != STATUS_OK) 
  {
    nerr_log_error(err);
    return -1;
  }

  for (x = 0; x < 1000; x++)
  {
    err = hdf_read_file (hdf, "test.hdf");
    if (err != STATUS_OK) 
    {
      nerr_log_error(err);
      return -1;
    }
  }
  hdf_dump(hdf, NULL);

  hdf_destroy(&hdf);

  return 0;
}
