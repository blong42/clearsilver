
#include "cs_config.h"
#include <unistd.h>
#include <string.h>
#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "util/neo_rand.h"

NEOERR* hdf_set_valuef(HDF *hdf, char *namefmt, char *valuefmt, ...)
{
  char newstr[2048];
  char one[2];
  va_list ap;
  char *nametok;
  char *valtok;

  char newfmt[2048];


  one[0]=(char)1;
  one[1]=(char)0;

  snprintf(newfmt,2048,"%s%s%s",namefmt,one,valuefmt);

  va_start(ap, valuefmt);
  vsnprintf (newstr, 1024, newfmt, ap);
  va_end(ap);

  /* split it at 1 */
  nametok=strtok(newstr,one);
  valtok=strtok(NULL,one);


  return hdf_set_value(hdf,nametok,valtok);
}


int TestCompare(const void* pa, const void* pb)
{
  HDF **a = (HDF **)pa;
  HDF **b = (HDF **)pb;
  float aVal,bVal;

  aVal = atof(hdf_get_value(*a,"val","0"));
  bVal = atof(hdf_get_value(*b,"val","0"));

  printf("TestCompare aVal=%f [%s]  bVal=%f [%s]\n",aVal,hdf_get_value(*a,"name","?"),bVal,hdf_get_value(*b,"name","?"));

  if (aVal<bVal) return -1;
  if (aVal==bVal) return 0;
  return 1;
}

void TestSort(HDF* hdf)
{
  int i;
  float value;

  for (i=0;i<15;i++)
  {
    value = rand()/(RAND_MAX+1.0);

    hdf_set_valuef(hdf, "test.%d", "%d", i, i);
    hdf_set_valuef(hdf, "test.%d.name", "item #%d", i, i);
    hdf_set_valuef(hdf, "test.%d.val", "%f", i, value );
  }

  hdf_dump(hdf,NULL);

  hdf_sort_obj(hdf_get_obj(hdf, "test"), TestCompare);

  hdf_dump(hdf,NULL);

}


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

  tstart = ne_timef();
  TestSort(hdf);
  ne_warn("sort took %5.5fs", ne_timef() - tstart);

  hdf_dump(hdf, NULL);

  hdf_destroy(&hdf);

  return 0;
}
