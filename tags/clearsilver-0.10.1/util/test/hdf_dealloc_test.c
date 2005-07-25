

#include "cs_config.h"
#include <unistd.h>
#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "util/neo_rand.h"

int main(int argc, char *argv[])
{
  HDF *hdf = NULL;
  int i, j;

  hdf_init(&hdf);

  ne_warn("creating 100000x10 nodes");
  for (i = 0; i < 100000; i++) {
    char buffer[64];
    for (j = 0; j < 10; j++) {
      snprintf(buffer, sizeof(buffer), "node.%d.test.%d", i, j);
      hdf_set_value(hdf, buffer, "test");
    }
  }

  ne_warn("calling dealloc");
  hdf_destroy(&hdf);    // <-- this takes forever to return with a hugely
  return 0;
}
