
#include <unistd.h>
#include <neo_hdf.h>
#include <neo_test.h>

int main(int argc, char *argv[])
{
  NEOERR *err;
  HDF *hdf, *h2;


  err = hdf_init(&hdf);
  if (err != STATUS_OK) 
  {
    nerr_log_error(err);
    return -1;
  }

  err = hdf_set_value(hdf, "CGI.Foo", "Bar");
  if (err) 
  {
    nerr_log_error(err);
    return -1;
  }
  err = hdf_set_value(hdf, "CGI.Foo", "Baz");
  if (err) 
  {
    nerr_log_error(err);
    return -1;
  }

  h2 = hdf_get_obj(hdf, "CGI");
  err = hdf_set_value(h2, "Foo", "Bang");

  hdf_dump(hdf, NULL); 

  hdf_destroy(&hdf);

  return 0;
}
