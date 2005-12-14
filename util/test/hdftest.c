
#include "cs_config.h"
#include <unistd.h>
#include <string.h>
#include "util/neo_misc.h"
#include "util/neo_hdf.h"
#include "util/neo_rand.h"

#define DIE_NOT_OK(err) \
  if (err != STATUS_OK) { \
      nerr_log_error(err); \
      exit(-1); \
  }


int rand_name (char *s, int slen)
{
  char buf[256];
  int x, m, l, rl;

  m = neo_rand(10);
  while (1) {
    neo_rand_word(s, slen);
    if (!strchr(s, '.')) break;
  }

  for (x = 1; x < m; x++)
  {
    l = strlen(s);
    neo_rand_word(buf, sizeof(buf));
    rl = strlen(buf);
    /* fprintf(stderr, "%s\n", buf); */
    if (rl && slen - l - rl > 1 && !strchr(buf, '.')) {
      snprintf(s + l, slen - l, ".%s", buf);
    }
  }

  return 0;
}

static int sortByName(const void *a, const void *b) {
  HDF **ha = (HDF **)a;
  HDF **hb = (HDF **)b;

  /* fprintf(stderr, "%s <=> %s\n", hdf_obj_name(*ha), hdf_obj_name(*hb));  */
  return strcasecmp(hdf_obj_name(*ha), hdf_obj_name(*hb));
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
  DIE_NOT_OK(err);

  err = hdf_set_value (hdf, "Beware", "1");
  DIE_NOT_OK(err);
  err = hdf_set_value (hdf, "Beware.The", "2");
  DIE_NOT_OK(err);
  err = hdf_set_valuef (hdf, "Beware.The.%s=%d", "Ides", 3);
  DIE_NOT_OK(err);
  err = hdf_set_value (hdf, "Beware.Off", "4");
  DIE_NOT_OK(err);
  err = hdf_set_value (hdf, "Beware.The.Ides.Of", "5");
  DIE_NOT_OK(err);
  err = hdf_set_value (hdf, "Beware.The.Butter", "6");
  DIE_NOT_OK(err);
  err = hdf_set_attr (hdf, "Beware.The.Butter", "Lang", "en");
  DIE_NOT_OK(err);
  err = hdf_set_attr (hdf, "Beware.The.Butter", "Lang", "1");
  DIE_NOT_OK(err);
  err = hdf_set_attr (hdf, "Beware.The.Butter", "Lang", NULL);
  DIE_NOT_OK(err);

  err = hdf_read_file (hdf, "test.hdf");
  DIE_NOT_OK(err);
  hdf_dump(hdf, NULL);


  x = hdf_get_int_value (hdf, "Beware.The.Ides", 0);
  if (x != 3)
  {
    ne_warn("hdf_get_int_value returned %d, expected 3", x);
    return -1;
  } 

  /* test symlinks */
  {
    const char *v;
    err = hdf_set_value(hdf, "Destination.Foo", "bar");
    DIE_NOT_OK(err);
    err = hdf_set_symlink(hdf, "Symlink.baz", "Destination.Foo");
    DIE_NOT_OK(err);
    v = hdf_get_value(hdf, "Symlink.baz", "notfound");
    if (strcmp(v, "bar")) {
      ne_warn("hdf_get_value through symlink returned %s, expected bar", v);
      return -1;
    }
    err = hdf_set_value(hdf, "Symlink.baz", "newvalue");
    DIE_NOT_OK(err);
    v = hdf_get_value(hdf, "Symlink.baz", "notfound");
    if (strcmp(v, "newvalue")) {
      ne_warn("hdf_get_value through symlink returned %s, expected newvalue",
              v);
      return -1;
    }
    err = hdf_set_value(hdf, "Symlink.baz.too", "newtoo");
    DIE_NOT_OK(err);
    v = hdf_get_value(hdf, "Symlink.baz.too", "newtoo");
    if (strcmp(v, "newtoo")) {
      ne_warn("hdf_get_value through symlink returned %s, expected newtoo",
              v);
      return -1;
    }
    v = hdf_get_value(hdf, "Destination.Foo.too", "newtoo");
    if (strcmp(v, "newtoo")) {
      ne_warn("hdf_get_value through symlink returned %s, expected newtoo",
              v);
      return -1;
    }
  }

  for (x = 0; x < 10000; x++)
  {
    rand_name(name, sizeof(name));
    neo_rand_word(value, sizeof(value));
    /* ne_warn("Setting %s = %s", name, value); */
    err = hdf_set_value (hdf, name, value);
    DIE_NOT_OK(err);
  }

  tstart = ne_timef();
  hdf_sort_obj(hdf, sortByName);
  ne_warn("sort took %5.5fs", ne_timef() - tstart);

  hdf_dump(hdf, NULL);

  hdf_destroy(&hdf);

  return 0;
}
