
#include <ClearSilver.h>


int main() {
  HDF *hdf;
  NEOERR *err;

  err = hdf_init(&hdf);

  if (err) {
      printf("error: %s\n", err->desc);
      return 1;
   }

   printf("success: 0x%X\n", hdf);
}
